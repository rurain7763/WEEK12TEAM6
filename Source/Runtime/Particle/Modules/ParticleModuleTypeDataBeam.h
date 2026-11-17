#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"

struct FBeamTypeDataPayload;

class UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
	DECLARE_CLASS(UParticleModuleTypeDataBeam, UParticleModuleTypeDataBase)
public:

	void Serialize(FStructuredArchive::FRecord Record) override;
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

	void ResolveEndPoints(const FParticleEmitterInstance* Emitter, FBeamTypeDataPayload* Payload) const;
private:
	int32 InterpolationPoints = 10; // 빔을 몇칸으로 나눌지(Source~Target)
	int32 MaxBeamCount = 1; 
	bool bAlwaysOn = true; // 빔이 꺼지지 않게 할지
	float Distance = 100.0f; // 빔길이
};
