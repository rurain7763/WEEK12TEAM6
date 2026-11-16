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
	REFLECT_START(ThisClass)
		PROPERTY(MaxParticleCount)
		PROPERTY_FLAGS(ParticleSize, CPF_VisibleAnywhere)
	REFLECT_END()

public:
	void PostEditChangeProperty(const FProperty& Property) override;

	virtual void Serialize(FStructuredArchive::FRecord Record) override;

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
