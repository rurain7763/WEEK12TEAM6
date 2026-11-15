#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleSize.h"
#include "Serialization/MathSerialization.h"


UParticleModuleSize::UParticleModuleSize()
{
	bSpawnModule = true;
}

void UParticleModuleSize::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->BaseSize = StartSize;
	Ctx.Particle->Size = StartSize;
}

void UParticleModuleSize::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartSize", StartSize);
}
