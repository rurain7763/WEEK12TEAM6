#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleLocationBoneSocket.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleBoneLookup.h"
#include "Serialization/MathSerialization.h"

UParticleModuleLocationBoneSocket::UParticleModuleLocationBoneSocket()
{
	bSpawnModule = true;
}

void UParticleModuleLocationBoneSocket::Spawn(const FSpawnContext& Ctx)
{
	FVector BoneWorld;
	FVector ParentWorld;
	if (BoneName.empty() || !FindBoneWorldPoints(Ctx.Emitter->Owner, BoneName, BoneWorld, ParentWorld))
	{
		return;
	}
	// 파티클 위치는 simulate 공간 (bUseLocalSpace면 컴포넌트 로컬, 아니면 월드)
	Ctx.Particle->Location = Ctx.Emitter->SimulateToWorld.Inverse().TransformPosition(BoneWorld);
}

void UParticleModuleLocationBoneSocket::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("BoneName", BoneName);
}
