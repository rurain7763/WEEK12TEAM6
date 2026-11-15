#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Serialization/MathSerialization.h"

void UParticleModuleSpawn::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("SpawnRate", SpawnRate);
}
