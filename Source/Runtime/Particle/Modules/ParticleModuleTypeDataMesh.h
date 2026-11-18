#pragma once

#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Render/Mesh.h"

class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase {
	DECLARE_CLASS(UParticleModuleTypeDataMesh, UParticleModuleTypeDataBase)

	REFLECT_START(ThisClass)
		PROPERTY(mStaticMesh)
	REFLECT_END()

public:
	UParticleModuleTypeDataMesh();

public:
	void Serialize(FStructuredArchive::FRecord Record) override;
	FParticleEmitterInstance* CreateInstance(UParticleEmitter* Emitter, UParticleSystemComponent* Owner) override;

	UStaticMesh* GetStaticMesh() const;
	void SetStaticMesh(UStaticMesh* StaticMesh);

private:
	UStaticMesh* mStaticMesh{};
};
