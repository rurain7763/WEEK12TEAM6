#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"

class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
	DECLARE_CLASS(UParticleModuleTypeDataRibbon, UParticleModuleTypeDataBase)
public:
	FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) override;
	uint32 RequiredBytes() const override;			// sizeof(FRibbonTypeDataPayload)

};
