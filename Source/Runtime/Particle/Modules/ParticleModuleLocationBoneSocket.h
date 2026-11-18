#pragma once

#include "Particle/ParticleModule.h"
#include "Particle/ParticleTypes.h"

// UE: UParticleModuleLocationBoneSocket. 새 파티클을 주인 액터 스켈레탈 메시의 BoneName 본 위치에서 태어나게 한다
// 리본에 붙이면 그 본(예: 칼끝)이 지나간 자리가 띠가 된다. 본을 못 찾으면 원래 위치(이미터)를 그대로 둔다
class UParticleModuleLocationBoneSocket : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleLocationBoneSocket, UParticleModule)

	REFLECT_START(ThisClass)
		PROPERTY(BoneName)
	REFLECT_END()

public:
	UParticleModuleLocationBoneSocket();

	void Serialize(FStructuredArchive::FRecord Record) override;
	void Spawn(const FSpawnContext& Ctx) override;

private:
	FString BoneName;   // 예: BladeTip
};
