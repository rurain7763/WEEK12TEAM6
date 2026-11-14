#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawnModule = true;
}

void UParticleModuleVelocity::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->Velocity = Ctx.Emitter->EmitterToSimulate.TransformVector(StartVelocity);
}
