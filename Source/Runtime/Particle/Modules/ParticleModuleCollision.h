#pragma once

#include "Particle/Modules/ParticleModuleCollisionBase.h"
#include "Particle/ParticleTypes.h"

class UParticleModuleCollision : public UParticleModuleCollisionBase
{
	DECLARE_CLASS(UParticleModuleCollision, UParticleModuleCollisionBase)

	REFLECT_START(ThisClass)
		PROPERTY(DampingFactor)
		PROPERTY(RotationDampingFactor)
		PROPERTY(MaxCollisions)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	// 충돌 후 남길 이동·회전 속도의 비율.
	FVector DampingFactor = FVector(0.5f, 0.5f, 0.5f);
	float RotationDampingFactor = 0.5f;
	int32 MaxCollisions = 1;
	EParticleCollisionComplete CompletionOption = EParticleCollisionComplete::Kill;
};
