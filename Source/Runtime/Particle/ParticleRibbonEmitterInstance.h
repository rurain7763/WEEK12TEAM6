#pragma once

#include "Particle/ParticleEmitterInstance.h"

struct FRibbonTypeDataPayload;

struct FParticleRibbonEmitterInstance : public FParticleEmitterInstance
{
	void SpawnParticle(const FVector& InitLocation) override;
	void KillParticle(int32 Position) override;
	void BuildRenderData(FParticleEmitterRenderData& RenderData) const override;

private:
	FRibbonTypeDataPayload* GetPayload(int32 Slot) const;
	int32 HeadSlot = -1;
};