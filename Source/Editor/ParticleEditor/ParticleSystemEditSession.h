#pragma once

#include "Core/EngineString.h"
#include "Containers/Array.h"
#include <memory>

class UParticleSystem;
class UParticleEmitter;
class UParticleLODLevel;
class UParticleModule;
class UClass;

class FParticleSystemEditSession {
private:
	using FOwnedParticleSystem = std::unique_ptr<UParticleSystem, void (*)(UParticleSystem*)>;
	using FOwnedEmitter = std::unique_ptr<UParticleEmitter, void (*)(UParticleEmitter*)>;

public:
	explicit FParticleSystemEditSession(UParticleSystem* Original);
	~FParticleSystemEditSession() = default;

public:
	UParticleSystem* GetWorkingCopy() const;
	const FString& GetError() const;
	bool Save(FString& Error);

	UParticleEmitter* AddEmitter(FString& Error);
	bool DeleteEmitter(UParticleEmitter* Emitter, FString& Error);
	UParticleModule* AddModule(UParticleEmitter* Emitter, UClass* ModuleClass, FString& Error);
	bool DeleteModule(UParticleEmitter* Emitter, UParticleModule* Module, FString& Error);

	static const TArray<UClass*>& GetAddableModuleClasses();

private:
	static FOwnedParticleSystem CloneParticleSystem(UParticleSystem* Source);
	static void DestroyParticleSystem(UParticleSystem* System);
	static void DestroyEmitter(UParticleEmitter* Emitter);
	static UParticleModule* CreateModule(UParticleLODLevel* LOD, UClass* ModuleClass);
	UParticleLODLevel* GetEditableLOD(UParticleEmitter* Emitter, FString& Error) const;
	static void ExchangeParticleSystemData(UParticleSystem* First, UParticleSystem* Second);

private:
	UParticleSystem* mOriginal{};
	FOwnedParticleSystem mWorkingCopy{ nullptr, &DestroyParticleSystem };
	FString mError{};
};
