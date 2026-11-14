#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleVelocity : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleVelocity, UParticleModule)

public:
	UParticleModuleVelocity();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector StartVelocity = FVector::UpVector;
};
