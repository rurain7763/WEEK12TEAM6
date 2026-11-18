#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleLocationPrimitiveCylinder : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocationPrimitiveCylinder, UParticleModule)

	REFLECT_START(ThisClass)
	REFLECT_END()

public:
	UParticleModuleLocationPrimitiveCylinder();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:

};
