#include "EnginePCH.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveCylinder.h"

UParticleModuleLocationPrimitiveCylinder::UParticleModuleLocationPrimitiveCylinder()
{
	bSpawnModule = true;
}

void UParticleModuleLocationPrimitiveCylinder::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("Center", Center);
	Record << SA_VALUE("Radius", Radius);
	Record << SA_VALUE("HalfHeight", HalfHeight);
}

void UParticleModuleLocationPrimitiveCylinder::Spawn(const FSpawnContext& Ctx)
{
	float RndRadius = FRandomStream::FRandRange(0.f, Radius);
	FVector RndUnitVector = FRandomStream::GetUnitVector() * RndRadius;
	float RndHeight = FRandomStream::FRandRange(-HalfHeight, HalfHeight);
	FVector RndLocation = Center + FVector(RndUnitVector.X, RndUnitVector.Y, RndHeight);
	FVector LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(RndLocation);

	Ctx.Particle->Location += LocationOffset;
}
