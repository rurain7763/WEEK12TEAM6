#include "EnginePCH.h"
#include "ParticleModuleTypeDataBeam.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleBeamEmitterInstance.h"
#include "Serialization/MathSerialization.h"

UParticleModuleTypeDataBeam::UParticleModuleTypeDataBeam()
{
}

void UParticleModuleTypeDataBeam::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("InterpolationPoints", InterpolationPoints);
	Record << SA_VALUE("MaxBeamCount", MaxBeamCount);
	Record << SA_VALUE("bAlwaysOn", bAlwaysOn);
	Record << SA_VALUE("Distance", Distance);
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
	// 1. Source = 이미터 위치 (입자 Location과 같은 simulate 공간)
	Payload->SourcePoint = Emitter->EmitterToSimulate.GetOrigin();
	// 2. 정면 = 이미터 X축
	FVector Forward = Emitter->EmitterToSimulate.TransformVector(FVector(1.f, 0.f, 0.f)).Normalized();
	// 3. Target = Source + 정면 × Distance
	Payload->TargetPoint = Payload->SourcePoint + Forward * Distance;
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

		FVector Delta = Payload->TargetPoint - Payload->SourcePoint;
		for (int32 k = 0;k < Payload->Steps;++k)
		{
			float T = (float)(k + 1) / (float)Payload->Steps;
			Points[k] = Payload->SourcePoint + Delta * T;
		}
	}

}