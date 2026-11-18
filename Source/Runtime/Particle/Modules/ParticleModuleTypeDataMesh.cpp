#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleTypeDataMesh.h"
#include "Asset/AssetManager.h"
#include "Particle/ParticleMeshEmitterInstance.h"
#include "Serialization/StructuredArchiveHelpers.h"

UParticleModuleTypeDataMesh::UParticleModuleTypeDataMesh() {
	mStaticMesh = UAssetManager::GetAssetByPath<UStaticMesh>("Cube");
}

void UParticleModuleTypeDataMesh::Serialize(FStructuredArchive::FRecord Record) {
	StructuredArchiveHelpers::SerializeAssetReference(Record, "StaticMesh", mStaticMesh);
}

FParticleEmitterInstance* UParticleModuleTypeDataMesh::CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) {
	FParticleEmitterInstance* Instance{ new FParticleMeshEmitterInstance{} };
	Instance->InitParameters(Emitter, Owner);
	return Instance;
}

UStaticMesh* UParticleModuleTypeDataMesh::GetStaticMesh() const {
	return mStaticMesh;
}

void UParticleModuleTypeDataMesh::SetStaticMesh(UStaticMesh* StaticMesh) {
	mStaticMesh = StaticMesh;
}
