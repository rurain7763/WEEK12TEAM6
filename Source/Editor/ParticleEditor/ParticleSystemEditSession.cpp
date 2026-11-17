#include "EnginePCH.h"
#include "ParticleSystemEditSession.h"
#include "Component/ParticleSystemComponent.h"
#include "Asset/AssetManager.h"
#include "Core/PathUtils.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleLifeTime.h"
#include "Particle/Modules/ParticleModuleLocation.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Particle/Modules/ParticleModuleSize.h"
#include "Particle/Modules/ParticleModuleColor.h"
#include "Particle/Modules/ParticleModuleRotation.h"
#include "Particle/Modules/ParticleModuleRotationRate.h"
#include "Particle/Modules/ParticleModuleSizeOfLifeTime.h"
#include "Particle/Modules/ParticleModuleColorOverLife.h"
#include "Serialization/JsonArchive.h"
#include "UObject/UObjectIterator.h"
#include "Render/Material.h"

#include <algorithm>
#include <utility>

FParticleSystemEditSession::FParticleSystemEditSession(UParticleSystem* Original)
	: mOriginal{ Original },
	  mWorkingCopy{ CloneParticleSystem(Original) } {
	if (mOriginal && !mWorkingCopy) {
		mError = "Could not create an editable copy of the particle system.";
	}
}

UParticleSystem* FParticleSystemEditSession::GetWorkingCopy() const {
	return mWorkingCopy.get();
}

const FString& FParticleSystemEditSession::GetError() const {
	return mError;
}

FParticleSystemEditSession::FOwnedParticleSystem FParticleSystemEditSession::CloneParticleSystem(UParticleSystem* Source) {
	FOwnedParticleSystem Copy{ nullptr, &DestroyParticleSystem };
	TArray<uint8> Bytes{};
	if (!Source || !FJsonArchive::SaveParticleSystemToBytes(Source, Bytes)) {
		return Copy;
	}

	Copy.reset(NewObject<UParticleSystem>(nullptr, NAME_None, EObjectFlags::RF_Transient));
	if (!FJsonArchive::LoadParticleSystemFromBytes(Copy.get(), Bytes)) {
		Copy.reset();
	}
	return Copy;
}

void FParticleSystemEditSession::DestroyParticleSystem(UParticleSystem* System) {
	if (!System) {
		return;
	}

	for (UParticleEmitter* Emitter : System->Emitters) {
		DestroyEmitter(Emitter);
	}
	delete System;
}

void FParticleSystemEditSession::DestroyEmitter(UParticleEmitter* Emitter) {
	if (!Emitter) {
		return;
	}
	for (UParticleLODLevel* LOD : Emitter->LODLevels) {
		if (!LOD) {
			continue;
		}
		for (UParticleModule* Module : LOD->Modules) {
			delete Module;
		}
		delete LOD->RequiredModule;
		delete LOD->SpawnModule;
		delete LOD;
	}
	delete Emitter;
}

const TArray<UClass*>& FParticleSystemEditSession::GetAddableModuleClasses() {
	static const TArray<UClass*> Classes{ UParticleModuleLifeTime::StaticClass(), UParticleModuleLocation::StaticClass(), UParticleModuleVelocity::StaticClass(), UParticleModuleSize::StaticClass(), UParticleModuleColor::StaticClass(), UParticleModuleRotation::StaticClass(), UParticleModuleRotationRate::StaticClass(), UParticleModuleSizeOfLifeTime::StaticClass(), UParticleModuleColorOverLife::StaticClass() };
	return Classes;
}

UParticleModule* FParticleSystemEditSession::CreateModule(UParticleLODLevel* LOD, UClass* ModuleClass) {
	TUniquePtr<UParticleModule> Module{ NewObject<UParticleModule>(LOD, ModuleClass) };
	LOD->Modules.Add(Module.get());
	return Module.release();
}

UParticleEmitter* FParticleSystemEditSession::AddEmitter(FString& Error) {
	Error.clear();
	if (!mWorkingCopy || !mWorkingCopy->ValidateLODLevels()) {
		Error = "A valid particle system is required to add an emitter.";
		return nullptr;
	}

	FOwnedEmitter Emitter{ NewObject<UParticleEmitter>(mWorkingCopy.get()), &DestroyEmitter };
	for (int32 Index{}; Index < mWorkingCopy->LODDistances.Num(); ++Index) {
		TUniquePtr<UParticleLODLevel> NewLOD{ NewObject<UParticleLODLevel>(Emitter.get()) };
		Emitter->LODLevels.Add(NewLOD.get());
		UParticleLODLevel* LOD{ NewLOD.release() };
		LOD->SetLevel(Index);
		LOD->RequiredModule = NewObject<UParticleModuleRequired>(LOD);
		LOD->RequiredModule->SetMaterial(UAssetManager::GetAssetByPath<UMaterial>("SpriteMaterial"));
		LOD->SpawnModule = NewObject<UParticleModuleSpawn>(LOD);
		UClass* DefaultClasses[]{ UParticleModuleLifeTime::StaticClass(), UParticleModuleLocation::StaticClass(), UParticleModuleVelocity::StaticClass(), UParticleModuleSize::StaticClass(), UParticleModuleColor::StaticClass() };
		for (UClass* ModuleClass : DefaultClasses) {
			CreateModule(LOD, ModuleClass);
		}
	}
	Emitter->CacheEmitterModuleInfo();
	mWorkingCopy->Emitters.Add(Emitter.get());
	return Emitter.release();
}

bool FParticleSystemEditSession::DeleteEmitter(UParticleEmitter* Emitter, FString& Error) {
	Error.clear();
	const int32 Index{ mWorkingCopy && Emitter ? mWorkingCopy->Emitters.Find(Emitter) : -1 };
	if (Index < 0) {
		Error = "The emitter does not belong to this particle system.";
		return false;
	}

	mWorkingCopy->Emitters.RemoveAt(static_cast<uint32>(Index));
	DestroyEmitter(Emitter);
	return true;
}

bool FParticleSystemEditSession::MoveEmitter(UParticleEmitter* Emitter, int32 TargetIndex, FString& Error) {
	Error.clear();
	const int32 SourceIndex{ mWorkingCopy && Emitter ? mWorkingCopy->Emitters.Find(Emitter) : -1 };
	if (SourceIndex < 0 || TargetIndex < 0 || TargetIndex >= mWorkingCopy->Emitters.Num()) {
		Error = "Only emitters in this particle system can be reordered.";
		return false;
	}
	if (SourceIndex == TargetIndex) {
		return false;
	}

	if (SourceIndex < TargetIndex) {
		std::rotate(mWorkingCopy->Emitters.begin() + SourceIndex, mWorkingCopy->Emitters.begin() + SourceIndex + 1, mWorkingCopy->Emitters.begin() + TargetIndex + 1);
	} else {
		std::rotate(mWorkingCopy->Emitters.begin() + TargetIndex, mWorkingCopy->Emitters.begin() + SourceIndex, mWorkingCopy->Emitters.begin() + SourceIndex + 1);
	}
	return true;
}

UParticleLODLevel* FParticleSystemEditSession::GetEditableLOD(UParticleEmitter* Emitter, FString& Error) const {
	if (!mWorkingCopy || !Emitter || mWorkingCopy->Emitters.Find(Emitter) < 0 || !Emitter->GetLODLevel(0)) {
		Error = "Select an emitter in this particle system to edit its modules.";
		return nullptr;
	}
	return Emitter->GetLODLevel(0);
}

UParticleModule* FParticleSystemEditSession::AddModule(UParticleEmitter* Emitter, UClass* ModuleClass, FString& Error) {
	Error.clear();
	UParticleLODLevel* LOD{ GetEditableLOD(Emitter, Error) };
	if (!LOD) {
		return nullptr;
	}
	if (GetAddableModuleClasses().Find(ModuleClass) < 0) {
		Error = "This module type cannot be added to the module stack.";
		return nullptr;
	}

	UParticleModule* Module{ CreateModule(LOD, ModuleClass) };
	LOD->BuildModuleLists();
	return Module;
}

bool FParticleSystemEditSession::DeleteModule(UParticleEmitter* Emitter, UParticleModule* Module, FString& Error) {
	Error.clear();
	UParticleLODLevel* LOD{ GetEditableLOD(Emitter, Error) };
	if (!LOD) {
		return false;
	}
	if (Module && (Module == LOD->RequiredModule || Module == LOD->SpawnModule)) {
		Error = "Required and Spawn modules cannot be deleted.";
		return false;
	}
	const int32 Index{ Module ? LOD->Modules.Find(Module) : -1 };
	if (Index < 0) {
		Error = "The module does not belong to this emitter.";
		return false;
	}

	LOD->Modules.RemoveAt(static_cast<uint32>(Index));
	LOD->BuildModuleLists();
	delete Module;
	return true;
}

bool FParticleSystemEditSession::MoveModule(UParticleEmitter* Emitter, UParticleModule* Module, int32 TargetIndex, FString& Error) {
	Error.clear();
	UParticleLODLevel* LOD{ GetEditableLOD(Emitter, Error) };
	if (!LOD) {
		return false;
	}
	const int32 SourceIndex{ Module ? LOD->Modules.Find(Module) : -1 };
	if (SourceIndex < 0 || TargetIndex < 0 || TargetIndex >= LOD->Modules.Num()) {
		Error = "Only modules in this emitter's module stack can be reordered.";
		return false;
	}
	if (SourceIndex == TargetIndex) {
		return false;
	}

	if (SourceIndex < TargetIndex) {
		std::rotate(LOD->Modules.begin() + SourceIndex, LOD->Modules.begin() + SourceIndex + 1, LOD->Modules.begin() + TargetIndex + 1);
	} else {
		std::rotate(LOD->Modules.begin() + TargetIndex, LOD->Modules.begin() + SourceIndex, LOD->Modules.begin() + SourceIndex + 1);
	}
	LOD->BuildModuleLists();
	return true;
}

bool FParticleSystemEditSession::SetEmitterMaterial(UParticleEmitter* Emitter, UMaterial* Material, FString& Error) {
	Error.clear();
	UParticleLODLevel* LOD{ GetEditableLOD(Emitter, Error) };
	if (!LOD || !LOD->RequiredModule) {
		if (LOD) {
			Error = "The emitter has no Required module to assign a material.";
		}
		return false;
	}
	if (Material && (Material->MaterialDomain != EMaterialDomain::Surface || Material->GetPath().empty() || UAssetManager::GetAssetByPath<UMaterial>(Material->GetPath()) != Material)) {
		Error = "Choose a registered surface material asset for this emitter.";
		return false;
	}
	if (LOD->RequiredModule->GetMaterial() == Material) {
		return false;
	}

	LOD->RequiredModule->SetMaterial(Material);
	return true;
}

void FParticleSystemEditSession::ExchangeParticleSystemData(UParticleSystem* First, UParticleSystem* Second) {
	std::swap(First->Emitters, Second->Emitters);
	std::swap(First->LODMethod, Second->LODMethod);
	std::swap(First->LODDistances, Second->LODDistances);
	std::swap(First->LODDistanceCheckTime, Second->LODDistanceCheckTime);
}

bool FParticleSystemEditSession::Save(FString& Error) {
	Error.clear();
	if (!mOriginal || !mWorkingCopy || mOriginal->GetAssetFilePath().empty()) {
		Error = "The particle system has no editable asset file.";
		return false;
	}

	FOwnedParticleSystem SavedCopy{ CloneParticleSystem(mWorkingCopy.get()) };
	if (!SavedCopy) {
		Error = "Could not prepare the edited particle system for saving.";
		return false;
	}

	const std::filesystem::path Destination{ PathUtils::FromUtf8(mOriginal->GetAssetFilePath()) };
	std::filesystem::path Temporary{ Destination };
	Temporary += L".tmp";
	std::error_code FileError{};
	for (int32 Index{ 1 }; std::filesystem::exists(Temporary, FileError); ++Index) {
		Temporary = Destination.wstring() + L".tmp" + std::to_wstring(Index);
	}
	if (FileError) {
		Error = "Could not prepare the particle system file for saving.";
		return false;
	}
	if (!FJsonArchive::SaveParticleSystem(SavedCopy.get(), PathUtils::ToUtf8(Temporary)) || !MoveFileExW(Temporary.c_str(), Destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		std::filesystem::remove(Temporary, FileError);
		Error = "Could not save the particle system.";
		return false;
	}

	ExchangeParticleSystemData(mOriginal, SavedCopy.get());
	for (TObjectIterator<UParticleSystemComponent> Component{}; Component; ++Component) {
		if (Component->GetTemplate() == mOriginal) {
			Component->ReinitializeEmitterInstances();
		}
	}
	return true;
}
