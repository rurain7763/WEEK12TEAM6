#include "EnginePCH.h"
#include "ParticleSystem.h"
#include "Containers/Set.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include <cmath>

void UParticleSystem::Serialize(FStructuredArchive::FRecord Record) {
	FArchive& Archive{ Record.GetUnderlyingArchive() };
	if (Archive.IsSaving()) {
		EnsureEmitterNames();
	}

	StructuredArchiveHelpers::SerializeArray(Record, "Emitters", Emitters);

	if (Archive.IsLoading()) {
		EnsureEmitterNames();
	}

	// 기존 에셋에 새 필드가 없으면 기본값을 유지한다.
	if (auto Slot = Record.TryEnterField("LODMethod", true))
	{
		int32 Method = static_cast<int32>(LODMethod);
		Slot.GetValue() << Method;
		if (Archive.IsLoading())
		{
			LODMethod = Method == static_cast<int32>(EParticleSystemLODMethod::DirectSet) ?
			EParticleSystemLODMethod::DirectSet : EParticleSystemLODMethod::Automatic;
		}
	}
	if (auto Slot = Record.TryEnterField("LODDistances", true))
	{
		int32 Count = LODDistances.Num();
		auto Distances = Slot->EnterArray(Count);
		if (Archive.IsLoading())
		{
			LODDistances.SetNum(Count);
		}
		for (float& Distance : LODDistances)
		{
			Distances.EnterElement() << Distance;
		}
	}
	if (auto Slot = Record.TryEnterField("LODDistanceCheckTime", true))
	{
		Slot.GetValue() << LODDistanceCheckTime;
	}

	if (Archive.IsLoading())
	{
		if (!std::isfinite(LODDistanceCheckTime) || LODDistanceCheckTime < 0.f)
		{
			LODDistanceCheckTime = 0.25f;
		}
		if (!Archive.IsError() && !ValidateLODLevels())
		{
			Archive.SetError();
		}
	}
}

void UParticleSystem::EnsureEmitterNames() {
	TSet<FString> Names{};
	for (const UParticleEmitter* Emitter : Emitters) {
		if (Emitter && !Emitter->GetEmitterName().empty()) {
			Names.Add(Emitter->GetEmitterName());
		}
	}

	int32 Number{ 1 };
	for (UParticleEmitter* Emitter : Emitters) {
		if (!Emitter || !Emitter->GetEmitterName().empty()) {
			continue;
		}

		FString Name{ "Emitter " + std::to_string(Number) };
		while (Names.Contains(Name)) {
			++Number;
			Name = "Emitter " + std::to_string(Number);
		}

		Emitter->SetEmitterName(Name);
		Names.Add(Name);
		++Number;
	}
}

bool UParticleSystem::ValidateLODLevels() const
{
	const int32 LODCount = LODDistances.Num();
	if (LODCount == 0 || LODDistances[0] != 0.f)
	{
		HTR_LOG(Warning, "Particle LOD: distances must contain LOD 0 starting at distance 0.");
		return false;
	}
	for (int32 Index = 0; Index < LODCount; ++Index)
	{
		if (!std::isfinite(LODDistances[Index]) || LODDistances[Index] < 0.f ||
			(Index > 0 && LODDistances[Index] <= LODDistances[Index - 1]))
		{
			HTR_LOG(Warning, "Particle LOD: invalid distance at index {}; distances must be finite and increasing.", Index);
			return false;
		}
	}

	for (int32 EmitterIndex = 0; EmitterIndex < Emitters.Num(); ++EmitterIndex)
	{
		const UParticleEmitter* Emitter = Emitters[EmitterIndex];
		if (!Emitter)
		{
			HTR_LOG(Warning, "Particle LOD: emitter {} is null.", EmitterIndex);
			return false;
		}
		if (Emitter->LODLevels.Num() != LODCount)
		{
			HTR_LOG(Warning, "Particle LOD: emitter {} has {} levels; expected {}.",
				EmitterIndex, Emitter->LODLevels.Num(), LODCount);
			return false;
		}

		bool bUseLocalSpace = false;
		for (int32 LODIndex = 0; LODIndex < LODCount; ++LODIndex)
		{
			const UParticleLODLevel* LOD = Emitter->LODLevels[LODIndex];
			if (!LOD || LOD->GetLevel() != LODIndex)
			{
				HTR_LOG(Warning, "Particle LOD: emitter {} must have a non-null level numbered {} at that index.",
					EmitterIndex, LODIndex);
				return false;
			}
			if (!LOD->RequiredModule || !LOD->SpawnModule)
			{
				HTR_LOG(Warning, "Particle LOD: emitter {}, level {} requires RequiredModule and SpawnModule.",
					EmitterIndex, LODIndex);
				return false;
			}
			if (LODIndex == 0)
			{
				bUseLocalSpace = LOD->RequiredModule->GetUseLocalSpace();
			}
			else if (LOD->RequiredModule->GetUseLocalSpace() != bUseLocalSpace)
			{
				HTR_LOG(Warning, "Particle LOD: emitter {}, level {} changes simulation space from LOD 0.",
					EmitterIndex, LODIndex);
				return false;
			}
			for (int32 ModuleIndex = 0; ModuleIndex < LOD->Modules.Num(); ++ModuleIndex)
			{
				if (!LOD->Modules[ModuleIndex])
				{
					HTR_LOG(Warning, "Particle LOD: emitter {}, level {}, module {} is null.",
						EmitterIndex, LODIndex, ModuleIndex);
					return false;
				}
			}
		}
	}
	return true;
}
