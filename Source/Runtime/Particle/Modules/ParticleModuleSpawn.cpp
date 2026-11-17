#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Serialization/MathSerialization.h"

void UParticleModuleSpawn::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("SpawnRate", SpawnRate);
}

void UParticleModuleSpawn::PostEditChangeProperty(const FProperty& Property) {
	Super::PostEditChangeProperty(Property);
	SpawnRate = FMath::Max(0.0f, SpawnRate);
}

float UParticleModuleSpawn::GetSpawnRate() const {
	return SpawnRate;
}
