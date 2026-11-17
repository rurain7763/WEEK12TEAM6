#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleTypeDataRibbon.h"

// 링크 에러를 막는 임시 정의. Ribbon 구현 때 채운다
FParticleEmitterInstance* UParticleModuleTypeDataRibbon::CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner)
{
	return nullptr;
}

uint32 UParticleModuleTypeDataRibbon::RequiredBytes() const
{
	return 0;
}
