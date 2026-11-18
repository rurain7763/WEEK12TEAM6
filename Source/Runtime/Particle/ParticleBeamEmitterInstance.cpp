#include "EnginePCH.h"
#include "Particle/ParticleBeamEmitterInstance.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleTypeDataBeam.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Particle/Modules/ParticleModuleRequired.h"

void FParticleBeamEmitterInstance::Initialize()
{
	Super::Initialize();
	BeamTypeData = static_cast<UParticleModuleTypeDataBeam*>(TypeDataModule);

	// UE: MaxBeamCount가 0이면 1로 (ParticleBeam2EmitterInstance.cpp:411)
	BeamCount = BeamTypeData ? FMath::Max(BeamTypeData->GetMaxBeamCount(), 1) : 0;
}

void FParticleBeamEmitterInstance::Tick(float DeltaTime)
{
	// TypeData가 아직 연결 안 됐으면 일반 파티클처럼 동작
	if (!BeamTypeData || !EmitterTemplate || !EmitterTemplate->IsEnabled()) {
		Super::Tick(DeltaTime);
		return;
	}

	// UE Beam2 순서: Kill -> Spawn -> ModuleUpdate -> ModulePostUpdate
	// 생성이 업데이트보다 먼저라서, 새 빔도 같은 프레임에 점이 채워진다
	Tick_KillParticles(DeltaTime);
	UpdateTransforms();
	SpawnBeamParticles(DeltaTime);
	Tick_ModuleUpdate(DeltaTime);
	Tick_ModulePostUpdate(DeltaTime);
	// 빔은 Source·Target으로 위치가 정해지므로 Velocity 이동(Tick_MoveParticles)은 하지 않는다
}

void FParticleBeamEmitterInstance::SpawnBeamParticles(float DeltaTime)
{
	int32 Number = 0;

	if (CurrentLODLevel->SpawnModule)
	{
		SpawnFraction += DeltaTime * CurrentLODLevel->SpawnModule->GetSpawnRate();
		Number = FMath::FloorToInt(SpawnFraction);
		SpawnFraction -= Number;
	}

	// UE: AlwaysOn이면 빔 수를 BeamCount까지 바로 채운다
	if (BeamTypeData->IsAlwaysOn())
	{
		Number = BeamCount;
	}

	// UE: 살아 있는 빔이 없으면 최소 1개는 만든다 ("Force a beam")
	if (ActiveParticles == 0 && Number == 0)
	{
		Number = 1;
	}

	// UE: BeamCount를 넘지 않게 (ParticleBeam2EmitterInstance.cpp:847)
	Number = FMath::Min(Number, BeamCount - ActiveParticles);
	// 배열 크기를 넘지 않게 (UE는 Resize로 늘리지만 우리 엔진은 고정 배열)
	Number = FMath::Min(Number, MaxActiveParticles - ActiveParticles);

	if (Number > 0)
	{
		SpawnParticles(Number, EmitterToSimulate.GetOrigin());
	}
}

void FParticleBeamEmitterInstance::BuildRenderData(FParticleEmitterRenderData& RenderData) const {
	RenderData = FParticleEmitterRenderData{};
	if (!EmitterTemplate || !EmitterTemplate->IsEnabled() || !CurrentLODLevel) {
		return;
	}

	if (!BeamTypeData) 
	{
		Super::BuildRenderData(RenderData);
		return;
	}
	if (!CurrentLODLevel->RequiredModule)
	{
		return;
	}

	RenderData.DynamicEmitterType = EDynamicEmitterType::Beam;
	RenderData.SimulateToWorld = SimulateToWorld;
	RenderData.Material = CurrentLODLevel->RequiredModule->GetMaterial();

	RenderData.Strips.SetNum(ActiveParticles);

	for (int32 i = 0;i < ActiveParticles;++i)
	{
		FBaseParticle* Particle = GetParticleFromPosition(i);
		FBeamTypeDataPayload* Payload = BeamTypeData->GetPayload(Particle, TypeDataOffset);
		FVector* Points = BeamTypeData->GetPoints(Particle, TypeDataOffset);

		FParticleStripRenderData& Strip = RenderData.Strips[i];
		Strip.Points.Add(SimulateToWorld.TransformPosition(Payload->SourcePoint));
		for (int32 k = 0; k < Payload->Steps; k++)
		{
			Strip.Points.Add(SimulateToWorld.TransformPosition(Points[k]));      
		}
		Strip.Width = Particle->Size.X;
		Strip.Color = Particle->Color;

	}



}
