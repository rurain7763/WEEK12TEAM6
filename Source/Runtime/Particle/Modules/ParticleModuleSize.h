#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSize : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleSize, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	UParticleModuleSize();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector StartSize = FVector::OneVector;
};
