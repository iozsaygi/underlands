#include <assert.h>
#include <vector>
#include "dash_controller.h"
#include <iostream>

void DashController::Birth()
{
	assert( GetOwner() != nullptr );
	m_OwnerTransform    = GetOwner()->GetComponent<engine::Transform>();
	m_Player            = GetOwner()->GetComponent<Player>();
	m_PlayerScoreText   = GetOwner()->GetOwnerWorld()->GetActorByName( "Player Score Text" )->GetComponent<engine::Text>();
	m_EnemySpawner      = GetOwner()->GetOwnerWorld()->GetActorByName( "Enemy Spawner" )->GetComponent<EnemySpawner>();
	m_PlayerAudioPlayer = GetOwner()->GetComponent<engine::AudioPlayer>();
	m_PlayerScanFailAP  = GetOwner()->GetOwnerWorld()->GetActorByName( "Player Scan Fail Static Audio Player" )->GetComponent<engine::AudioPlayer>();;
}

void DashController::OnTick( float deltaTime )
{
	if ( engine::Input::MouseState.GetIsLeftMouseButtonDown() && !m_IsDashTriggered )
		Dash();

	if ( !engine::Input::MouseState.GetIsLeftMouseButtonDown() )
		ResetDashState();
}

void DashController::Dash()
{
	// Do not bother with all of this stuff if we are already in dash state.
	if ( m_IsDashTriggered )
		return;

	engine::World *ctx = engine::WorldManager::GetActiveWorld();
	if ( ctx != nullptr )
	{
		// We want to get all the alive enemies!
		std::vector<ChaseController *> enemies = std::vector<ChaseController *>();
		for ( engine::Actor *actor : ctx->GetActors() )
		{
			ChaseController *cController = actor->GetComponent<ChaseController>();
			if ( cController != nullptr )
				enemies.push_back( cController );
		}

		engine::Actor *closestEnemy = GetClosestEnemy( enemies );
		if ( closestEnemy != nullptr )
		{
			// Only dash if we are in the range.
			float distance =  engine::Vector2D::GetManhattanDistance( m_OwnerTransform->GetPosition(), closestEnemy->GetComponent<engine::Transform>()->GetPosition() );
			if ( distance <= m_DashRange )
			{

				// Disable the box collider of closest enemy so we can dash it...
				engine::BoxCollider2D *bCollider = closestEnemy->GetComponent<engine::BoxCollider2D>();
				if ( bCollider != nullptr )
					bCollider->GetIsEnabled() = false;

				// Now dash the enemy!
				m_OwnerTransform->GetPosition() = closestEnemy->GetComponent<engine::Transform>()->GetPosition();

				// Add score and update the UI.
				m_Player->AddScore( 10 );
				m_PlayerScoreText->UpdateText( std::to_string( m_Player->GetScore() ) );

				// Update the enemy spawner.
				int cScore = m_Player->GetScore();
				if ( cScore % 20 == 0 )
					m_EnemySpawner->SetSpawnRate( m_EnemySpawner->GetSpawnRate() - 0.5f );

				engine::Actor *bloodSplash = new engine::Actor( "Blood Splash" );
				BlueprintManager::CraftBloodSplash( m_EngineEntry, bloodSplash, closestEnemy->GetComponent<engine::Transform>()->GetPosition() );
				engine::WorldManager::GetActiveWorld()->AddActorRuntime( bloodSplash );

				m_PlayerAudioPlayer->PlayOneShot( 0, 80 );

				// Destroy the closest enemy next frame!
				ctx->ScheduleActorForDestroy( closestEnemy );
			}
			else
			{
				// The enemy is not in the range.
				m_PlayerScanFailAP->PlayOneShot( 1, 10 );
			}
		}
		else
		{
			// Failed to find any near enemy.
			m_PlayerScanFailAP->PlayOneShot( 1, 10 );
		}

		m_IsDashTriggered = true;
	}
}

void DashController::ResetDashState()
{
	if ( m_IsDashTriggered )
		m_IsDashTriggered = false;
}

engine::Actor *DashController::GetClosestEnemy( std::vector<ChaseController *> &enemies )
{
	engine::Actor *closestEnemy = nullptr;

	if ( enemies.size() > 0 )
	{
		closestEnemy = enemies[ 0 ]->GetOwner();
		for ( int i = 0; i < enemies.size(); i++ )
		{
			if ( i + 1 < enemies.size() )
			{
				float fDistanceToPlayer = engine::Vector2D::GetManhattanDistance( enemies[ i ]->GetOwner()->GetComponent<engine::Transform>()->GetPosition(),
					m_OwnerTransform->GetPosition() );

				float sDistanceToPlayer = engine::Vector2D::GetManhattanDistance( enemies[ i + 1 ]->GetOwner()->GetComponent<engine::Transform>()->GetPosition(),
					m_OwnerTransform->GetPosition() );

				if ( fDistanceToPlayer <= sDistanceToPlayer )
				{
					closestEnemy = enemies[ i ]->GetOwner();
					break;
				}
			}
		}
	}

	return closestEnemy;
}