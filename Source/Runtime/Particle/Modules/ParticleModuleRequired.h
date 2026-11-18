#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"
#include "Render/Material.h"

class UParticleModuleRequired : public UParticleModule {
	DECLARE_CLASS(UParticleModuleRequired, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(bUseLocalSpace)
		PROPERTY(isLit)
		PROPERTY(Material)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	void SetList(bool isLit);

	void SetMaterial(UMaterial* InMaterial);
	UMaterial* GetMaterial();

	void EnableLocalSpace(bool Enable);
	bool GetUseLocalSpace();

private:
	bool bUseLocalSpace = true;
	bool isLit = false;
	UMaterial* Material = nullptr;
};
