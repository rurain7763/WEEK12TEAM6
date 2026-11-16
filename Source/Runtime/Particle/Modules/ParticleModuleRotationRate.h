#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleRotationRate : public UParticleModule {
	DECLARE_CLASS(UParticleModuleRotationRate, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartRotationRate)
	REFLECT_END()

public:
	UParticleModuleRotationRate();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

	void SetRotationRate(float RotationRate);

private:
	float StartRotationRate = PI * 0.5f;
};
