#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Serialization/MathSerialization.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Component/ParticleSystemComponent.h"
#include "Engine/World.h"
#include "Collision/HitResult.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Particle/Modules/ParticleModuleTypeDataMesh.h"
#include "Collision/CollisionShape.h"
#include "Collision/PhysicsScene.h"
#include "Math/Quat.h"

namespace
{
	// 접촉면에서 띄우는 거리. 다음 프레임에 표면 안쪽으로 판정되는 것 방지
	constexpr float SurfaceOffset = 0.01f;
}

void UParticleModuleCollision::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("DampingFactor", DampingFactor);
	Record << SA_VALUE("RotationDampingFactor", RotationDampingFactor);
	Record << SA_VALUE("MaxCollisions", MaxCollisions);
	StructuredArchiveHelpers::SerializeEnum(Record, "CompletionOption", CompletionOption, EParticleCollisionComplete::Freeze);
}

bool UParticleModuleCollision::MoveParticle(FParticleEmitterInstance& Owner, FBaseParticle& Particle,
	FParticleCollisionPayload& Payload, float DeltaTime) const
{
	const FVector SimDelta = Particle.Velocity * DeltaTime;

	UParticleSystemComponent* Component = Owner.Owner;
	UWorld* World = Component ? Component->GetWorld() : nullptr;
	if (!World || SimDelta.SizeSquared() < SMALL_NUMBER)
	{
		Particle.Location += SimDelta;
		return false;
	}

	// 충돌 검사는 월드 좌표 기준 (Local Space 입자 포함)
	const FMatrix& SimToWorld = Owner.SimulateToWorld;
	const FVector WorldStart = SimToWorld.TransformPosition(Particle.Location);
	const FVector WorldDelta = SimToWorld.TransformVector(SimDelta);

	// 메시 파티클은 메시 바운드 박스로 Sweep, 그 외에는 점(LineTrace)
	const UParticleModuleTypeDataMesh* MeshType = Owner.CurrentLODLevel	? Cast<UParticleModuleTypeDataMesh>(Owner.CurrentLODLevel->TypeDataModule) : nullptr;
	const UStaticMesh* Mesh = MeshType ? MeshType->GetStaticMesh() : nullptr;

	// 자기 Actor의 충돌체는 제외
	FHitResult Hit;
	FVector CenterOffset = FVector::ZeroVector; // 파티클 원점 → 박스 중심 (월드)
	bool bHit = false;
	if (Mesh)
	{
		// 로컬 바운드 × Size × SimulateToWorld 의 월드 AABB (파티클 회전은 무시, UE4 방식)
		const FMatrix ParticleToWorld = FMatrix::MakeFromTRS(Particle.Location, FQuat::Identity(), Particle.Size) * SimToWorld;
		const FBox WorldBox = Mesh->GetRenderBounds().GetWorldAABB(ParticleToWorld);
		const FVector Center = (WorldBox.Min + WorldBox.Max) * 0.5f;
		const FVector Extent = (WorldBox.Max - WorldBox.Min) * 0.5f;
		CenterOffset = Center - WorldStart;

		bHit = World->GetPhysicsScene().SweepSingleByShape(*World, FCollisionShape::MakeBox(Extent), Center, WorldDelta,
			FRotator(0.f, 0.f, 0.f), Component->GetOwner(), Hit);

		// 시작부터 겹친 경우는 반사하면 표면에 갇히므로 충돌로 보지 않는다
		if (bHit && (Hit.bStartPentrating || Hit.Time <= 0.f))
		{
			bHit = false;
		}
	}
	else
	{
		bHit = World->GetPhysicsScene().LineTraceSingle(WorldStart, WorldDelta, Component->GetOwner(), Hit);
	}

	if (!bHit)
	{
		Particle.Location += SimDelta;
		return false;
	}

	// 표면에서 멀어지는 중이면 충돌로 보지 않음
	const FVector WorldVelocity = SimToWorld.TransformVector(Particle.Velocity);
	const FVector Normal = Hit.Normal;
	const float IntoSurface = FVector::Dot(WorldVelocity, Normal);
	if (IntoSurface >= 0.f)
	{
		Particle.Location += SimDelta;
		return false;
	}

	// 법선 방향 성분 반사 후 감쇠
	const FVector Reflected = WorldVelocity - Normal * (2.f * IntoSurface);
	const FVector NewWorldVelocity = Reflected * DampingFactor;
	Particle.RotationRate *= RotationDampingFactor;

	// 접촉점에서 남은 시간만큼 반사 방향으로 이동 (재검사 없음)
	// Sweep의 Hit.Location은 박스 중심이므로 파티클 원점으로 되돌린다 (LineTrace는 Offset 0)
	const FVector ContactLocation = Hit.Location - CenterOffset + Normal * SurfaceOffset;
	const float RemainingTime = (1.f - Hit.Time) * DeltaTime;

	// 시뮬레이션 좌표로 복귀
	const FMatrix WorldToSim = SimToWorld.Inverse();
	Particle.Location = WorldToSim.TransformPosition(ContactLocation + NewWorldVelocity * RemainingTime);
	Particle.Velocity = WorldToSim.TransformVector(NewWorldVelocity);

	Payload.CollisionCount++;

	// Kill로 슬롯이 재사용되기 전에 충돌 보고
	if (UParticleModuleEventGenerator* Generator = Owner.CurrentLODLevel->EventGeneratorModule)
	{
		Generator->HandleParticleCollision(Owner, Particle, Payload.CollisionCount, MaxCollisions, Hit, WorldVelocity);
	}

	if (Payload.CollisionCount < MaxCollisions)
	{
		return false;
	}

	// MaxCollisions 도달 시 종료 동작
	switch (CompletionOption)
	{
	case EParticleCollisionComplete::Kill:
		return true;
	case EParticleCollisionComplete::Freeze:
		Particle.Location = WorldToSim.TransformPosition(ContactLocation);
		Payload.Flags |= PCF_Freeze;
		return false;
	}
	return false;
}
