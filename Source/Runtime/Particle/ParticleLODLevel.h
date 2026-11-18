#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"

class UParticleModule;
class UParticleModuleSpawn;
class UParticleModuleRequired;
class UParticleModuleTypeDataBase;
class UParticleModuleCollision;
class UParticleModuleEventGenerator;
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
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;

	// 이 LOD의 Collision 모듈 (없으면 nullptr). BuildModuleLists에서 갱신
	UParticleModuleCollision* CollisionModule = nullptr;

	// 이 LOD의 EventGenerator 모듈 (없으면 nullptr). BuildModuleLists에서 갱신
	UParticleModuleEventGenerator* EventGeneratorModule = nullptr;

private:
	friend struct FParticleEmitterInstance;

	int32 Level = 0;
	bool bEnable = true;

	TArray<UParticleModule*> SpawnModules;
	TArray<UParticleModule*> UpdateModules;
};
