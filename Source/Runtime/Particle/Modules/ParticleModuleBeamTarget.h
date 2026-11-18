#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

struct FParticleBeamEmitterInstance;

// 빔 끝점. 컴포넌트에 대상 액터(SetBeamTargetActor)가 있으면 Actor, 없으면 Default(시작점 + 정면 × Distance)
class UParticleModuleBeamTarget : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleBeamTarget, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(TargetOffset)
		PROPERTY(Strength)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	// simulate 공간의 TargetPoint, TargetTangent(단위벡터), TargetStrength를 채운다. Source가 먼저 정해져 있어야 한다
	void ResolveTargetData(const FParticleBeamEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const;

private:
	FVector TargetOffset = FVector(0.f, 0.f, 1.f); // Actor: 액터 원점에서 띄울 높이 (월드)
	float Strength = 0.f;                          // 빔 길이 비율
};
