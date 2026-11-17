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
	// Template 지정 후 호출한다. Automatic에서는 다음 거리 검사 때 다시 선택될 수 있다.
	void SetLODLevel(int32 InLODIndex);
	// 초기화 전에는 인스턴스 생성 시 적용할 단계를 반환한다.
	int32 GetLODLevel() const { return CurrentLODLevel; }

	// 이 PSC의 선택 방식만 재정의한다. 해제하면 에셋의 LODMethod를 따른다.
	void SetLODMethod(EParticleSystemLODMethod InLODMethod);
	void ClearLODMethodOverride();
	EParticleSystemLODMethod GetLODMethod() const;

	// 뷰어는 Tick 전에 월드 공간의 카메라 위치를 전달한다. 해제하면 게임 카메라를 사용한다.
	void SetLODViewLocation(const FVector& InViewLocation);
	void ClearLODViewLocation();

	void ClearEmitterInstances();

	void SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View) override;

	void UpdateDynamicData();

	virtual FPrimitiveSceneProxy* CreateSceneProxy();

private:
	void UpdateAutomaticLOD(float DeltaTime);
	bool GetLODViewLocation(FVector& OutViewLocation) const;

	UParticleSystem* Template = nullptr;
	int32 CurrentLODLevel = 0;
	bool bOverrideLODMethod = false;
	EParticleSystemLODMethod LODMethod = EParticleSystemLODMethod::Automatic;
	bool bHasLODViewLocation = false;
	FVector LODViewLocation;

	TArray<FParticleEmitterInstance*> EmitterInstances;
	float LODDistanceCheckElapsed = 0.f;
};
