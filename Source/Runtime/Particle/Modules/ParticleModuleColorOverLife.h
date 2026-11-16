#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleColorOverLife : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleColorOverLife, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	UParticleModuleColorOverLife();

	void Update(const FUpdateContext& Ctx) override;

	inline void SetStartColor(const FLinearColor& InColor)
	{
		StartColor = InColor;
	}

	inline void SetEndColor(const FLinearColor& InColor)
	{
		EndColor = InColor;
	}

private:
	FLinearColor StartColor = FLinearColor::White;
	FLinearColor EndColor = FLinearColor::White;
};
