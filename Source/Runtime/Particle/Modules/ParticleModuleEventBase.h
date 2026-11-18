#pragma once

#include "Particle/ParticleModule.h"

class UParticleModuleEventBase : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleEventBase, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override = 0;
};
