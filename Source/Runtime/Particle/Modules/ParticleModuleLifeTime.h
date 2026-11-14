#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleLifeTime : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLifeTime, UParticleModule)

public:
	UParticleModuleLifeTime();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	float LifeTime = 5.f;
};
