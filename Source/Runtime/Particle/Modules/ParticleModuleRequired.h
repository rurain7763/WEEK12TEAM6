#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

class UMaterial;

class UParticleModuleRequired : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleRequired, UParticleModule)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	inline void SetMaterial(UMaterial* InMaterial)
	{
		Material = InMaterial;
	}

	inline UMaterial* GetMaterial()
	{
		return Material;
	}

	inline void EnableLocalSpace(bool bEnable)
	{
		bUseLocalSpace = bEnable;
	}

	inline bool GetUseLocalSpace()
	{
		return bUseLocalSpace;
	}

private:
	bool bUseLocalSpace = true;
	UMaterial* Material = nullptr;
};
