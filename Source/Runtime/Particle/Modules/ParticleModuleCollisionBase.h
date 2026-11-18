#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleCollisionBase : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleCollisionBase, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override = 0;
};
