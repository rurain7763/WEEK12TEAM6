#include "EnginePCH.h"
#include "Component/ParticleSystemComponent.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"


UParticleSystemComponent::UParticleSystemComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UParticleSystemComponent::~UParticleSystemComponent()
{
	ClearEmitterInstances();
}

void UParticleSystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Template || !Template->ValidateLODLevels())
	{
		return;
	}

	EmitterInstances.SetNum(Template->Emitters.Num());
	for (int32 i = 0; i < Template->Emitters.Num(); i++)
	{
		UParticleEmitter* Emitter = Template->Emitters[i];
		EmitterInstances[i] = Emitter->CreateInstance(this);
	}

	// 첫 Tick에서 바로 카메라 거리를 검사한다.
	LODDistanceCheckElapsed = Template->LODDistanceCheckTime;
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

	UpdateAutomaticLOD(DeltaTime);

	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		Instance->Tick(DeltaTime);
	}

	UpdateDynamicData();
}

void UParticleSystemComponent::EndPlay()
{
	ClearEmitterInstances();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* InSystem)
{
	Template = InSystem;
}

void UParticleSystemComponent::SetLODLevel(int32 InLODIndex)
{
	if (!Template || !Template->LODDistances.IsValidIndex(InLODIndex))
	{
		return;
	}

	// 일부 이미터만 전환되는 것을 막는다.
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance && (!Instance->EmitterTemplate ||
			!Instance->EmitterTemplate->GetLODLevel(InLODIndex)))
		{
			return;
		}
	}

	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance && Instance->CurrentLODLevelIndex != InLODIndex)
		{
			Instance->SetCurrentLODIndex(InLODIndex);
		}
	}
}

void UParticleSystemComponent::UpdateAutomaticLOD(float DeltaTime)
{
	if (!Template || Template->LODMethod != EParticleSystemLODMethod::Automatic ||
		EmitterInstances.IsEmpty())
	{
		return;
	}

	LODDistanceCheckElapsed += DeltaTime;
	if (LODDistanceCheckElapsed < Template->LODDistanceCheckTime)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetPlayerController() : nullptr;
	if (!PlayerController || !PlayerController->GetPlayerCameraManager())
	{
		return;
	}
	LODDistanceCheckElapsed = 0.f;

	// 로드 및 BeginPlay에서 모든 이미터의 단계 수가 일치하는지 검사한다.
	const int32 LODCount = Template->LODDistances.Num();

	const FVector CameraLocation = PlayerController->GetPlayerView().Location;
	const float DistanceSquared = (CameraLocation - GetWorldLocation()).SizeSquared();
	int32 DesiredLODIndex = 0;
	for (int32 Index = 1; Index < LODCount; ++Index)
	{
		const float MinDistance = Template->LODDistances[Index];
		if (DistanceSquared < MinDistance * MinDistance)
		{
			break;
		}
		DesiredLODIndex = Index;
	}
	SetLODLevel(DesiredLODIndex);
}

void UParticleSystemComponent::ClearEmitterInstances()
{
	for (int32 i = 0; i < EmitterInstances.Num(); i++)
	{
		delete EmitterInstances[i];
	}
	EmitterInstances.Reset();
	LODDistanceCheckElapsed = 0.f;
}

void UParticleSystemComponent::SubmitToRenderQueue(FRenderQueue& RenderQueue, const FViewBasis& View)
{
	FPrimitiveSceneProxy* Proxy = GetSceneProxy();
	if (!Proxy)
	{
		return;
	}

	Proxy->SubmitToRenderQueue(RenderQueue, View);
}

void UParticleSystemComponent::UpdateDynamicData()
{
	FParticleSystemSceneProxy* Proxy = static_cast<FParticleSystemSceneProxy*>(GetSceneProxy());
	if (!Proxy)
	{
		return;
	}

	TArray<FParticleEmitterRenderData> RenderDatas;
	RenderDatas.SetNum(EmitterInstances.Num());
	for (int32 i = 0; i < EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances[i];
		Instance->BuildRenderData(RenderDatas[i]);
	}

	Proxy->SetDynamicData(RenderDatas);
}

FPrimitiveSceneProxy* UParticleSystemComponent::CreateSceneProxy()
{
	return new FParticleSystemSceneProxy(this);
}
