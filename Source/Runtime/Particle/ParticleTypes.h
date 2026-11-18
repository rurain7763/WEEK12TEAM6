#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include "Math/Vector.h"
#include "Math/Color.h"

class UPrimitiveComponent;

// MaxCollisions 도달 시 입자 처리 방식
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

// 입자별 충돌 상태. 0으로 초기화, 횟수는 올려 셈
struct FParticleCollisionPayload
{
	int32 CollisionCount = 0;
	uint32 Flags = PCF_None;
};

// 이미터 인스턴스별 누적 카운터. 개별 입자의 payload와 분리.
struct FParticleEventInstancePayload
{
	uint32 CollisionTrackingCount = 0;
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
	float TexU = 0.f;   // TextureTileTime > 0일 때 태어난 시각으로 정한 U (이후 고정)
};

// UE: FAnimTrailTypeDataPayload (ParticleHelper.h). 입자 위치 = 두 소켓의 중앙 (simulate 공간)
// 정점: Location - Direction * Length, Location + Direction * Length. 카메라 방향은 쓰지 않는다
struct FAnimTrailTypeDataPayload : public FRibbonTypeDataPayload
{
	FVector Direction = FVector::ZeroVector; // 첫 소켓 -> 두 번째 소켓 방향 (단위 벡터)
	float Length = 0.f;                      // 두 소켓 거리의 절반 x Width
};