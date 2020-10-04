#include <assert.h>
#include <SDL.h>
#include "core/event_system/input.h"
#include "world.h"

namespace engine
{
	World::World( EngineEntry *engineEntry )
	{
		assert( engineEntry != nullptr );
		m_EngineEntry = engineEntry;
		m_Actors = std::vector<Actor *>();
		m_ActorsToDestroy = std::vector<Actor *>();
		m_RenderersInWorld = std::vector<SpriteRenderer *>();
		m_CollidersInWorld = std::vector<BoxCollider2D *>();
		m_Texts = std::vector<Text *>();
	}

	World::World( EngineEntry *engineEntry, bool isActive )
	{
		assert( engineEntry != nullptr );
		m_EngineEntry = engineEntry;
		m_Actors = std::vector<Actor *>();
		m_ActorsToDestroy = std::vector<Actor *>();
		m_RenderersInWorld = std::vector<SpriteRenderer *>();
		m_CollidersInWorld = std::vector<BoxCollider2D *>();
		m_Texts = std::vector<Text *>();
		m_IsActive = isActive;
	}

	World::~World()
	{
		for ( auto actor : m_Actors )
			delete actor;

		m_Actors.clear();
		m_RenderersInWorld.clear();
		m_CollidersInWorld.clear();
		m_Texts.clear();
	}

	void World::Tick()
	{
		for ( auto actor : m_Actors )
		{
			if ( actor->GetIsActive() )
			{
				SpriteRenderer *spriteRenderer = actor->GetComponent<SpriteRenderer>();
				if ( spriteRenderer != nullptr && spriteRenderer->GetIsEnabled() )
					m_RenderersInWorld.push_back( spriteRenderer );

				BoxCollider2D *boxCollider2D = actor->GetComponent<BoxCollider2D>();
				if ( boxCollider2D != nullptr && boxCollider2D->GetIsEnabled() )
					m_CollidersInWorld.push_back( boxCollider2D );

				Text *text = actor->GetComponent<Text>();
				if ( text != nullptr )
					m_Texts.push_back( text );
			}
		}

		// That make a big mess...
		for ( auto actor : m_Actors )
		{
			if ( actor->GetIsActive() )
			{
				for ( auto component : actor->GetComponents() )
				{
					if ( component->GetIsEnabled() )
						component->Birth();
				}
			}
		}

		for ( auto actor : m_Actors )
		{
			if ( actor->GetIsActive() )
			{
				for ( auto component : actor->GetComponents() )
				{
					if ( component->GetIsEnabled() )
						component->Begin();
				}
			}
		}

		const int frameDelay = 1000 / m_EngineEntry->GetTargetFPS();
		Uint32 frameStart = 0;
		int frameTime = 0;

		while ( m_IsActive )
		{
			float deltaTime = ( SDL_GetTicks() - frameStart ) / 1000.0f;
			frameStart = SDL_GetTicks();

			ProcessEvents();
			UpdateActors( deltaTime );
			HandleCollisions();
			Render();
			RenderUI();
			DestroyMarkedActors();

			frameTime = SDL_GetTicks() - frameStart;
			if ( frameDelay > frameTime )
				SDL_Delay( frameDelay - frameTime );
		}

		for ( auto actor : m_Actors )
		{
			for ( auto component : actor->GetComponents() )
			{
				if ( component->GetIsEnabled() )
					component->OnShutdown();
			}
		}
	}

	void World::AddActor( Actor *actor )
	{
		assert( actor != nullptr );
		m_Actors.push_back( actor );
		actor->SetOwnerWorld( this );
	}

	void World::AddActorRuntime( Actor *actor )
	{
		assert( actor != nullptr );
		m_Actors.push_back( actor );
		actor->SetOwnerWorld( this );

		if ( actor->GetIsActive() )
		{
			for ( auto component : actor->GetComponents() )
			{
				if ( component->GetIsEnabled() )
					component->Birth();
			}

			for ( auto component : actor->GetComponents() )
			{
				if ( component->GetIsEnabled() )
					component->Begin();
			}

			SpriteRenderer *spriteRenderer = actor->GetComponent<SpriteRenderer>();
			if ( spriteRenderer != nullptr && spriteRenderer->GetIsEnabled() )
				m_RenderersInWorld.push_back( spriteRenderer );

			// Add actor's collider to the collision map. (If actor has one)
			BoxCollider2D *boxCollider2D = actor->GetComponent<BoxCollider2D>();
			if ( boxCollider2D != nullptr && boxCollider2D->GetIsEnabled() )
				m_CollidersInWorld.push_back( boxCollider2D );

			Text *text = actor->GetComponent<Text>();
			if ( text != nullptr )
				m_Texts.push_back( text );
		}
	}

	std::vector<Actor *> &World::GetActors()
	{
		return m_Actors;
	}

	void World::ProcessEvents()
	{
		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			if ( event.type == SDL_QUIT )
				m_IsActive = false;

			Input::UpdateKeyStates( event );
		}
	}

	void World::UpdateActors( float deltaTime )
	{
		int numberObActorsThisFrame = m_Actors.size();
		for ( int i = 0; i < numberObActorsThisFrame; i++ )
		{
			if ( m_Actors[ i ]->GetIsActive() )
			{
				for ( auto component : m_Actors[ i ]->GetComponents() )
				{
					if ( component->GetIsEnabled() )
						component->OnTick( deltaTime );
				}
			}
		}
	}

	void World::HandleCollisions()
	{
		for ( int i = 0; i < m_CollidersInWorld.size(); i++ )
		{
			if ( i + 1 < m_CollidersInWorld.size() )
			{
				if ( m_CollidersInWorld[ i ]->GetOwner()->GetIsActive() && m_CollidersInWorld[ i + 1 ]->GetOwner()->GetIsActive() )
				{
					if ( SDL_HasIntersection( &m_CollidersInWorld[ i ]->GetColliderRectangle(), &m_CollidersInWorld[ i + 1 ]->GetColliderRectangle() ) )
					{
						for ( auto component : m_CollidersInWorld[ i ]->GetOwner()->GetComponents() )
						{
							if ( component->GetIsEnabled() )
								component->OnCollision( m_CollidersInWorld[ i + 1 ]->GetOwner() );
						}

						for ( auto component : m_CollidersInWorld[ i + 1 ]->GetOwner()->GetComponents() )
						{
							if ( component->GetIsEnabled() )
								component->OnCollision( m_CollidersInWorld[ i ]->GetOwner() );
						}
					}
				}
			}
		}
	}

	void World::Render()
	{
		auto renderer = m_EngineEntry->GetRenderer();
		if ( renderer != nullptr )
		{
			SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
			SDL_RenderClear( renderer );

			for ( auto spriteRenderer : m_RenderersInWorld )
			{
				if ( spriteRenderer != nullptr && spriteRenderer->GetOwner()->GetIsActive() && spriteRenderer->GetIsEnabled() )
					spriteRenderer->Render( renderer );
			}

			SDL_RenderPresent( renderer );
		}
	}

	void World::RenderUI()
	{
		auto renderer = m_EngineEntry->GetRenderer();
		if ( renderer != nullptr )
		{
			for ( auto text : m_Texts )
				text->Render();

			SDL_RenderPresent( renderer );
		}
	}

	void World::DestroyMarkedActors()
	{
		for ( auto actor : m_ActorsToDestroy )
		{
			// Remove the actor from our "m_Actors" vector.
			auto actorRemoveIt = std::find( m_Actors.begin(), m_Actors.end(), actor );
			if ( actorRemoveIt != m_Actors.end() )
				m_Actors.erase( actorRemoveIt );

			SpriteRenderer *spriteRenderer = actor->GetComponent<SpriteRenderer>();
			if ( spriteRenderer != nullptr )
			{
				auto rendererRemoveIt = std::find( m_RenderersInWorld.begin(), m_RenderersInWorld.end(), spriteRenderer );
				if ( rendererRemoveIt != m_RenderersInWorld.end() )
					m_RenderersInWorld.erase( rendererRemoveIt );
			}

			// If actor has collider component, also remove it from our collider map.
			BoxCollider2D *boxCollider2D = actor->GetComponent<BoxCollider2D>();
			if ( boxCollider2D != nullptr )
			{
				auto colliderRemoveIt = std::find( m_CollidersInWorld.begin(), m_CollidersInWorld.end(), boxCollider2D );
				if ( colliderRemoveIt != m_CollidersInWorld.end() )
					m_CollidersInWorld.erase( colliderRemoveIt );
			}

			Text *text = actor->GetComponent<Text>();
			if ( text != nullptr )
			{
				auto textRemoveIt = std::find( m_Texts.begin(), m_Texts.end(), text );
				if ( textRemoveIt != m_Texts.end() )
					m_Texts.erase( textRemoveIt );
			}

			delete actor;
		}

		m_ActorsToDestroy.clear();
	}

	Actor *World::GetActorByName( const std::string &name )
	{
		for ( auto actor : m_Actors )
		{
			if ( actor != nullptr )
			{
				if ( actor->GetName() == name )
					return actor;
			}
		}

		return nullptr;
	}

	void World::ScheduleActorForDestroy( Actor *actor )
	{
		assert( actor != nullptr );
		m_ActorsToDestroy.push_back( actor );
	}

	std::vector<BoxCollider2D *> World::GetCollidersInWorld()
	{
		return m_CollidersInWorld;
	}
}