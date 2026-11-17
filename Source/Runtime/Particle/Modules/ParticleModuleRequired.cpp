#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Render/Material.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleModuleRequired::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("bUseLocalSpace", bUseLocalSpace);
	FArchive& Archive{ Record.GetUnderlyingArchive() };
	UMaterial* BaseMaterial{ Material ? const_cast<UMaterial*>(Material->GetBaseAsset()) : nullptr };
	StructuredArchiveHelpers::SerializeAssetReference(Record, "Material", BaseMaterial);
	if (Archive.IsLoading()) {
		Material = BaseMaterial;
	}

	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("MaterialInstance", Archive.IsSaving() && Material && Material->GetPath().empty()) }) {
		UMaterial* Loaded{ Archive.IsLoading() ? nullptr : Material };
		UMaterial::SerializeMaterialReference(Slot.GetValue(), Loaded);
		if (Archive.IsLoading()) {
			if (Loaded && Loaded->GetPath().empty()) {
				mMaterialInstance.reset(Loaded);
			}
			Material = Loaded;
		}
	}
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
