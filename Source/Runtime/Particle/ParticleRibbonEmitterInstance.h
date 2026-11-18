#pragma once

#include "Particle/ParticleEmitterInstance.h"

struct FRibbonTypeDataPayload;

struct FParticleRibbonEmitterInstance : public FParticleEmitterInstance
{
	void SpawnParticles(int32 Count, const FVector& InitLocation) override;
	void SpawnParticle(const FVector& InitLocation) override;
	void KillParticle(int32 Position) override;
	void Tick(float DeltaTime) override;
	void BuildRenderData(FParticleEmitterRenderData& RenderData) const override;

	// 한 프레임에 여러 개를 만들 때 이 입자의 위치 (0 = 직전 프레임, 1 = 이번 프레임). AnimTrail 보간·TexU에 쓴다
	float SpawnAlpha = 1.f;

	// AnimTrail: 직전에 만든 입자의 칼날 (월드). 이번 프레임 입자들을 이 값과 현재 칼날 사이에서 보간한다
	bool bHasPrevTrail = false;
	FVector PrevTrailCenter = FVector::ZeroVector;
	FVector PrevTrailDir = FVector::ZeroVector;
	float PrevTrailHalfLength = 0.f;

private:
	FRibbonTypeDataPayload* GetPayload(int32 Slot) const;
	int32 HeadSlot = -1;
	float RibbonTime = 0.f;   // 인스턴스가 만들어진 뒤 누적 시간 (TextureTileTime용 U 시계)
	float LastDeltaTime = 0.f;
};