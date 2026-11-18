#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleLocationPrimitiveBox : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocationPrimitiveBox, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(Center)
		PROPERTY(HalfExtent)
	REFLECT_END()

public:
	UParticleModuleLocationPrimitiveBox();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector Center;
	FVector HalfExtent;
};
