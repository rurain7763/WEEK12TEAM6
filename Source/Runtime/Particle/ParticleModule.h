#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"
#include <random>

struct FParticleEmitterInstance;
struct FBaseParticle;

struct FRandomStream
{
	inline static std::mt19937 Engine{ std::random_device{}() };

	static float FRandRange(float Min, float Max)
	{
		std::uniform_real_distribution<float> Dist(0.0f, 1.0f);
		return Min + (Max - Min) * Dist(Engine);
	}

	static float GetFraction()
	{
		return FRandRange(0.f, 1.f);
	}

	static FVector GetUnitVector()
	{
		FVector Result;
		float L;
		do
		{
			// Check random vectors in the unit sphere so result is statistically uniform.
			Result.X = GetFraction() * 2.f - 1.f;
			Result.Y = GetFraction() * 2.f - 1.f;
			Result.Z = GetFraction() * 2.f - 1.f;
			L = Result.SizeSquared();
		} while (L > 1.f || L < SMALL_NUMBER);

		return Result.Normalized();
	}
};

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
