#pragma once

#include "GameFramework/Actor.h"
#include "Particle/ParticleTypes.h"

class UParticleSystemComponent;

// 월드당 하나. PSC가 모은 파티클 이벤트를 받아 PSC의 Delegate로 전달
class AParticleEventManager : public AActor
{
	DECLARE_CLASS(AParticleEventManager, AActor)

public:
	AParticleEventManager() = default;
	~AParticleEventManager() override = default;

	// 게임 전체 공통 처리가 필요하면 상속해서 재정의
	virtual void HandleParticleCollisionEvents(UParticleSystemComponent* Component,
		const TArray<FParticleEventCollideData>& CollisionEvents);
};
