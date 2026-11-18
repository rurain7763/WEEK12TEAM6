#include "EnginePCH.h"
#include "Particle/ParticleRibbonEmitterInstance.h"
#include "Particle/ParticleTypes.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleTypeDataAnimTrail.h"
#include "UObject/Casts.h"

void FParticleRibbonEmitterInstance::SpawnParticle(const FVector& InitLocation)
{
	if (ActiveParticles >= MaxActiveParticles)
	{
		return;
	}

	const int32 NewSlot = ParticleIndices[ActiveParticles];
	const int32 CountBefore = ActiveParticles;

	FParticleEmitterInstance::SpawnParticle(InitLocation);
	if (ActiveParticles == CountBefore) // full
	{
		return;
	}

	GetPayload(NewSlot)->NextSlot = HeadSlot;
	if (HeadSlot != -1)
	{
		GetPayload(HeadSlot)->PrevSlot = NewSlot;
	}
	HeadSlot = NewSlot;

}

void FParticleRibbonEmitterInstance::KillParticle(int32 Position)
{
	if (Position < 0 || Position >= ActiveParticles) return;

	const int32 DeadSlot = ParticleIndices[Position];
	FRibbonTypeDataPayload* Dead = GetPayload(DeadSlot);

	
	if (Dead->PrevSlot != -1) 
		GetPayload(Dead->PrevSlot)->NextSlot = Dead->NextSlot;
	else                      
		HeadSlot = Dead->NextSlot;     

	if (Dead->NextSlot != -1) 
		GetPayload(Dead->NextSlot)->PrevSlot = Dead->PrevSlot;

	FParticleEmitterInstance::KillParticle(Position);
}

void FParticleRibbonEmitterInstance::BuildRenderData(FParticleEmitterRenderData& RenderData) const {
	RenderData = FParticleEmitterRenderData{};

	if (!EmitterTemplate || !EmitterTemplate->IsEnabled() || !CurrentLODLevel || !CurrentLODLevel->RequiredModule) {
		return;
	}

	RenderData.DynamicEmitterType = EDynamicEmitterType::Ribbon;
	RenderData.SimulateToWorld = SimulateToWorld;
	RenderData.Material = CurrentLODLevel->RequiredModule->GetMaterial();

	if (HeadSlot == -1)
	{
		return; // 살아 있는 입자가 없다
	}

	// Ribbon은 입자 전체가 띠 1개
	RenderData.Strips.SetNum(1);
	FParticleStripRenderData& Strip = RenderData.Strips[0];

	// AnimTrail이면 점마다 면의 반쪽 벡터도 넘긴다 (payload가 FAnimTrailTypeDataPayload)
	const bool bAnimTrail = Cast<UParticleModuleTypeDataAnimTrail>(TypeDataModule) != nullptr;

	// 머리부터 Next를 따라간다 (출석부 순서가 아니라 태어난 순서)
	int32 Slot = HeadSlot;
	int32 Visited = 0;
	while (Slot != -1 && Visited < ActiveParticles) // 연결이 꼬여도 무한 루프 방지
	{
		// 칸 번호 → 입자 주소 (출석부를 거치지 않는다)
		const FBaseParticle* Particle = (const FBaseParticle*)(ParticleData + Slot * ParticleStride);
		Strip.Points.Add(SimulateToWorld.TransformPosition(Particle->Location));
		if (bAnimTrail)
		{
			const FAnimTrailTypeDataPayload* Trail = static_cast<const FAnimTrailTypeDataPayload*>(GetPayload(Slot));
			Strip.EdgeOffsets.Add(SimulateToWorld.TransformVector(Trail->Direction * Trail->Length));
		}

		Slot = GetPayload(Slot)->NextSlot;
		++Visited;
	}

	// 지금 렌더 구조는 띠 하나에 두께·색이 하나 → 머리 입자 값을 쓴다
	const FBaseParticle* Head = (const FBaseParticle*)(ParticleData + HeadSlot * ParticleStride);
	Strip.Width = Head->Size.X;
	Strip.Color = Head->Color;
}

FRibbonTypeDataPayload* FParticleRibbonEmitterInstance::GetPayload(int32 Slot) const
{
	// Slot = 메모리 칸 번호 (ParticleIndices를 거치지 않는다)
	return (FRibbonTypeDataPayload*)(ParticleData + Slot * ParticleStride + TypeDataOffset);
}
