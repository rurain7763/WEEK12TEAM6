#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSizeOfLifeTime : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleSizeOfLifeTime, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	UParticleModuleSizeOfLifeTime();

	void Update(const FUpdateContext& Ctx) override;

private:
	float StartScale = 1.f;
	float EndScale = 2.f;
};
