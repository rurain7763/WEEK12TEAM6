#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleLocationPrimitiveCylinder.h"

UParticleModuleLocationPrimitiveCylinder::UParticleModuleLocationPrimitiveCylinder()
{
	bSpawnModule = true;
}

void UParticleModuleLocationPrimitiveCylinder::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
}

void UParticleModuleLocationPrimitiveCylinder::Spawn(const FSpawnContext& Ctx)
{
	// TODO: Sample the cylinder and apply the spawn location offset.
}
