#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleColorOverLife.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleColorOverLife::UParticleModuleColorOverLife()
{
	bUpdateModule = true;
}

void UParticleModuleColorOverLife::Update(const FUpdateContext& Ctx)
{
	for (int32 i = 0; i < Ctx.Emitter->ActiveParticles; i++)
	{
		FBaseParticle* Particle = Ctx.Emitter->GetParticleFromPosition(i);

		FLinearColor Color = FMath::Lerp(StartColor, EndColor, Particle->RelativeTime);

		Particle->Color = Color;
	}
}

void UParticleModuleColorOverLife::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartColor", StartColor);
	Record << SA_VALUE("EndColor", EndColor);
}

void UParticleModuleColorOverLife::SetStartColor(const FLinearColor& Color) {
	StartColor = Color;
}

void UParticleModuleColorOverLife::SetEndColor(const FLinearColor& Color) {
	EndColor = Color;
}
