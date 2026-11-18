#include "EnginePCH.h"
#include "Particle/ParticleEmitter.h"
#include <algorithm>
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleLODLevel.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Particle/Modules/ParticleModuleCollision.h"

void UParticleEmitter::Serialize(FStructuredArchive::FRecord Record) {
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("bEnabled", true) }) {
		Slot.GetValue() << mEnabled;
	}

	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("EmitterName", true) }) {
		Slot.GetValue() << mEmitterName;
	}

	Record << SA_VALUE("MaxParticleCount", MaxParticleCount);
	Record << SA_VALUE("ParticleSize", ParticleSize);
	StructuredArchiveHelpers::SerializeArray(Record, "LODLevels", LODLevels);
}

const FString& UParticleEmitter::GetEmitterName() const {
	return mEmitterName;
}

void UParticleEmitter::SetEmitterName(const FString& Name) {
	mEmitterName = Name;
}

bool UParticleEmitter::IsEnabled() const {
	return mEnabled;
}

void UParticleEmitter::SetEnabled(bool Enabled) {
	mEnabled = Enabled;
}

UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODLevel)
{
	return LODLevels.IsValidIndex(LODLevel) ? LODLevels[LODLevel] : nullptr;
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
	ParticleSize = sizeof(FBaseParticle);
	TypeDataOffset = 0;
	UParticleLODLevel* LOD = GetLODLevel(0);
	if (LOD && LOD->TypeDataModule)
	{
		TypeDataOffset = ParticleSize;
		ParticleSize += LOD->TypeDataModule->RequiredBytes();
	}

	// 어느 LOD에든 Collision이 있으면 모든 입자에 Collision payload 공간 확보
	CollisionPayloadOffset = -1;
	bool bHasCollision = false;
	for (UParticleLODLevel* LODLevel : LODLevels)
	{
		if (!LODLevel) continue;
		for (UParticleModule* Module : LODLevel->Modules)
		{
			if (Cast<UParticleModuleCollision>(Module))
			{
				bHasCollision = true;
				break;
			}
		}
		if (bHasCollision) break;
	}

	if (bHasCollision)
	{
		constexpr int32 Alignment = alignof(FParticleCollisionPayload);
		ParticleSize = (ParticleSize + Alignment - 1) / Alignment * Alignment;
		CollisionPayloadOffset = ParticleSize;
		ParticleSize += sizeof(FParticleCollisionPayload);
	}
}

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* Owner)
{
	UParticleLODLevel* LOD = GetLODLevel(0);
	FParticleEmitterInstance* Instance;
	if (LOD && LOD->TypeDataModule)
	{
		Instance = LOD->TypeDataModule->CreateInstance(this, Owner);
	}
	else
	{
		Instance= new FParticleEmitterInstance();
		Instance->InitParameters(this, Owner);
	}
	Instance->Initialize();

	return Instance;
}

void UParticleEmitter::PostEditChangeProperty(const FProperty& Property) {
	Super::PostEditChangeProperty(Property);
	MaxParticleCount = std::clamp(MaxParticleCount, 1, 65535);
}
