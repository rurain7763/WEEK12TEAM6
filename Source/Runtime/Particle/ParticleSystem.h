#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"
#include "Asset/Asset.h"

class UParticleEmitter;

enum class EParticleSystemLODMethod : uint8
{
	Automatic,
	DirectSet
};

class UParticleSystem : public UAsset {
	DECLARE_CLASS(UParticleSystem, UAsset)

public:
	virtual void Serialize(FStructuredArchive::FRecord Record);
	bool ValidateLODLevels() const;

	void EnsureEmitterNames();

public:
	TArray<UParticleEmitter*> Emitters;

	EParticleSystemLODMethod LODMethod = EParticleSystemLODMethod::Automatic;
	// 각 LOD를 선택할 최소 거리. 첫 단계는 0이며 오름차순으로 저장한다.
	TArray<float> LODDistances = { 0.f };
	float LODDistanceCheckTime = 0.25f;
};
