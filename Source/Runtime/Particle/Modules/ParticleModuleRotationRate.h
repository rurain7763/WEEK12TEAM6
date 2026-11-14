#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleRotationRate : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleRotationRate, UParticleModule)

public:
	UParticleModuleRotationRate();

	void Spawn(const FSpawnContext& Ctx) override;

	void SetRotationRate(float InRotationRate);

private:
	float StartRotationRate = PI * 0.5f;
};
