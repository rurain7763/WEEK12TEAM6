#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRotationRate.h"
#include "Serialization/MathSerialization.h"


UParticleModuleRotationRate::UParticleModuleRotationRate()
{
	bSpawnModule = true;
}

void UParticleModuleRotationRate::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->RotationRate = StartRotationRate;
}

void UParticleModuleRotationRate::SetRotationRate(float InRotationRate)
{
	StartRotationRate = InRotationRate;
}

void UParticleModuleRotationRate::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartRotationRate", StartRotationRate);
}
