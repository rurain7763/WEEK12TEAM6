#include "EnginePCH.h"
#include "Particle/ParticleMeshEmitterInstance.h"
#include "Engine/PrimitiveSceneProxy.h"
#include "Math/Quat.h"
#include "Particle/Modules/ParticleModuleTypeDataMesh.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"

void FParticleMeshEmitterInstance::BuildRenderData(FParticleEmitterRenderData& RenderData) const {
	RenderData = FParticleEmitterRenderData{};
	RenderData.DynamicEmitterType = EDynamicEmitterType::Mesh;

	if (!EmitterTemplate || !EmitterTemplate->IsEnabled() || !CurrentLODLevel) {
		return;
	}

	const UParticleModuleTypeDataMesh* TypeData{ Cast<UParticleModuleTypeDataMesh>(CurrentLODLevel->TypeDataModule) };
	if (!TypeData || !TypeData->GetStaticMesh()) {
		return;
	}

	RenderData.mStaticMesh = TypeData->GetStaticMesh();
	RenderData.SimulateToWorld = SimulateToWorld;
	RenderData.mMeshParticles.SetNum(ActiveParticles);

	for (int32 Index{}; Index < ActiveParticles; ++Index) {
		const FBaseParticle* Particle{ GetParticleFromPosition(Index) };
		const FQuat Rotation{ FQuat::MakeFromAxisAngle(FVector{ 0.0f, 0.0f, 1.0f }, Particle->Rotation) };
		FObjectInstance& Instance{ RenderData.mMeshParticles[Index] };
		Instance.Model = FMatrix::MakeFromTRS(Particle->Location, Rotation, Particle->Size) * SimulateToWorld;
		Instance.NormalMatrix = Instance.Model.Inverse().GetTransposed();
		Instance.Color = Particle->Color;
	}
}
