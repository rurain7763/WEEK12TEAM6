#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleRotation : public UParticleModule {
	DECLARE_CLASS(UParticleModuleRotation, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartRotation)
	REFLECT_END()

public:
	UParticleModuleRotation();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	float StartRotation{};
};
