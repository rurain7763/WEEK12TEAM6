#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

struct FParticleBeamEmitterInstance;

// 빔 시작점. bSpawnModule/bUpdateModule이 false라 일반 모듈 목록에 들어가지 않고, TypeDataBeam이 직접 호출한다
// BoneName이 비어 있으면 Default(이미터 위치/X축), 지정하면 주인 액터 스켈레탈 메시의 그 본 위치, 방향은 부모 본 -> 그 본
class UParticleModuleBeamSource : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleBeamSource, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(BoneName)
		PROPERTY(Strength)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	// simulate 공간의 SourcePoint, SourceTangent(단위벡터), SourceStrength를 채운다
	void ResolveSourceData(const FParticleBeamEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const;

private:
	FString BoneName;      // 예: StaffTip
	float Strength = 0.f;  // 빔 길이 비율. 0 = 직선
};
