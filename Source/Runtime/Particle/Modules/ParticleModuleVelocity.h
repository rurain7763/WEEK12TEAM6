#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleVelocity : public UParticleModule {
	DECLARE_CLASS(UParticleModuleVelocity, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartVelocity)
		PROPERTY(bHasRandomValue)
		PROPERTY(MinVelocity)
		PROPERTY(MaxVelocity)
	REFLECT_END()

public:
	UParticleModuleVelocity();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	FVector StartVelocity = FVector::UpVector;

	bool bHasRandomValue = true;
	float MinVelocity = 0.1f, MaxVelocity = 1.f;
};
