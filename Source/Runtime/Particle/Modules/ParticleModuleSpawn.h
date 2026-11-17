#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSpawn : public UParticleModule {
	DECLARE_CLASS(UParticleModuleSpawn, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(SpawnRate)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void PostEditChangeProperty(const FProperty& Property) override;

	float GetSpawnRate() const;

private:
	float SpawnRate = 100;
};
