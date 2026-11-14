#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleRotation : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleRotation, UParticleModule)

public:
	UParticleModuleRotation();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	float StartRotation;
};
