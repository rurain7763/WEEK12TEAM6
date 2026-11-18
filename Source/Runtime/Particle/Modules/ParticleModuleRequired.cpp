#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Render/Material.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Render/RenderResourceManager.h"
#include "Render/RenderCommand.h"

void UParticleModuleRequired::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("bUseLocalSpace", bUseLocalSpace);
	Record << SA_VALUE("isLit", isLit);

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
			SetMaterial(Loaded);
		}
	}
}

void UParticleModuleRequired::SetList(bool InIsLit) {
	if (isLit == InIsLit) {
		return;
	}

	isLit = InIsLit;
	SetMaterial(Material);
}

void UParticleModuleRequired::SetMaterial(UMaterial* InMaterial) {
	Material = InMaterial && (!InMaterial->GetPath().empty() || !InMaterial->bIsInstance) ? UMaterial::CreateInstance(InMaterial) : InMaterial;
	if (!Material)
	{
		return;
	}

	Material->bUseViewModeShader = false;
	Material->Shader = FRenderResourceManager::GetShaderProgram(isLit ? "TransparentUber_Static_Lit" : "TransparentUber_Static_Unlit");
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
