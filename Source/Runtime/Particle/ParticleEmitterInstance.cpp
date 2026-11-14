#include "EnginePCH.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Component/ParticleSystemComponent.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Engine/PrimitiveSceneProxy.h"


FParticleEmitterInstance::~FParticleEmitterInstance()
{
	if (ParticleData)
	{
		delete[] ParticleData;
	}

	if (ParticleIndices)
	{
		delete[] ParticleIndices;
	}
}

void FParticleEmitterInstance::InitParameters(UParticleEmitter* Emitter, UParticleSystemComponent* InOwner)
{
	EmitterTemplate = Emitter;

	if (Emitter)
	{
		CurrentLODLevel = Emitter->GetLODLevel(0);
	}

	Owner = InOwner;
}

void FParticleEmitterInstance::Initialize()
{
	if (!EmitterTemplate || !CurrentLODLevel)
	{
		return;
	}

	EmitterTemplate->CacheEmitterModuleInfo();
	ParticleStride = EmitterTemplate->ParticleSize;
	CurrentLODLevel->BuildModuleLists();

	MaxActiveParticles = EmitterTemplate->MaxParticleCount;

	ParticleData = new uint8[ParticleStride * MaxActiveParticles];

	ParticleIndices = new uint16[MaxActiveParticles];
	for (uint16 i = 0; i < MaxActiveParticles; i++)
	{
		ParticleIndices[i] = i;
	}

	ActiveParticles = 0;
	SpawnFraction = 0.f;
	EmitterToSimulate = FMatrix::Identity;
	SimulateToWorld = FMatrix::Identity;
}

void FParticleEmitterInstance::SpawnParticles(int32 Count, const FVector& InitLocation)
{
	for (int32 i = 0; i < Count; i++)
	{
		SpawnParticle(InitLocation);
	}
}

void FParticleEmitterInstance::SpawnParticle(const FVector& InitLocation)
{
	if (!ParticleData || !CurrentLODLevel)
	{
		return;
	}

	FBaseParticle* Particle = GetParticleFromPosition(ActiveParticles);
	*Particle = FBaseParticle();

	Particle->Location = InitLocation;

	UParticleModule::FSpawnContext Context;
	Context.Offset = 0;
	Context.Emitter = this;
	Context.Particle = Particle;
	Context.SpawnTime = 0.f;

	for (UParticleModule* Module : CurrentLODLevel->SpawnModules)
	{
		Module->Spawn(Context);
	}
		
	ActiveParticles++;
}

void FParticleEmitterInstance::KillParticle(int32 Position)
{
	if (Position >= ActiveParticles)
	{
		return;
	}

	ActiveParticles--;
	std::swap(ParticleIndices[Position], ParticleIndices[ActiveParticles]);
}

void FParticleEmitterInstance::UpdateTransforms()
{
	if (!CurrentLODLevel->RequiredModule)
	{
		EmitterToSimulate = FMatrix::Identity;
		SimulateToWorld = FMatrix::Identity;
		return;
	}

	FMatrix OriginMatrix = Owner ? Owner->GetWorldMatrix() : FMatrix::Identity;

	if (CurrentLODLevel->RequiredModule->GetUseLocalSpace())
	{
		EmitterToSimulate = FMatrix::Identity;
		SimulateToWorld = OriginMatrix;
	}
	else
	{
		EmitterToSimulate = OriginMatrix;
		SimulateToWorld = FMatrix::Identity;
	}
}

void FParticleEmitterInstance::Tick(float DeltaTime)
{
	// 파티클 수명 관리
	for (int32 i = ActiveParticles - 1; i > -1; i--)
	{
		FBaseParticle* Particle = GetParticleFromPosition(i);
		Particle->RelativeTime += DeltaTime * Particle->InvMaxLifeTime;

		if (Particle->RelativeTime >= 1.f)
		{
			KillParticle(i);
		}
	}

	UpdateTransforms();

	// 모듈 업데이트
	UParticleModule::FUpdateContext Context;
	Context.Offset = 0;
	Context.Emitter = this;
	Context.DeltaTime = DeltaTime;

	for (UParticleModule* Module : CurrentLODLevel->UpdateModules)
	{
		Module->Update(Context);
	}

	// 파티클 생성 관리
	if (CurrentLODLevel->SpawnModule)
	{
		SpawnFraction += DeltaTime * CurrentLODLevel->SpawnModule->GetSpawnRate();

		int32 RequiredSpawnCount = FMath::FloorToInt(SpawnFraction);
		int32 SpawnCount = FMath::Min(RequiredSpawnCount, MaxActiveParticles - ActiveParticles);

		FVector InitLocation = EmitterToSimulate.GetOrigin();

		SpawnParticles(SpawnCount, InitLocation);

		// NOTE: 이번 프레임에 요청이 되었으나 생성이 되지 못한 파티클에 대해서는 무시한다. (나중에 수정할지도...?)
		SpawnFraction -= RequiredSpawnCount;
	}

	// 파티클 위치 업데이트
	for (int32 i = 0; i < ActiveParticles; i++)
	{
		FBaseParticle* Particle = GetParticleFromPosition(i);
		Particle->Location += Particle->Velocity * DeltaTime;
		Particle->Rotation += Particle->RotationRate * DeltaTime;
	}
}

void FParticleEmitterInstance::BuildRenderData(FParticleEmitterRenderData& RenderData) const
{
	RenderData.Particles.Reset();

	if (!CurrentLODLevel->RequiredModule)
	{
		return;
	}

	RenderData.SimulateToWorld = SimulateToWorld;
	RenderData.Material = CurrentLODLevel->RequiredModule->GetMaterial();

	RenderData.Particles.SetNum(ActiveParticles);
	for (int32 i = 0; i < ActiveParticles; i++)
	{
		FBaseParticle* Particle = GetParticleFromPosition(i);

		FParticleSpriteRenderData& ParticleRenderData = RenderData.Particles[i];
		ParticleRenderData.Location = SimulateToWorld.TransformPosition(Particle->Location);
		ParticleRenderData.Size = Particle->Size;
		ParticleRenderData.Rotation = Particle->Rotation;
		ParticleRenderData.MaterialParams.TintColor = Particle->Color;
	}
}
