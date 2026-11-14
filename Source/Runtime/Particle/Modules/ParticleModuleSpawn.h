#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSpawn : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleSpawn, UParticleModule)

public:
	inline float GetSpawnRate() const
	{
		return SpawnRate;
	}

private:
	float SpawnRate = 100;
};
