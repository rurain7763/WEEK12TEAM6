#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleColor.h"
#include "Serialization/MathSerialization.h"

UParticleModuleColor::UParticleModuleColor()
{
	bSpawnModule = true;
}

void UParticleModuleColor::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartColor", StartColor);
}

void UParticleModuleColor::Spawn(const FSpawnContext& Ctx)
{
	Ctx.Particle->BaseColor = StartColor;
	Ctx.Particle->Color = StartColor;
}
