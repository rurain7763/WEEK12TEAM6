#pragma once

#include "Containers/Array.h"
#include "Particle/Modules/ParticleModuleEventBase.h"
#include "Particle/ParticleTypes.h"

struct FHitResult;

struct FParticleEventGeneratorInfo
{
	FString EventName = "Collision";
	int32 Frequency = 1;
	bool bFirstTimeOnly = false;
	bool bLastTimeOnly = false;
};

class UParticleModuleEventGenerator : public UParticleModuleEventBase
{
	DECLARE_CLASS(UParticleModuleEventGenerator, UParticleModuleEventBase)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	// 충돌 1회를 보고 규칙과 비교해 통과한 규칙마다 PSC에 기록
	void HandleParticleCollision(FParticleEmitterInstance& Owner, const FBaseParticle& Particle,
		int32 CollisionCount, int32 MaxCollisions, const FHitResult& Hit, const FVector& WorldVelocity) const;

	// 복수 보고 규칙. 실행 시 이미터의 CollisionTrackingCount 하나를 공유.
	TArray<FParticleEventGeneratorInfo> Events = { FParticleEventGeneratorInfo{} };
};
