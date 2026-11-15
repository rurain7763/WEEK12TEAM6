#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleLifeTime : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLifeTime, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	UParticleModuleLifeTime();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	float LifeTime = 5.f;
};
