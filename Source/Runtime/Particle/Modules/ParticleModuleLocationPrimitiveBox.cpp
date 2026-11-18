#include "EnginePCH.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveBox.h"

UParticleModuleLocationPrimitiveBox::UParticleModuleLocationPrimitiveBox()
{
	bSpawnModule = true;
}

void UParticleModuleLocationPrimitiveBox::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("Center", Center);
	Record << SA_VALUE("HalfExtent", HalfExtent);
}

void UParticleModuleLocationPrimitiveBox::Spawn(const FSpawnContext& Ctx)
{
	FVector MinLocation = Center - HalfExtent;
	FVector MaxLocation = Center + HalfExtent;
	FVector RndLocation = FVector(FRandomStream::FRandRange(MinLocation.X, MaxLocation.X), FRandomStream::FRandRange(MinLocation.Y, MaxLocation.Y), FRandomStream::FRandRange(MinLocation.Z, MaxLocation.Z));
	FVector LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(RndLocation);

	Ctx.Particle->Location += LocationOffset;
}
