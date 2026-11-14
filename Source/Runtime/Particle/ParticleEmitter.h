#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"

class UParticleLODLevel;
class UParticleSystemComponent;
struct FParticleEmitterInstance;

class UParticleEmitter : public UObject
{
	DECLARE_CLASS(UParticleEmitter, UObject)

public:
	FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* Owner);

	UParticleLODLevel* GetLODLevel(int32 LODLevel);

	void CacheEmitterModuleInfo();

public:
	TArray<UParticleLODLevel*> LODLevels;

private:
	friend class FParticleEmitterInstance;

	int32 MaxParticleCount = 1000;
	int32 ParticleSize = 0;
};
