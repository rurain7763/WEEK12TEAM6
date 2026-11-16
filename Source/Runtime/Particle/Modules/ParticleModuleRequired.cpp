#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleModuleRequired::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("bUseLocalSpace", bUseLocalSpace);
	StructuredArchiveHelpers::SerializeAssetReference(Record, "Material", Material);
}
