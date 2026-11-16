#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Render/Material.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleModuleRequired::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("bUseLocalSpace", bUseLocalSpace);
	StructuredArchiveHelpers::SerializeAssetReference(Record, "Material", Material);
}

void UParticleModuleRequired::SetMaterial(UMaterial* InMaterial) {
	Material = InMaterial;
}

UMaterial* UParticleModuleRequired::GetMaterial() {
	return Material;
}

void UParticleModuleRequired::EnableLocalSpace(bool Enable) {
	bUseLocalSpace = Enable;
}

bool UParticleModuleRequired::GetUseLocalSpace() {
	return bUseLocalSpace;
}
