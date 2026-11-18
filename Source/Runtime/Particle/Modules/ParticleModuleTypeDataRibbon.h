#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"

struct FRibbonTypeDataPayload;

class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
	DECLARE_CLASS(UParticleModuleTypeDataRibbon, UParticleModuleTypeDataBase)

	REFLECT_START(ThisClass)
		PROPERTY(TextureTileTime)
	REFLECT_END()

public:
	// 재정의하지 않으면 UObject::Serialize가 없는 "Properties" 필드를 읽다가 예외를 던진다
	void Serialize(FStructuredArchive::FRecord Record) override;
	FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) override;
	uint32 RequiredBytes() const override;			// sizeof(FRibbonTypeDataPayload)
	FRibbonTypeDataPayload* GetPayload(FBaseParticle* Particle, int32 Offset) const;
	void Spawn(const FSpawnContext& Ctx) override;

	float GetTextureTileTime() const { return TextureTileTime; }

private:
	// 텍스처 한 장이 지나가는 시간(초). U를 입자가 태어난 시각으로 고정해 프레임마다 이어 붙인다 (SamplerState LinearWrap 필요)
	// 0이면 띠 전체에 텍스처 한 장을 늘린다
	float TextureTileTime = 0.f;
};
