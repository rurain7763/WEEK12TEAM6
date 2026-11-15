#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"
#include "Asset/Asset.h"

class UParticleEmitter;

class UParticleSystem : public UAsset
{
	DECLARE_CLASS(UParticleSystem, UAsset)

public:
	virtual void Serialize(FStructuredArchive::FRecord Record);

public:
	TArray<UParticleEmitter*> Emitters;
};
