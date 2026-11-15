#include "EnginePCH.h"
#include "ParticleActor.h"
#include "Component/ParticleSystemComponent.h"

AParticleActor::AParticleActor()
{
	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");
	ParticleComponent->SetupAttachment(GetRootComponent());
}
