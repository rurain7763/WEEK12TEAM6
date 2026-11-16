#pragma once

#include "Component/FXSystemComponent.h"
#include "Particle/ParticleSystem.h"

struct FParticleEmitterInstance;

class UParticleSystemComponent : public UFXSystemComponent
{
	DECLARE_CLASS(UParticleSystemComponent, UFXSystemComponent)

	REFLECT_START(ThisClass)
		PROPERTY(Template)
	REFLECT_END()

public:
	UParticleSystemComponent();

	~UParticleSystemComponent();

	void BeginPlay() override;

	void TickComponent(float DeltaTime) override;

	void EndPlay() override;

	void SetTemplate(UParticleSystem* InSystem);
	UParticleSystem* GetTemplate() const;
	void ReinitializeEmitterInstances();
	void SetLODLevel(int32 InLODIndex);

	void ClearEmitterInstances();

	void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View) override;

	void UpdateDynamicData();

	virtual FPrimitiveSceneProxy* CreateSceneProxy();

private:
	void UpdateAutomaticLOD(float DeltaTime);

	UParticleSystem* Template = nullptr;

	TArray<FParticleEmitterInstance*> EmitterInstances;
	bool mEmitterInstancesInitialized{};
	float LODDistanceCheckElapsed = 0.f;
};
