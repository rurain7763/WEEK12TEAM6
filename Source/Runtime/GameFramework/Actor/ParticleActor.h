#pragma once

#include "GameFramework/Actor.h"

class UParticleSystemComponent;

// Hosts a particle system component configured with an asset.
class AParticleActor : public AActor
{
	DECLARE_CLASS(AParticleActor, AActor)

	REFLECT_START(ClassName)
	REFLECT_END()

public:
	AParticleActor();
	~AParticleActor() override = default;

private:
	UParticleSystemComponent* ParticleComponent = nullptr;
};
