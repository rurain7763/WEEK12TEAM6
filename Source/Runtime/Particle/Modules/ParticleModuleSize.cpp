#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleSize.h"


UParticleModuleSize::UParticleModuleSize()
{
	bSpawnModule = true;
}

void UParticleModuleSize::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->BaseSize = StartSize;
	Ctx.Particle->Size = StartSize;
}
