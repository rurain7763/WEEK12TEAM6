#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleTypeDataAnimTrail.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleBoneLookup.h"

void UParticleModuleTypeDataAnimTrail::Serialize(FStructuredArchive::FRecord Record)
{
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("FirstSocketName", true) })
	{
		Slot.GetValue() << FirstSocketName;
	}
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("SecondSocketName", true) })
	{
		Slot.GetValue() << SecondSocketName;
	}
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("Width", true) })
	{
		Slot.GetValue() << Width;
	}
}

uint32 UParticleModuleTypeDataAnimTrail::RequiredBytes() const
{
	return sizeof(FAnimTrailTypeDataPayload);
}

FAnimTrailTypeDataPayload* UParticleModuleTypeDataAnimTrail::GetAnimTrailPayload(FBaseParticle* Particle, int32 Offset) const
{
	return (FAnimTrailTypeDataPayload*)((uint8*)Particle + Offset);
}

void UParticleModuleTypeDataAnimTrail::Spawn(const FSpawnContext& Ctx)
{
	FAnimTrailTypeDataPayload* Payload = GetAnimTrailPayload(Ctx.Particle, Ctx.Offset);
	*Payload = FAnimTrailTypeDataPayload{};   // 재사용된 칸의 예전 이웃 번호·면 정보를 지운다

	// UE FParticleAnimTrailEmitterInstance::SpawnParticles: 두 소켓 샘플 -> 방향·길이, 입자는 면의 중앙
	FVector First, Second, Unused;
	if (!FindBoneWorldPoints(Ctx.Emitter->Owner, FirstSocketName, First, Unused)
		|| !FindBoneWorldPoints(Ctx.Emitter->Owner, SecondSocketName, Second, Unused))
	{
		// 스켈레탈 메시가 없을 때 (파티클 에디터 미리보기 등): 이미터 위치에 가짜 칼날을 세운다
		const FVector Center = Ctx.Emitter->SimulateToWorld.TransformPosition(Ctx.Emitter->EmitterToSimulate.GetOrigin());
		First = Center - FVector(0.f, 0.f, 0.5f);
		Second = Center + FVector(0.f, 0.f, 0.5f);
	}

	const FMatrix WorldToSimulate = Ctx.Emitter->SimulateToWorld.Inverse();
	First = WorldToSimulate.TransformPosition(First);
	Second = WorldToSimulate.TransformPosition(Second);

	const FVector Edge = Second - First;
	const float HalfLength = Edge.Size() * 0.5f;
	if (HalfLength < 1e-6f)
	{
		return;
	}
	Payload->Direction = Edge * (0.5f / HalfLength);
	Payload->Length = HalfLength * Width;
	Ctx.Particle->Location = First + Payload->Direction * HalfLength;
}
