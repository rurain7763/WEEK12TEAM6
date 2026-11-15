#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawnModule = true;
}

void UParticleModuleVelocity::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->Velocity = Ctx.Emitter->EmitterToSimulate.TransformVector(StartVelocity);
}

void UParticleModuleVelocity::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartVelocity", StartVelocity);
}
