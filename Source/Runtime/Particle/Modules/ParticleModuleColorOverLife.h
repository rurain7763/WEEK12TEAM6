#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleColorOverLife : public UParticleModule {
	DECLARE_CLASS(UParticleModuleColorOverLife, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(StartColor)
		PROPERTY(EndColor)
	REFLECT_END()

public:
	UParticleModuleColorOverLife();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	void Update(const FUpdateContext& Context) override;

	void SetStartColor(const FLinearColor& Color);
	void SetEndColor(const FLinearColor& Color);

private:
	FLinearColor StartColor = FLinearColor::White;
	FLinearColor EndColor = FLinearColor::White;
};
