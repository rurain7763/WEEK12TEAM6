#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"

class UParticleModule;
class UParticleModuleSpawn;
class UParticleModuleRequired;
struct FParticleEmitterInstance;

class UParticleLODLevel : public UObject
{
	DECLARE_CLASS(UParticleLODLevel, UObject)

public:
	virtual void Serialize(FStructuredArchive::FRecord Record);

	void BuildModuleLists();
	void SetLevel(int32 InLevel);

	int32 GetLevel() const { return Level; }
	bool IsEnabled() const { return bEnable; }

public:
	TArray<UParticleModule*> Modules;
	UParticleModuleSpawn* SpawnModule = nullptr;
	UParticleModuleRequired* RequiredModule = nullptr;

private:
	friend struct FParticleEmitterInstance;

	int32 Level = 0;
	bool bEnable = true;

	TArray<UParticleModule*> SpawnModules;
	TArray<UParticleModule*> UpdateModules;
};
