#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleTypeDataAnimTrail.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleRibbonEmitterInstance.h"
#include "Particle/ParticleBoneLookup.h"

void UParticleModuleTypeDataAnimTrail::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
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

	// 한 프레임에 여러 입자를 만들면 직전 칼날 -> 현재 칼날 사이를 보간해 칸을 촘촘하게 채운다
	// 끝점을 그대로 lerp하면 회전 중간에 칼날이 짧아지므로 중심·방향·길이를 따로 보간한다
	// AnimTrail은 Ribbon의 CreateInstance를 물려받으므로 인스턴스는 항상 FParticleRibbonEmitterInstance
	FParticleRibbonEmitterInstance* Ribbon = static_cast<FParticleRibbonEmitterInstance*>(Ctx.Emitter);
	FVector Center = (First + Second) * 0.5f;
	FVector Dir = Second - First;
	float Half = Dir.Size() * 0.5f;
	if (Half > 1e-6f)
	{
		Dir = Dir * (0.5f / Half);
		if (Ribbon->bHasPrevTrail && Ribbon->SpawnAlpha < 1.f)
		{
			const float A = Ribbon->SpawnAlpha;
			const FVector LerpDir = Ribbon->PrevTrailDir + (Dir - Ribbon->PrevTrailDir) * A;
			if (LerpDir.SizeSquared() > 1e-8f) // 정반대로 뒤집힌 경우는 현재 칼날을 그대로 쓴다
			{
				Center = Ribbon->PrevTrailCenter + (Center - Ribbon->PrevTrailCenter) * A;
				Half = Ribbon->PrevTrailHalfLength + (Half - Ribbon->PrevTrailHalfLength) * A;
				Dir = LerpDir.Normalized();
				First = Center - Dir * Half;
				Second = Center + Dir * Half;
			}
		}
		else if (Ribbon->SpawnAlpha >= 1.f)
		{
			// 이번 프레임의 마지막 입자 = 실제 칼날. 다음 프레임 보간의 시작점이 된다
			Ribbon->bHasPrevTrail = true;
			Ribbon->PrevTrailCenter = Center;
			Ribbon->PrevTrailDir = Dir;
			Ribbon->PrevTrailHalfLength = Half;
		}
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
