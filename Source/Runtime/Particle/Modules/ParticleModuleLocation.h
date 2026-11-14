#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"


class UParticleModuleLocation : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocation, UParticleModule)

public:
	UParticleModuleLocation();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector StartLocation = FVector::ZeroVector;

	bool bHasRandomValue = true;
	FVector MinLocation = -FVector::OneVector;
	FVector MaxLocation = FVector::OneVector;
};
