#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleSizeOfLifeTime.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleSizeOfLifeTime::UParticleModuleSizeOfLifeTime()
{
	bUpdateModule = true;
}

void UParticleModuleSizeOfLifeTime::Update(const FUpdateContext& Ctx)
{
	for (int32 i = 0; i < Ctx.Emitter->ActiveParticles; i++)
	{
		FBaseParticle* Particle = Ctx.Emitter->GetParticleFromPosition(i);

		float Scale = FMath::Lerp(StartScale, EndScale, Particle->RelativeTime);

		Particle->Size = Particle->BaseSize * Scale;
	}
}
