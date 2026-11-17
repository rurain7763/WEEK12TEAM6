#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleLifeTime : public UParticleModule {
	DECLARE_CLASS(UParticleModuleLifeTime, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(LifeTime)
	REFLECT_END()

public:
	UParticleModuleLifeTime();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	float LifeTime = 5.f;
};
