#pragma once

#include "UObject/Class.h"
#include "UObject/Object.h"

class UParticleEmitter;

class UParticleSystem : public UObject
{
	DECLARE_CLASS(UParticleSystem, UObject)

public:
	TArray<UParticleEmitter*> Emitters;
};
