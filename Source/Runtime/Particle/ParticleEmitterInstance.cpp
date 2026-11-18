#include "EnginePCH.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Component/ParticleSystemComponent.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Particle/Modules/ParticleModuleCollision.h"
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
	SetCurrentLODIndex(0);
	Owner = InOwner;
}

void FParticleEmitterInstance::SetCurrentLODIndex(int32 InLODIndex)
{
	if (EmitterTemplate == nullptr) return;

	UParticleLODLevel* NewLODLevel = EmitterTemplate->GetLODLevel(InLODIndex);
	if (NewLODLevel == nullptr) return;

	NewLODLevel->BuildModuleLists();

	CurrentLODLevelIndex = InLODIndex;
	CurrentLODLevel = NewLODLevel;
}

void FParticleEmitterInstance::Initialize()
{
	if (!EmitterTemplate || !CurrentLODLevel)
	{
		return;
	}

	EmitterTemplate->CacheEmitterModuleInfo();
	TypeDataModule = CurrentLODLevel->TypeDataModule;
	TypeDataOffset = EmitterTemplate->TypeDataOffset;
	CollisionPayloadOffset = EmitterTemplate->CollisionPayloadOffset;
	EventPayload = FParticleEventInstancePayload();

	ParticleStride = EmitterTemplate->ParticleSize;
	MaxActiveParticles = EmitterTemplate->MaxParticleCount;

	// 입자 크기 × 최대 개수만큼의 메모리 확보
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
	if (!ParticleData || !EmitterTemplate || !EmitterTemplate->IsEnabled() || !CurrentLODLevel || !CurrentLODLevel->IsEnabled()) {
		return;
	}

	// 활성 영역 바로 다음에 있는 첫 빈 슬롯을 가져옴
	FBaseParticle* Particle = GetParticleFromPosition(ActiveParticles);
	*Particle = FBaseParticle();

	// 재사용 슬롯에 남은 이전 입자의 충돌 상태 제거
	if (FParticleCollisionPayload* CollisionPayload = GetCollisionPayload(Particle))
	{
		*CollisionPayload = FParticleCollisionPayload();
	}

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
	if (TypeDataModule) { Context.Offset = TypeDataOffset; TypeDataModule->Spawn(Context); }

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

void FParticleEmitterInstance::Tick(float DeltaTime) {
	if (!EmitterTemplate || !EmitterTemplate->IsEnabled()) {
		while (ActiveParticles > 0) {
			KillParticle(ActiveParticles - 1);
		}
		SpawnFraction = 0.0f;
		return;
	}

	Tick_KillParticles(DeltaTime);
	UpdateTransforms();
	Tick_SpawnParticles(DeltaTime);
	Tick_ModuleUpdate(DeltaTime);
	Tick_ModulePostUpdate(DeltaTime);
	Tick_MoveParticles(DeltaTime);
}

void FParticleEmitterInstance::Tick_KillParticles(float DeltaTime)
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
}

void FParticleEmitterInstance::Tick_ModuleUpdate(float DeltaTime)
{
	// 모듈 업데이트
	UParticleModule::FUpdateContext Context;
	Context.Offset = 0;
	Context.Emitter = this;
	Context.DeltaTime = DeltaTime;

	for (UParticleModule* Module : CurrentLODLevel->UpdateModules)
	{
		Module->Update(Context);
	}
}

void FParticleEmitterInstance::Tick_ModulePostUpdate(float DeltaTime)
{
	if (!TypeDataModule)
	{
		return;
	}

	UParticleModule::FUpdateContext Context;
	Context.Offset = TypeDataOffset;
	Context.Emitter = this;
	Context.DeltaTime = DeltaTime;

	TypeDataModule->Update(Context);
}

void FParticleEmitterInstance::Tick_SpawnParticles(float DeltaTime)
{
	// 비활성 LOD는 생성량을 누적하지 않는다. 기존 입자의 갱신과 수명 처리는 유지한다.
	if (CurrentLODLevel->IsEnabled() && CurrentLODLevel->SpawnModule)
	{
		SpawnFraction += DeltaTime * CurrentLODLevel->SpawnModule->GetSpawnRate();

		int32 RequiredSpawnCount = FMath::FloorToInt(SpawnFraction);
		int32 SpawnCount = FMath::Min(RequiredSpawnCount, MaxActiveParticles - ActiveParticles);

		FVector InitLocation = EmitterToSimulate.GetOrigin();

		SpawnParticles(SpawnCount, InitLocation);

		// NOTE: 이번 프레임에 요청이 되었으나 생성이 되지 못한 파티클에 대해서는 무시한다. (나중에 수정할지도...?)
		SpawnFraction -= RequiredSpawnCount;
	}
}

void FParticleEmitterInstance::Tick_MoveParticles(float DeltaTime)
{
	// 파티클 위치 업데이트
	UParticleModuleCollision* Collision = CurrentLODLevel->CollisionModule;

	// Kill 시 마지막 입자가 현재 자리로 오므로 뒤에서부터 순회
	for (int32 i = ActiveParticles - 1; i >= 0; i--)
	{
		FBaseParticle* Particle = GetParticleFromPosition(i);

		// 멈춘 입자는 이동하지 않음 (Collision 없는 LOD에서도 유지)
		FParticleCollisionPayload* CollisionPayload = GetCollisionPayload(Particle);
		if (CollisionPayload && (CollisionPayload->Flags & PCF_Freeze))
		{
			continue;
		}

		// 현재 LOD에 Collision이 있으면 이동 경로 충돌 검사 후 이동
		if (Collision && CollisionPayload)
		{
			if (Collision->MoveParticle(*this, *Particle, *CollisionPayload, DeltaTime))
			{
				KillParticle(i);
				continue;
			}
		}
		else
		{
			Particle->Location += Particle->Velocity * DeltaTime;
		}
		Particle->Rotation += Particle->RotationRate * DeltaTime;
	}
}

void FParticleEmitterInstance::BuildRenderData(FParticleEmitterRenderData& RenderData) const {
	RenderData = FParticleEmitterRenderData{};

	if (!EmitterTemplate || !EmitterTemplate->IsEnabled() || !CurrentLODLevel || !CurrentLODLevel->RequiredModule) {
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
		ParticleRenderData.TintColor = Particle->Color;
	}
}
