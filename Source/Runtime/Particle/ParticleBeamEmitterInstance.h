#pragma once
#include "Particle/ParticleEmitterInstance.h"

class UParticleModuleTypeDataBeam;

struct FParticleBeamEmitterInstance : public FParticleEmitterInstance
{
	using Super = FParticleEmitterInstance;

	void Initialize() override;
	void Tick(float DeltaTime) override;
	void BuildRenderData(FParticleEmitterRenderData& RenderData) const override;

private:
	// UE FParticleBeam2EmitterInstance::SpawnBeamParticles 대응. 빔 개수 기준으로만 생성
	void SpawnBeamParticles(float DeltaTime);

	UParticleModuleTypeDataBeam* BeamTypeData = nullptr;
	int32 BeamCount = 0; // 동시에 존재할 빔 수 (UE: BeamCount = MaxBeamCount)
};
