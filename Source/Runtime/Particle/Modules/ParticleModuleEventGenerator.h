#pragma once

#include "Containers/Array.h"
#include "Particle/Modules/ParticleModuleEventBase.h"

struct FParticleEventGeneratorInfo
{
	FString EventName = "Collision";
	int32 Frequency = 1;
	bool bFirstTimeOnly = false;
	bool bLastTimeOnly = false;
};

class UParticleModuleEventGenerator : public UParticleModuleEventBase
{
	DECLARE_CLASS(UParticleModuleEventGenerator, UParticleModuleEventBase)

public:
	void Serialize(FStructuredArchive::FRecord Record) override;

	// 복수 보고 규칙. 실행 시 이미터의 CollisionTrackingCount 하나를 공유.
	TArray<FParticleEventGeneratorInfo> Events = { FParticleEventGeneratorInfo{} };
};
