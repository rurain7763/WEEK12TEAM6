#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include "Math/Vector.h"
#include "Math/Color.h"

class UPrimitiveComponent;

// 충돌 횟수가 MaxCollisions에 도달한 입자의 처리. 이번 범위는 Kill/Freeze 2종.
enum class EParticleCollisionComplete : int32
{
	Kill,
	Freeze
};

enum EParticleCollisionFlags : uint32
{
	PCF_None = 0,
	PCF_Freeze = 1u << 0
};

// 입자별 충돌 상태. 0으로 지운 상태가 곧 초기 상태다.
// 횟수는 올려 세고, 충돌 시점에 현재 LOD의 MaxCollisions와 비교한다.
struct FParticleCollisionPayload
{
	int32 CollisionCount = 0;
	uint32 Flags = PCF_None;
};

// 이미터 인스턴스별 누적 카운터. 개별 입자의 payload와 분리.
struct FParticleEventInstancePayload
{
	uint32 CollisionCount = 0;
};

// 충돌 당시 값의 복사본. 입자 메모리의 주소는 저장하지 않음.
struct FParticleEventCollideData
{
	FString EventName;
	int32 EmitterIndex = -1;
	float ParticleRelativeTime = 0.f;
	float Time = 0.f; // 월드 시각

	FVector Location = FVector::ZeroVector;
	FVector Normal = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector; // 충돌 직전 월드 속도
	UPrimitiveComponent* Component = nullptr; // 비소유 참조
};

struct FBaseParticle
{
	FVector Location = FVector::ZeroVector;
	float RelativeTime = 0.f; // 정규화된 나이 0~1

	FVector Velocity = FVector::ZeroVector;
	float InvMaxLifeTime = 0.f; // 1 / MaxLifeTime

	FVector BaseSize = FVector::OneVector;
	float Rotation = 0.f;

	FVector Size = FVector::OneVector;
	float RotationRate = 1.f;
	
	FLinearColor BaseColor = FLinearColor::White;
	FLinearColor Color = FLinearColor::White;
};
struct FBeamTypeDataPayload
{
	// Beam 시작
	FVector		SourcePoint; //Beam 시작점
	FVector		SourceTangent; //Beam 각도
	float		SourceStrength; // 곡선 계산시 강도

	// Beam 끝
	FVector		TargetPoint; 
	FVector		TargetTangent;
	float		TargetStrength;

	/** Target lock, extreme max, Number of noise points				*/
	int32		Lock_Max_NumNoisePoints;
	
	/** Number of segments to render (steps)							*/
	int32		InterpolationSteps;

	/** Direction to step in											*/
	FVector		Direction;
	/** StepSize (for each segment to be rendered)						*/
	float		StepSize;
	/** Number of segments to render (steps)							*/
	int32		Steps;
	/** The 'extra' amount to travel (partial segment)					*/
	float		TravelRatio;

	/** The number of triangles to render for this beam					*/
	int32		TriangleCount;
};

struct FRibbonTypeDataPayload
{
	int32 NextSlot = -1;
	int32 PrevSlot = -1;
};