#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleLocation.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleLocation::UParticleModuleLocation()
{
	bSpawnModule = true;
}

void UParticleModuleLocation::Spawn(const FSpawnContext& Ctx)
{
	FVector LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(StartLocation);
	Ctx.Particle->Location += LocationOffset;
}

void UParticleModuleLocation::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartLocation", StartLocation);
}
