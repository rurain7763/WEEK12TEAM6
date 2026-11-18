#pragma once
#include "Particle/ParticleEmitterInstance.h"

class UParticleModuleTypeDataBeam;
class UParticleModuleBeamSource;
class UParticleModuleBeamTarget;
class UParticleLODLevel;

struct FParticleBeamEmitterInstance : public FParticleEmitterInstance
{
	using Super = FParticleEmitterInstance;

	void Initialize() override;
	void Tick(float DeltaTime) override;
	void BuildRenderData(FParticleEmitterRenderData& RenderData) const override;

	// UE: BeamModule_Source / BeamModule_Target. 일반 모듈 목록에 없고 TypeData가 끝점을 정할 때 직접 호출한다
	UParticleModuleBeamSource* BeamModule_Source = nullptr;
	UParticleModuleBeamTarget* BeamModule_Target = nullptr;

	UParticleModuleTypeDataBeam* GetBeamTypeData() const { return BeamTypeData; }

private:
	// UE FParticleBeam2EmitterInstance::SpawnBeamParticles 대응. 빔 개수 기준으로만 생성
	void SpawnBeamParticles(float DeltaTime);
	// UE: UParticleModuleTypeDataBeam2::CacheModuleInfo 대응. 현재 LOD의 빔 모듈을 찾아 둔다
	void CacheBeamModules();

	UParticleModuleTypeDataBeam* BeamTypeData = nullptr;
	int32 BeamCount = 0; // 동시에 존재할 빔 수 (UE: BeamCount = MaxBeamCount)
	UParticleLODLevel* CachedBeamLOD = nullptr; // LOD가 바뀌면 모듈을 다시 찾는다
};
