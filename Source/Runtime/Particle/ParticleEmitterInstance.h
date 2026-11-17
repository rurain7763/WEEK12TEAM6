#pragma once

#include "Particle/ParticleTypes.h"
#include "Math/Matrix.h"

class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;
class UParticleModuleTypeDataBase;
struct FParticleEmitterRenderData;

struct FParticleEmitterInstance
{
	UParticleEmitter* EmitterTemplate = nullptr;

	int32 CurrentLODLevelIndex = -1;
	UParticleLODLevel* CurrentLODLevel = nullptr;

	UParticleSystemComponent* Owner = nullptr;
	
	// 파티클 데이터 배열 시작 주소
	uint8* ParticleData = nullptr;
	
	// 다음 입자 슬롯까지 이동할 바이트 수.
	int32 ParticleStride = 0;

	// 확보한 메모리에 최대 몇 개의 입자를 담을 수 있는지
	int32 MaxActiveParticles = 0;

	uint16* ParticleIndices = nullptr; // 활성화된 파티클 인덱스 배열 시작 주소
	int32 ActiveParticles = 0;

	float SpawnFraction = 0.f;

	FMatrix EmitterToSimulate;
	FMatrix SimulateToWorld;

	FParticleEmitterInstance() = default;

	virtual ~FParticleEmitterInstance();

	// 복사 생성과 복사 대입 금지
	FParticleEmitterInstance(const FParticleEmitterInstance& Other) = delete;
	FParticleEmitterInstance& operator=(const FParticleEmitterInstance& Other) = delete;

	void InitParameters(UParticleEmitter* Emitter, UParticleSystemComponent* InOwner);

	virtual void Initialize();

	inline FBaseParticle* GetParticleFromPosition(int32 Position) const
	{
		return (FBaseParticle*)&ParticleData[ParticleIndices[Position] * ParticleStride];
	}

	virtual void SpawnParticles(int32 Count, const FVector& InitLocation);
	virtual void KillParticle(int32 Position);
	virtual void Tick(float DeltaTime);
	virtual void BuildRenderData(FParticleEmitterRenderData& RenderData) const;

	virtual void SpawnParticle(const FVector& InitLocation);
	void UpdateTransforms();

	// Tick 단계 (UE의 Tick_* 하위 함수 대응). 자식 Instance가 순서를 바꿔 조립할 수 있게 나눔
	void Tick_KillParticles(float DeltaTime);    // 수명 진행 + 죽은 파티클 제거
	void Tick_ModuleUpdate(float DeltaTime);     // Update 모듈들
	void Tick_ModulePostUpdate(float DeltaTime); // TypeData Update
	void Tick_SpawnParticles(float DeltaTime);   // SpawnModule 비율로 생성
	void Tick_MoveParticles(float DeltaTime);    // Velocity·RotationRate 적용

	//TypeDataBase계열 변수
	int32 TypeDataOffset = 0;
	UParticleModuleTypeDataBase* TypeDataModule = nullptr;

	// Collision payload 시작 위치 (없으면 -1)
	int32 CollisionPayloadOffset = -1;

	// 이미터 전체 충돌 횟수 (EventGenerator Frequency 계산용)
	FParticleEventInstancePayload EventPayload;

	inline FParticleCollisionPayload* GetCollisionPayload(FBaseParticle* Particle) const
	{
		if (CollisionPayloadOffset < 0) return nullptr;
		return (FParticleCollisionPayload*)((uint8*)Particle + CollisionPayloadOffset);
	}


	void SetCurrentLODIndex(int32 InLODIndex);
};
