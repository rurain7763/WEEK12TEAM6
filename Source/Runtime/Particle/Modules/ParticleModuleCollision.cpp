#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleModuleCollision::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("DampingFactor", DampingFactor);
	Record << SA_VALUE("RotationDampingFactor", RotationDampingFactor);
	Record << SA_VALUE("MaxCollisions", MaxCollisions);
	StructuredArchiveHelpers::SerializeEnum(Record, "CompletionOption", CompletionOption, EParticleCollisionComplete::Freeze);
}
