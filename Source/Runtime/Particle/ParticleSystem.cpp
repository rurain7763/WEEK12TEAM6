#include "EnginePCH.h"
#include "ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleSystem::Serialize(FStructuredArchive::FRecord Record)
{
	StructuredArchiveHelpers::SerializeArray(Record, "Emitters", Emitters);
}
