#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRotation.h"
#include "Serialization/MathSerialization.h"


UParticleModuleRotation::UParticleModuleRotation()
{
	bSpawnModule = true;
}

void UParticleModuleRotation::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->Rotation = StartRotation;
}

void UParticleModuleRotation::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartRotation", StartRotation);
}
