#include "EnginePCH.h"
#include "ParticleEventManager.h"
#include "Component/ParticleSystemComponent.h"

void AParticleEventManager::HandleParticleCollisionEvents(UParticleSystemComponent* Component,
	const TArray<FParticleEventCollideData>& CollisionEvents)
{
	if (!Component)
	{
		return;
	}

	for (const FParticleEventCollideData& Event : CollisionEvents)
	{
		Component->OnParticleCollide.Broadcast(Component, Event);
	}
}
