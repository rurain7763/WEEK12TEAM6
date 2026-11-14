#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"

struct FParticleEmitterInstance;
struct FBaseParticle;

class UParticleModule : public UObject
{
	DECLARE_CLASS(UParticleModule, UObject)

public:
	struct FSpawnContext
	{
		int32 Offset;
		FParticleEmitterInstance* Emitter;
		float SpawnTime;
		FBaseParticle* Particle;
	};

	struct FUpdateContext
	{
		int32 Offset;
		FParticleEmitterInstance* Emitter;
		float DeltaTime;
	};

	virtual void Spawn(const FSpawnContext& Ctx) {}
	virtual void Update(const FUpdateContext& Ctx) {}

protected:
	friend class UParticleLODLevel;

	bool bSpawnModule = false; // 파티클 생성 시 실행할 묘듈
	bool bUpdateModule = false; // 매 프레임 실행할 모듈
};
