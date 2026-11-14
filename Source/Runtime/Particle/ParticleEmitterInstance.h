#pragma once

#include "Particle/ParticleTypes.h"
#include "Math/Matrix.h"

class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;
struct FParticleEmitterRenderData;

struct FParticleEmitterInstance
{
	UParticleEmitter* EmitterTemplate = nullptr;
	UParticleLODLevel* CurrentLODLevel = nullptr;
	UParticleSystemComponent* Owner = nullptr;

	uint8* ParticleData = nullptr; // 파티클 데이터 배열 시작 주소
	int32 ParticleStride = 0;
	int32 MaxActiveParticles = 0;

	uint16* ParticleIndices = nullptr; // 활성화된 파티클 인덱스 배열 시작 주소
	int32 ActiveParticles = 0;

	float SpawnFraction = 0.f;

	FMatrix EmitterToSimulate;
	FMatrix SimulateToWorld;

	FParticleEmitterInstance() = default;

	~FParticleEmitterInstance();

	FParticleEmitterInstance(const FParticleEmitterInstance& Other) = delete;
	FParticleEmitterInstance& operator=(const FParticleEmitterInstance& Other) = delete;

	void InitParameters(UParticleEmitter* Emitter, UParticleSystemComponent* InOwner);

	void Initialize();

	inline FBaseParticle* GetParticleFromPosition(int32 Position) const
	{
		return (FBaseParticle*)&ParticleData[ParticleIndices[Position] * ParticleStride];
	}

	void SpawnParticles(int32 Count, const FVector& InitLocation);

	void SpawnParticle(const FVector& InitLocation);

	void KillParticle(int32 Position);

	void UpdateTransforms();

	void Tick(float DeltaTime);

	void BuildRenderData(FParticleEmitterRenderData& RenderData) const;
};
