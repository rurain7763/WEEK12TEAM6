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

	if (!Template->LODDistances.IsValidIndex(CurrentLODLevel))
	{
		CurrentLODLevel = 0;
	}
	// 인스턴스 생성 전에 지정한 단계도 적용한다.
	SetLODLevel(CurrentLODLevel);

	// Automatic은 첫 Tick에서 바로 카메라 거리를 검사한다.
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

	// 인스턴스 생성 전에도 에셋에 해당 단계가 있는지 확인한다.
	for (UParticleEmitter* Emitter : Template->Emitters)
	{
		if (!Emitter || !Emitter->GetLODLevel(InLODIndex))
		{
			return;
		}
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

	CurrentLODLevel = InLODIndex;

	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance && Instance->CurrentLODLevelIndex != InLODIndex)
		{
			Instance->SetCurrentLODIndex(InLODIndex);
		}
	}
}

void UParticleSystemComponent::SetLODMethod(EParticleSystemLODMethod InLODMethod)
{
	if (bOverrideLODMethod && LODMethod == InLODMethod)
	{
		return;
	}

	bOverrideLODMethod = true;
	LODMethod = InLODMethod;
	LODDistanceCheckElapsed = Template ? Template->LODDistanceCheckTime : 0.f;
}

void UParticleSystemComponent::ClearLODMethodOverride()
{
	if (!bOverrideLODMethod)
	{
		return;
	}

	bOverrideLODMethod = false;
	LODDistanceCheckElapsed = Template ? Template->LODDistanceCheckTime : 0.f;
}

EParticleSystemLODMethod UParticleSystemComponent::GetLODMethod() const
{
	if (bOverrideLODMethod)
	{
		return LODMethod;
	}
	return Template ? Template->LODMethod : EParticleSystemLODMethod::Automatic;
}

void UParticleSystemComponent::SetLODViewLocation(const FVector& InViewLocation)
{
	if (!bHasLODViewLocation)
	{
		LODDistanceCheckElapsed = Template ? Template->LODDistanceCheckTime : 0.f;
	}
	bHasLODViewLocation = true;
	LODViewLocation = InViewLocation;
}

void UParticleSystemComponent::ClearLODViewLocation()
{
	if (!bHasLODViewLocation)
	{
		return;
	}
	bHasLODViewLocation = false;
	LODDistanceCheckElapsed = Template ? Template->LODDistanceCheckTime : 0.f;
}

bool UParticleSystemComponent::GetLODViewLocation(FVector& OutViewLocation) const
{
	if (bHasLODViewLocation)
	{
		OutViewLocation = LODViewLocation;
		return true;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetPlayerController() : nullptr;
	if (!PlayerController || !PlayerController->GetPlayerCameraManager())
	{
		return false;
	}
	OutViewLocation = PlayerController->GetPlayerView().Location;
	return true;
}

void UParticleSystemComponent::UpdateAutomaticLOD(float DeltaTime)
{
	if (!Template || EmitterInstances.IsEmpty())
	{
		return;
	}

	if (GetLODMethod() != EParticleSystemLODMethod::Automatic)
	{
		// 에셋의 방식이 Automatic으로 바뀌어도 다음 Tick에 바로 검사한다.
		LODDistanceCheckElapsed = Template->LODDistanceCheckTime;
		return;
	}

	LODDistanceCheckElapsed += DeltaTime;
	if (LODDistanceCheckElapsed < Template->LODDistanceCheckTime)
	{
		return;
	}

	FVector CameraLocation;
	if (!GetLODViewLocation(CameraLocation))
	{
		return;
	}
	LODDistanceCheckElapsed = 0.f;

	// 로드 및 BeginPlay에서 모든 이미터의 단계 수가 일치하는지 검사한다.
	const int32 LODCount = Template->LODDistances.Num();

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
