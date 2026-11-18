#include "EnginePCH.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/Modules/ParticleModuleVelocityCone.h"

UParticleModuleVelocityCone::UParticleModuleVelocityCone()
{
	bSpawnModule = true;
}

void UParticleModuleVelocityCone::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);

	Record << SA_VALUE("Direction", Direction);
	Record << SA_VALUE("InnerAngle", InnerAngle);
	Record << SA_VALUE("OuterAngle", OuterAngle);
	Record << SA_VALUE("Speed", Speed);
}

void UParticleModuleVelocityCone::Spawn(const FSpawnContext& Ctx)
{
	float SafeOuterRadius = FMath::Min(OuterAngle, 90.f);
	float SafeInnerAngle = FMath::Clamp(InnerAngle, 0.f, SafeOuterRadius);

	float InnerRadius = tan(FMath::DegreesToRadians(SafeInnerAngle));
	float OuterRadius = tan(FMath::DegreesToRadians(SafeOuterRadius));

	FVector ForwardAxis, RightAxis, UpAxis;
	FVector::BuildOrthonormalBasis(Direction, ForwardAxis, RightAxis, UpAxis);

	float RndRotation = FRandomStream::FRandRange(0.f, 2.f * PI);
	float RndRadius = FRandomStream::FRandRange(InnerRadius, OuterRadius);

	FVector Result = ForwardAxis + (RightAxis * cos(RndRotation) + UpAxis * sin(RndRotation)) * RndRadius;
	Result = Result.Normalized();

	Ctx.Particle->Velocity = Ctx.Emitter->EmitterToSimulate.TransformVector(Result * Speed);
}
