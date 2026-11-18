#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleSizeScaleBySpeed : public UParticleModule 
{
	DECLARE_CLASS(UParticleModuleSizeScaleBySpeed, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(SpeedScale)
		PROPERTY(MaxScale)
	REFLECT_END()

public:
	UParticleModuleSizeScaleBySpeed();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Update(const FUpdateContext& Context) override;

private:
	FVector SpeedScale = FVector::OneVector;
	FVector MaxScale = FVector::OneVector;
};
