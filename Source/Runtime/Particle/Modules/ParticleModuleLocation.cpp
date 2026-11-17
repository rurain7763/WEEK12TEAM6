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
	FVector LocationOffset = FVector::ZeroVector;
	if (bHasRandomValue)
	{
		FVector RndLocation = FVector(FRandomStream::FRandRange(MinLocation.X, MaxLocation.X), FRandomStream::FRandRange(MinLocation.Y, MaxLocation.Y), FRandomStream::FRandRange(MinLocation.Z, MaxLocation.Z));
		LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(RndLocation);
	}
	else
	{
		LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(StartLocation);
	}

	Ctx.Particle->Location += LocationOffset;
}

void UParticleModuleLocation::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartLocation", StartLocation);
}
