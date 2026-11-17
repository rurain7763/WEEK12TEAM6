#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"

struct FRibbonTypeDataPayload;

class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
	DECLARE_CLASS(UParticleModuleTypeDataRibbon, UParticleModuleTypeDataBase)
public:
	// 저장할 값이 없다. 재정의하지 않으면 UObject::Serialize가 없는 "Properties" 필드를 읽다가 예외를 던진다
	void Serialize(FStructuredArchive::FRecord Record) override {}
	FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) override;
	uint32 RequiredBytes() const override;			// sizeof(FRibbonTypeDataPayload)
	FRibbonTypeDataPayload* GetPayload(FBaseParticle* Particle, int32 Offset) const;
	void Spawn(const FSpawnContext& Ctx) override;
};
