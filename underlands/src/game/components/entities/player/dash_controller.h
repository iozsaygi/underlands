#ifndef DASH_CONTROLLER_H
#define DASH_CONTROLLER_H

#include "engine.h"
#include "game/components/entities/spawners/enemy_spawner.h"
#include "game/components/entities/player/player.h"
#include "game/components/ai/chase_controller.h"

class DashController : public engine::Component
{
public:
	inline DashController( engine::EngineEntry *engineEntry, engine::Actor *owner, float dashRange ) : Component( owner )
	{
		m_EngineEntry = engineEntry;
		m_DashRange   = dashRange;
	}

	void Birth() override;
	void OnTick( float deltaTime ) override;

private:
	engine::EngineEntry *m_EngineEntry = nullptr;
	bool m_IsDashTriggered = false;
	float m_DashRange = 0.0f;
	engine::Transform *m_OwnerTransform = nullptr;
	Player *m_Player = nullptr;
	engine::Text *m_PlayerScoreText = nullptr;
	EnemySpawner *m_EnemySpawner = nullptr;
	engine::AudioPlayer *m_PlayerAudioPlayer = nullptr;
	engine::AudioPlayer *m_PlayerScanFailAP = nullptr;

	void Dash();
	void ResetDashState();
	engine::Actor *GetClosestEnemy( std::vector<ChaseController *> &enemies );
};

#endif // !DASH_CONTROLLER_H