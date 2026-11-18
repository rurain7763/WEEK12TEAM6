#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleLocationPrimitiveSphere : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocationPrimitiveSphere, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(Center)
		PROPERTY(OuterRadius)
		PROPERTY(InnerRadius)
	REFLECT_END()

public:
	UParticleModuleLocationPrimitiveSphere();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector Center = FVector::ZeroVector;
	float OuterRadius = 1.f;
	float InnerRadius = 1.f;
};
