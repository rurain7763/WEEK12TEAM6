#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleVelocity.h"
#include "Serialization/MathSerialization.h"
#include "Particle/ParticleEmitterInstance.h"


UParticleModuleVelocity::UParticleModuleVelocity()
{
	bSpawnModule = true;
}

void UParticleModuleVelocity::Spawn(const FSpawnContext& Ctx)
{
	FVector Velocity;
	if (bHasRandomValue)
	{
		Velocity = FRandomStream::GetUnitVector() * FRandomStream::FRandRange(MinVelocity, MaxVelocity);
	}
	else
	{
		Velocity = StartVelocity;
	}

	Ctx.Particle->Velocity = Ctx.Emitter->EmitterToSimulate.TransformVector(Velocity);
}

void UParticleModuleVelocity::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("StartVelocity", StartVelocity);

	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("bHasRandomValue", true) }) {
		Slot.GetValue() << bHasRandomValue;
	}

	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("MinVelocity", true) }) {
		Slot.GetValue() << MinVelocity;
	}

	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("MaxVelocity", true) }) {
		Slot.GetValue() << MaxVelocity;
	}
}
