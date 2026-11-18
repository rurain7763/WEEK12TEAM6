#pragma once

#include "Particle/Modules/ParticleModuleAccelerationBase.h"

class UParticleModuleAccelerationConstant : public UParticleModuleAccelerationBase
{
	DECLARE_CLASS(UParticleModuleAccelerationConstant, UParticleModuleAccelerationBase)

	REFLECT_START(ThisClass)
		PROPERTY(Acceleration)
	REFLECT_END()

public:
	UParticleModuleAccelerationConstant();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Update(const FUpdateContext& Ctx) override;

private:
	FVector Acceleration = FVector(0.f, 0.f, -9.8f);
};
