#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleLocation : public UParticleModule {
	DECLARE_CLASS(UParticleModuleLocation, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartLocation)
		PROPERTY(bHasRandomValue)
		PROPERTY(MinLocation)
		PROPERTY(MaxLocation)
	REFLECT_END()

public:
	UParticleModuleLocation();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	FVector StartLocation = FVector::ZeroVector;

	bool bHasRandomValue = true;
	FVector MinLocation = -FVector::OneVector;
	FVector MaxLocation = FVector::OneVector;
};
