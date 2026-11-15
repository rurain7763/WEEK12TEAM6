#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSpawn : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleSpawn, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	inline float GetSpawnRate() const
	{
		return SpawnRate;
	}

private:
	float SpawnRate = 10;
};
