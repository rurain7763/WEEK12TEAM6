#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleRotationRate : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleRotationRate, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	UParticleModuleRotationRate();

	void Spawn(const FSpawnContext& Ctx) override;

	void SetRotationRate(float InRotationRate);

private:
	float StartRotationRate = PI * 0.5f;
};
