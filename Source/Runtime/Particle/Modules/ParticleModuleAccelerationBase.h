#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleAccelerationBase : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleAccelerationBase, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(bAlwaysInWorldSpace)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Update(const FUpdateContext& Ctx) override = 0;

protected:
	bool bAlwaysInWorldSpace = true;
};
