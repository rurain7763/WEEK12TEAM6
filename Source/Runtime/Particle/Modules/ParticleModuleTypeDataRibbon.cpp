#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleTypeDataRibbon.h"
#include "Particle/ParticleTypes.h"
#include "Particle/ParticleRibbonEmitterInstance.h"

FParticleEmitterInstance* UParticleModuleTypeDataRibbon::CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner)
{
	FParticleEmitterInstance* Instance = new FParticleRibbonEmitterInstance();
	Instance->InitParameters(Emitter, Owner);

	return Instance;
}

void UParticleModuleTypeDataRibbon::Serialize(FStructuredArchive::FRecord Record)
{
	if (TOptional<FStructuredArchive::FSlot> Slot{ Record.TryEnterField("TextureTileTime", true) })
	{
		Slot.GetValue() << TextureTileTime;
	}
}

uint32 UParticleModuleTypeDataRibbon::RequiredBytes() const
{
	return sizeof(FRibbonTypeDataPayload);
}

FRibbonTypeDataPayload* UParticleModuleTypeDataRibbon::GetPayload(FBaseParticle* Particle, int32 Offset) const
{
	// (uint8*) 을 안하면 Particle을 offset만큼 배열로 점프
	return (FRibbonTypeDataPayload*)((uint8*)Particle + Offset);
}

void UParticleModuleTypeDataRibbon::Spawn(const FSpawnContext& Ctx)
{
	// 재사용된 칸에 남은 예전 이웃 번호를 지운다
	*GetPayload(Ctx.Particle, Ctx.Offset) = FRibbonTypeDataPayload{};
}
