#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleColor : public UParticleModule {
	DECLARE_CLASS(UParticleModuleColor, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartColor)
	REFLECT_END()

public:
	UParticleModuleColor();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Context) override;

private:
	FLinearColor StartColor = FLinearColor::White;
};
