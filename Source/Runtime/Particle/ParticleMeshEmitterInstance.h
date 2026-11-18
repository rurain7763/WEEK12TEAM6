#pragma once

#include "Particle/ParticleEmitterInstance.h"

struct FParticleMeshEmitterInstance : public FParticleEmitterInstance {
public:
	void BuildRenderData(FParticleEmitterRenderData& RenderData) const override;
};
