#include "EnginePCH.h"
#include "Component/ParticleSystemComponent.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Engine/PrimitiveSceneProxy.h"


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

	if (!Template)
	{
		return;
	}

	EmitterInstances.SetNum(Template->Emitters.Num());
	for (int32 i = 0; i < Template->Emitters.Num(); i++)
	{
		UParticleEmitter* Emitter = Template->Emitters[i];
		EmitterInstances[i] = Emitter->CreateInstance(this);
	}

	// 임시 확인용: 거리 기반 선택 연결 전 LOD 1로 재생한다.
	SetLODLevel(1);
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	Super::TickComponent(DeltaTime);

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
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance)
		{
			Instance->SetCurrentLODIndex(InLODIndex);
		}
	}
}

void UParticleSystemComponent::ClearEmitterInstances()
{
	for (int32 i = 0; i < EmitterInstances.Num(); i++)
	{
		delete EmitterInstances[i];
	}
	EmitterInstances.Reset();
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
