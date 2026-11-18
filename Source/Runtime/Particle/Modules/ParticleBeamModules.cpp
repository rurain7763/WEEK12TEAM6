// 빔 모듈(BeamSource / BeamTarget) 구현. UE처럼 한 파일에 모은다 (UE: Private/Particles/ParticleBeamModules.cpp)
#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleBeamSource.h"
#include "Particle/Modules/ParticleModuleBeamTarget.h"
#include "Particle/Modules/ParticleModuleTypeDataBeam.h"
#include "Particle/ParticleBeamEmitterInstance.h"
#include "Component/ParticleSystemComponent.h"
#include "Component/SkeletalMeshComponent.h"
#include "Animation/Skeleton.h"
#include "GameFramework/Actor.h"
#include "Serialization/MathSerialization.h"

namespace
{
	// 파티클 컴포넌트의 주인 액터 -> 스켈레탈 메시 -> 본 이름으로 노드를 찾아 본과 부모 본의 월드 위치를 구한다
	// 방향은 부모 -> 본으로 잡는다. 본 축 규약이 FBX 변환을 거치며 바뀌어도 영향을 받지 않는다
	bool FindBoneWorldPoints(const UParticleSystemComponent* PSC, const FString& BoneName, FVector& OutBone, FVector& OutParent)
	{
		AActor* Actor = PSC ? PSC->GetOwner() : nullptr;
		const USkeletalMeshComponent* Mesh = Actor ? static_cast<USkeletalMeshComponent*>(Actor->FindComponentByClass(USkeletalMeshComponent::StaticClass())) : nullptr;
		const USkeleton* Skeleton = Mesh ? Mesh->GetSkeleton() : nullptr;
		if (!Skeleton)
		{
			return false;
		}

		const TArray<FModelBone>& Bones = Skeleton->GetBones();
		const TArray<FModelNode>& Nodes = Skeleton->GetNodes();
		for (int32 i = 0; i < Bones.Num(); ++i)
		{
			if (Nodes[Bones[i].NodeIndex].Name == BoneName)
			{
				const FMatrix MeshWorld = Mesh->GetWorldMatrix();
				const int32 Parent = Bones[i].ParentBoneIndex;
				OutBone = (Mesh->GetBoneComponentMatrix(i) * MeshWorld).GetOrigin(); // 행벡터 규약: 로컬 * 부모
				OutParent = Parent >= 0 ? (Mesh->GetBoneComponentMatrix(Parent) * MeshWorld).GetOrigin() : OutBone;
				return true;
			}
		}
		return false;
	}
}

// ---------------- BeamSource ----------------

void UParticleModuleBeamSource::ResolveSourceData(const FParticleBeamEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const
{
	Payload->SourceStrength = Strength;

	FVector BoneWorld, ParentWorld;
	if (!BoneName.empty() && FindBoneWorldPoints(Emitter->Owner, BoneName, BoneWorld, ParentWorld)
		&& (BoneWorld - ParentWorld).SizeSquared() > 1e-8f)
	{
		const FMatrix WorldToSimulate = Emitter->SimulateToWorld.Inverse();
		Payload->SourcePoint = WorldToSimulate.TransformPosition(BoneWorld);
		Payload->SourceTangent = WorldToSimulate.TransformVector(BoneWorld - ParentWorld).Normalized();
		return;
	}

	// Default: 이미터 위치/X축 (본을 못 찾았을 때도)
	Payload->SourcePoint = Emitter->EmitterToSimulate.GetOrigin();
	Payload->SourceTangent = Emitter->EmitterToSimulate.TransformVector(FVector(1.f, 0.f, 0.f)).Normalized();
}

void UParticleModuleBeamSource::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("BoneName", BoneName);
	Record << SA_VALUE("Strength", Strength);
}

// ---------------- BeamTarget ----------------

void UParticleModuleBeamTarget::ResolveTargetData(const FParticleBeamEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const
{
	AActor* TargetActor = Emitter->Owner ? Emitter->Owner->GetBeamTargetActor() : nullptr;

	if (TargetActor)
	{
		// Actor: 대상 위치 + 오프셋
		const FMatrix WorldToSimulate = Emitter->SimulateToWorld.Inverse();
		Payload->TargetPoint = WorldToSimulate.TransformPosition(TargetActor->GetActorLocation() + TargetOffset);
	}
	else
	{
		// Default: 시작점 + 이미터 정면 × Distance
		const FVector Forward = Emitter->EmitterToSimulate.TransformVector(FVector(1.f, 0.f, 0.f)).Normalized();
		Payload->TargetPoint = Payload->SourcePoint + Forward * Emitter->GetBeamTypeData()->GetDistance();
	}

	// 도착 방향 = 시작 핸들(Bezier B1)에서 대상 쪽. 시작에서 휜 흐름을 이어서 들어간다
	const float Len = (Payload->TargetPoint - Payload->SourcePoint).Size();
	const FVector SourceHandle = Payload->SourcePoint + Payload->SourceTangent * (Payload->SourceStrength * Len / 3.f);
	Payload->TargetTangent = (Payload->TargetPoint - SourceHandle).Normalized();
	Payload->TargetStrength = Strength;
}

void UParticleModuleBeamTarget::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("TargetOffset", TargetOffset);
	Record << SA_VALUE("Strength", Strength);
}
