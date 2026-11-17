#pragma once

#include "Math/Vector.h"
#include "Math/Color.h"

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