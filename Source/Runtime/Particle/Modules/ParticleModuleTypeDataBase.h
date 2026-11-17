#pragma once

#include "Particle/ParticleModule.h"

struct FParticleEmitterInstance;
class UParticleEmitter;
class UParticleSystemComponent;

class UParticleModuleTypeDataBase : public UParticleModule
{
	DECLARE_CLASS(UParticleModuleTypeDataBase, UParticleModule)
public:
	virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter,UParticleSystemComponent* Owner) { return nullptr; }

	virtual void CacheModuleInfo(UParticleEmitter* Emitter) {} 
};