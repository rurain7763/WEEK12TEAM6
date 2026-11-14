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
