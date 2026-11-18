#include "EnginePCH.h"
#include "ParticleModuleTypeDataBeam.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleBeamEmitterInstance.h"
#include "Particle/Modules/ParticleModuleBeamSource.h"
#include "Particle/Modules/ParticleModuleBeamTarget.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleModule.h" // FRandomStream
#include <cmath>

#include <algorithm>
#include <cmath>

UParticleModuleTypeDataBeam::UParticleModuleTypeDataBeam()
{
}

void UParticleModuleTypeDataBeam::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("InterpolationPoints", InterpolationPoints);
	Record << SA_VALUE("MaxBeamCount", MaxBeamCount);
	Record << SA_VALUE("bAlwaysOn", bAlwaysOn);
	Record << SA_VALUE("Distance", Distance);
	// 기존 에셋엔 없으므로 선택적으로 읽는다
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("NoiseRange", true) })
	{
		Slot.GetValue() << NoiseRange;
	}
}

void UParticleModuleTypeDataBeam::PostEditChangeProperty(const FProperty& Property) {
	Super::PostEditChangeProperty(Property);
	InterpolationPoints = std::clamp(InterpolationPoints, 1, 1024);
	MaxBeamCount = std::clamp(MaxBeamCount, 1, 65535);
	Distance = std::isfinite(Distance) ? std::max(0.0f, Distance) : 0.0f;
}

uint32 UParticleModuleTypeDataBeam::RequiredBytes() const
{
	return sizeof(FBeamTypeDataPayload) + sizeof(FVector) * InterpolationPoints;
}

FParticleEmitterInstance* UParticleModuleTypeDataBeam::CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner)
{
	FParticleEmitterInstance* Instance = new FParticleBeamEmitterInstance();
	Instance->InitParameters(Emitter, Owner);

	return Instance;
}

FBeamTypeDataPayload* UParticleModuleTypeDataBeam::GetPayload(FBaseParticle* Particle, int32 Offset) const
{
	// (uint8*) 을 안하면 Particle을 offset만큼 배열로 점프
	return (FBeamTypeDataPayload*)((uint8*)Particle + Offset);
}

FVector* UParticleModuleTypeDataBeam::GetPoints(FBaseParticle* Particle, int32 Offset) const
{
	return (FVector*)((uint8*)Particle + Offset + sizeof(FBeamTypeDataPayload));
}

void UParticleModuleTypeDataBeam::ResolveEndPoints(const FParticleEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const
{
	const FParticleBeamEmitterInstance* BeamInst = static_cast<const FParticleBeamEmitterInstance*>(Emitter);
	const FVector Forward = Emitter->EmitterToSimulate.TransformVector(FVector(1.f, 0.f, 0.f)).Normalized();

	// 1. Source (UE: BeamModule_Source가 없으면 이미터 위치, ParticleBeamModules.cpp:129)
	if (BeamInst->BeamModule_Source)
	{
		BeamInst->BeamModule_Source->ResolveSourceData(BeamInst, Payload);
	}
	else
	{
		Payload->SourcePoint = Emitter->EmitterToSimulate.GetOrigin();
		Payload->SourceTangent = Forward;
		Payload->SourceStrength = 0.f;
	}

	// 2. Target. Source가 먼저 정해져야 도착 방향을 계산할 수 있다 (UE: ParticleBeamModules.cpp:137)
	if (BeamInst->BeamModule_Target)
	{
		BeamInst->BeamModule_Target->ResolveTargetData(BeamInst, Payload);
	}
	else
	{
		Payload->TargetPoint = Payload->SourcePoint + Forward * Distance;
		Payload->TargetTangent = Forward;
		Payload->TargetStrength = 0.f;
	}
}

void UParticleModuleTypeDataBeam::Spawn(const FSpawnContext& Ctx)
{
	FBeamTypeDataPayload* Payload = GetPayload(Ctx.Particle, Ctx.Offset);
	*Payload = FBeamTypeDataPayload{}; // payload부분까지 초기화
	
	//양끝점 Init
	ResolveEndPoints(Ctx.Emitter, Payload);
	
	Payload->Steps = InterpolationPoints;
	Payload->TriangleCount = InterpolationPoints * 2;

}
void UParticleModuleTypeDataBeam::Update(const FUpdateContext& Ctx)
{
	/*
	| Particle | Payload | Points |
	*/
	for (int32 i = 0;i < Ctx.Emitter->ActiveParticles; ++i)
	{
		FBaseParticle* Particle = Ctx.Emitter->GetParticleFromPosition(i);
		FBeamTypeDataPayload* Payload = GetPayload(Particle, Ctx.Offset);
		FVector* Points = GetPoints(Particle, Ctx.Offset);

		ResolveEndPoints(Ctx.Emitter, Payload);

		const FVector P0 = Payload->SourcePoint;
		const FVector P1 = Payload->TargetPoint;
		const FVector Delta = P1 - P0;

		// 세기 0 = 기존 직선 (점 간격 균일 유지)
		if (Payload->SourceStrength == 0.f && Payload->TargetStrength == 0.f)
		{
			for (int32 k = 0;k < Payload->Steps;++k)
			{
				const float T = (float)(k + 1) / (float)Payload->Steps;
				Points[k] = P0 + Delta * T;
			}
		}
		else
		{
			// Hermite 곡선: 세기는 빔 길이에 대한 비율
			const float Len = Delta.Size();
			const FVector M0 = Payload->SourceTangent * (Payload->SourceStrength * Len);
			const FVector M1 = Payload->TargetTangent * (Payload->TargetStrength * Len);

			for (int32 k = 0;k < Payload->Steps;++k)
			{
				const float T = (float)(k + 1) / (float)Payload->Steps;
				const float T2 = T * T;
				const float T3 = T2 * T;

				const float H00 = 2.f * T3 - 3.f * T2 + 1.f;  // P0 가중치
				const float H10 = T3 - 2.f * T2 + T;          // M0 가중치
				const float H01 = -2.f * T3 + 3.f * T2;       // P1 가중치
				const float H11 = T3 - T2;                    // M1 가중치

				Points[k] = P0 * H00 + M0 * H10 + P1 * H01 + M1 * H11;
			}
		}

		// 노이즈: 매 프레임 빔 축에 수직인 무작위 방향으로 흔든다. sin(πT)로 양 끝은 0 (지팡이·대상에서 안 떨어짐)
		if (NoiseRange > 0.f)
		{
			const FVector Axis = Delta.Normalized();
			for (int32 k = 0;k < Payload->Steps - 1;++k)   // 마지막 점(대상)은 제외
			{
				const float T = (float)(k + 1) / (float)Payload->Steps;
				FVector Random = FRandomStream::GetUnitVector();
				Random = Random - Axis * (Random | Axis);   // 축 방향 성분 제거 = 수직 방향만
				Points[k] = Points[k] + Random * (NoiseRange * std::sin(PI * T));
			}
		}
	}

}