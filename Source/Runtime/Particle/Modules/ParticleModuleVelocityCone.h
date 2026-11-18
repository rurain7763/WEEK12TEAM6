#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleVelocityCone : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleVelocityCone, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY_FLAGS(Direction, CPF_None)
		PROPERTY(InnerAngle)
		PROPERTY(OuterAngle)
		PROPERTY(Speed)
	REFLECT_END()

public:
	UParticleModuleVelocityCone();
	const FVector& GetDirection() const { return Direction; }
	void SetDirection(const FVector& Value) { Direction = Value; }

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:
	FVector Direction = FVector::ForwardVector;
	float InnerAngle = 0.f;
	float OuterAngle = 30.f;
	float Speed = 1.f;
};
