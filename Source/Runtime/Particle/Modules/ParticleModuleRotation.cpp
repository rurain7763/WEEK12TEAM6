#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRotation.h"


UParticleModuleRotation::UParticleModuleRotation()
{
	bSpawnModule = true;
}

void UParticleModuleRotation::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->Rotation = StartRotation;
}
