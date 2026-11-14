#include "EnginePCH.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleLODLevel* UParticleEmitter::GetLODLevel(int32 LODLevel)
{
	return LODLevels.IsValidIndex(LODLevel) ? LODLevels[LODLevel] : nullptr;
}

void UParticleEmitter::CacheEmitterModuleInfo()
{
	ParticleSize = sizeof(FBaseParticle);
}

FParticleEmitterInstance* UParticleEmitter::CreateInstance(UParticleSystemComponent* Owner)
{
	FParticleEmitterInstance* Instance = new FParticleEmitterInstance();
	Instance->InitParameters(this, Owner);
	Instance->Initialize();

	return Instance;
}
