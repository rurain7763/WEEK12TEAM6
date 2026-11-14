#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleColor.h"


UParticleModuleColor::UParticleModuleColor()
{
	bSpawnModule = true;
}

void UParticleModuleColor::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->BaseColor = StartColor;
	Ctx.Particle->Color = StartColor;
}
