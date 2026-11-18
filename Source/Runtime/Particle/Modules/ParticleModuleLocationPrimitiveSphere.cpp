#include "EnginePCH.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveSphere.h"

UParticleModuleLocationPrimitiveSphere::UParticleModuleLocationPrimitiveSphere()
{
	bSpawnModule = true;
}

void UParticleModuleLocationPrimitiveSphere::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("Center", Center);
	Record << SA_VALUE("OuterRadius", OuterRadius);
	Record << SA_VALUE("InnerRadius", InnerRadius);
}

void UParticleModuleLocationPrimitiveSphere::Spawn(const FSpawnContext& Ctx)
{
	float SafeOuterRadius = FMath::Max(0.f, OuterRadius);
	float SafeInnerRadius = FMath::Clamp(InnerRadius, 0.f, SafeOuterRadius);

	float RndRadius = FRandomStream::FRandRange(SafeInnerRadius, SafeOuterRadius);
	FVector Dir = FRandomStream::GetUnitVector() * RndRadius;
	FVector RndLocation = Center + Dir;
	FVector LocationOffset = Ctx.Emitter->EmitterToSimulate.TransformVector(RndLocation);

	Ctx.Particle->Location += LocationOffset;
}
