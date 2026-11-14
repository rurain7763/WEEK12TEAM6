#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleColor : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleColor, UParticleModule)

public:
	UParticleModuleColor();

	void Spawn(const FSpawnContext& Ctx) override;

private:
	FLinearColor StartColor = FLinearColor::White;
};
