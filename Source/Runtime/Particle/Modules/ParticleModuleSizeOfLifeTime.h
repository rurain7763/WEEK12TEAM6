#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSizeOfLifeTime : public UParticleModule {
	DECLARE_CLASS(UParticleModuleSizeOfLifeTime, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartScale)
		PROPERTY(EndScale)
	REFLECT_END()

public:
	UParticleModuleSizeOfLifeTime();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Update(const FUpdateContext& Context) override;

private:
	float StartScale = 1.f;
	float EndScale = 2.f;
};
