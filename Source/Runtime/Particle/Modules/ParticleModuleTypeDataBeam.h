#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"

struct FBeamTypeDataPayload;

class UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
	DECLARE_CLASS(UParticleModuleTypeDataBeam, UParticleModuleTypeDataBase)

	REFLECT_START(ThisClass)
		PROPERTY(InterpolationPoints)
		PROPERTY(MaxBeamCount)
		PROPERTY(bAlwaysOn)
		PROPERTY(Distance)
	REFLECT_END()

public:

	void Serialize(FStructuredArchive::FRecord Record) override;
	void PostEditChangeProperty(const FProperty& Property) override;
	UParticleModuleTypeDataBeam();

	FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) override;
	uint32 RequiredBytes() const override;

	void Spawn(const FSpawnContext& Ctx) override;
	void Update(const FUpdateContext& Ctx) override;

	FBeamTypeDataPayload* GetPayload(FBaseParticle* Particle, int32 Offset) const;
	FVector* GetPoints(FBaseParticle* Particle, int32 Offset) const;
	
	int32 GetMaxBeamCount() const { return MaxBeamCount; }
	bool IsAlwaysOn() const { return bAlwaysOn; }
	int32 GetInterpolationPonts() const { return InterpolationPoints; }
	float GetDistance() const { return Distance; }

	void ResolveEndPoints(const FParticleEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const;
private:
	int32 InterpolationPoints = 10; // 빔을 몇칸으로 나눌지(Source~Target)
	int32 MaxBeamCount = 1; 
	bool bAlwaysOn = true; // 빔이 꺼지지 않게 할지
	float Distance = 100.0f; // 빔길이
	float NoiseRange = 0.f;  // 빔 안쪽 점을 수직으로 흔드는 최대 거리. 0 = 노이즈 없음
};
