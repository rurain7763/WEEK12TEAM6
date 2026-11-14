#pragma once

#include "Component/FXSystemComponent.h"

class UParticleSystem;
struct FParticleEmitterInstance;

class UParticleSystemComponent : public UFXSystemComponent
{
	DECLARE_CLASS(UParticleSystemComponent, UFXSystemComponent)

public:
	UParticleSystemComponent();

	~UParticleSystemComponent();

	void BeginPlay() override;

	void TickComponent(float DeltaTime) override;

	void EndPlay() override;

	void SetTemplate(UParticleSystem* InSystem);

	void ClearEmitterInstances();

	void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View) override;

	void UpdateDynamicData();

	virtual FPrimitiveSceneProxy* CreateSceneProxy();

private:
	UParticleSystem* Template = nullptr;
	TArray<FParticleEmitterInstance*> EmitterInstances;
};
