#pragma once

#include "Particle/Modules/ParticleModuleTypeDataRibbon.h"

struct FAnimTrailTypeDataPayload;

// UE: UParticleModuleTypeDataAnimTrail. 리본과 같은 연결·렌더 경로를 쓰고, 입자마다 두 소켓(본) 사이의 면을 저장한다
// 입자 위치 = 두 본의 중앙, 폭 = 두 본을 잇는 방향 x 거리 (UE ETrailWidthMode_FromCentre)
// UE는 소켓 이름을 AnimNotifyState가 BeginTrails로 넘기지만, 여기서는 TypeData에 둔다
class UParticleModuleTypeDataAnimTrail : public UParticleModuleTypeDataRibbon
{
	DECLARE_CLASS(UParticleModuleTypeDataAnimTrail, UParticleModuleTypeDataRibbon)

	REFLECT_START(ThisClass)
		PROPERTY(FirstSocketName)
		PROPERTY(SecondSocketName)
		PROPERTY(Width)
	REFLECT_END()

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	uint32 RequiredBytes() const override;   // sizeof(FAnimTrailTypeDataPayload)
	void Spawn(const FSpawnContext& Ctx) override;

	FAnimTrailTypeDataPayload* GetAnimTrailPayload(FBaseParticle* Particle, int32 Offset) const;

private:
	FString FirstSocketName;    // 예: Weapon_R (손잡이)
	FString SecondSocketName;   // 예: BladeTip (칼끝)
	float Width = 1.f;          // 1 = 두 본 사이 전체
};
