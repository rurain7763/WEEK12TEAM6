#include "EnginePCH.h"
#include "Particle/ParticleEmitter.h"
#include <algorithm>
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleLODLevel.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
void UParticleEmitter::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("MaxParticleCount", MaxParticleCount);
	Record << SA_VALUE("ParticleSize", ParticleSize);
	StructuredArchiveHelpers::SerializeArray(Record, "LODLevels", LODLevels);
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
