#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSize : public UParticleModule {
	DECLARE_CLASS(UParticleModuleSize, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartSize)
	REFLECT_END()

public:
	UParticleModuleSize();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	FVector StartSize = FVector::OneVector;
};
