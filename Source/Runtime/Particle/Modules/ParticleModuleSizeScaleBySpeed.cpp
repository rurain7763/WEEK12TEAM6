#include "EnginePCH.h"
#include "Particle/ParticleTypes.h"
#include "ParticleModuleSizeScaleBySpeed.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Serialization/MathSerialization.h"

UParticleModuleSizeScaleBySpeed::UParticleModuleSizeScaleBySpeed()
{
	bUpdateModule = true;
}

void UParticleModuleSizeScaleBySpeed::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("SpeedScale", SpeedScale);
	Record << SA_VALUE("MaxScale", MaxScale);
}

void UParticleModuleSizeScaleBySpeed::Update(const FUpdateContext& Context)
{
	for (int32 i = 0; i < Context.Emitter->ActiveParticles; i++)
	{
		FBaseParticle* Particle = Context.Emitter->GetParticleFromPosition(i);

		FVector Size = SpeedScale * Particle->Velocity.Size();
		Size = FVector(Size.MaxComponent());
		Size = FVector(Size.MinComponent());

		Size.X = FMath::Clamp(Size.X, 1.f, MaxScale.X);
		Size.Y = FMath::Clamp(Size.Y, 1.f, MaxScale.Y);
		Size.Z = FMath::Clamp(Size.Z, 1.f, MaxScale.Z);

		Particle->Size = Particle->BaseSize * Size;
	}
}
