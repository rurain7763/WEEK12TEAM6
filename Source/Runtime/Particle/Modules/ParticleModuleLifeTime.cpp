#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleLifeTime.h"


UParticleModuleLifeTime::UParticleModuleLifeTime()
{
	bSpawnModule = true;
}

void UParticleModuleLifeTime::Spawn(const FSpawnContext& Ctx)
{
	if (LifeTime <= 0.f)
	{
		Ctx.Particle->InvMaxLifeTime = 0.f;
	}
	else
	{
		Ctx.Particle->InvMaxLifeTime = 1.f / LifeTime;
	}
}
