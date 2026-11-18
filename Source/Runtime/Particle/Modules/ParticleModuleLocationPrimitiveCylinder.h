#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleLocationPrimitiveCylinder : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocationPrimitiveCylinder, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(Center)
		PROPERTY(Radius)
		PROPERTY(HalfHeight)
	REFLECT_END()

public:
	UParticleModuleLocationPrimitiveCylinder();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector Center = FVector::ZeroVector;
	float Radius = 1.f;
	float HalfHeight = 0.5f;
};
