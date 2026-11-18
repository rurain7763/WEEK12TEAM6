#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleAccelerationConstant.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Serialization/MathSerialization.h"

UParticleModuleAccelerationConstant::UParticleModuleAccelerationConstant()
{
	bUpdateModule = true;
}

void UParticleModuleAccelerationConstant::Serialize(FStructuredArchive::FRecord Record)
{
	Super::Serialize(Record);
	Record << SA_VALUE("Acceleration", Acceleration);
}

void UParticleModuleAccelerationConstant::Update(const FUpdateContext& Ctx)
{
	// 월드 중력은 Local Space 입자에서도 같은 월드 방향으로 작용한다.
	const FVector SimAcceleration = bAlwaysInWorldSpace ?
		Ctx.Emitter->SimulateToWorld.Inverse().TransformVector(Acceleration) :
		Ctx.Emitter->EmitterToSimulate.TransformVector(Acceleration);
		
	const FVector DeltaVelocity = SimAcceleration * Ctx.DeltaTime;
	
	for (int32 i = 0; i < Ctx.Emitter->ActiveParticles; ++i)
	{
		Ctx.Emitter->GetParticleFromPosition(i)->Velocity += DeltaVelocity;
	}
}
