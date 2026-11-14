#pragma once

#include "GameFramework/Actor.h"
#include <memory>

class UParticleSystemComponent;
struct FParticlePlaybackTestSettings;

// Temporary playback host for the new particle-system implementation.
class AParticleActor : public AActor
{
	DECLARE_CLASS(AParticleActor, AActor)

	REFLECT_START(ClassName)
	REFLECT_END()

public:
	AParticleActor();
	~AParticleActor() override;

	void BeginPlay() override;
	void EndPlay() override;

private:
	void ClearTestPlayback();
	UParticleSystemComponent* ParticleComponent = nullptr;
	std::unique_ptr<FParticlePlaybackTestSettings> TestPlaybackSettings;
};
