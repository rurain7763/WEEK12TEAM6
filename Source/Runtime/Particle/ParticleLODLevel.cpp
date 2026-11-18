#include "EnginePCH.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
void UParticleLODLevel::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("Level", Level);
	Record << SA_VALUE("bEnable", bEnable);
	StructuredArchiveHelpers::SerializeArray(Record, "Modules", Modules);
	StructuredArchiveHelpers::SerializeObject(Record, "SpawnModule", SpawnModule);
	StructuredArchiveHelpers::SerializeObject(Record, "RequiredModule", RequiredModule);
	StructuredArchiveHelpers::SerializeObject(Record, "TypeDataModule", TypeDataModule);

}

void UParticleLODLevel::SetLevel(int32 InLevel) {
	Level = InLevel;
}

void UParticleLODLevel::BuildModuleLists()
{
	SpawnModules.Reset();
	UpdateModules.Reset();
	CollisionModule = nullptr;
	EventGeneratorModule = nullptr;

	for (UParticleModule* Module : Modules)
	{
		if (UParticleModuleCollision* Collision = Cast<UParticleModuleCollision>(Module))
		{
			CollisionModule = Collision;
		}
		if (UParticleModuleEventGenerator* Generator = Cast<UParticleModuleEventGenerator>(Module))
		{
			EventGeneratorModule = Generator;
		}

		if (Module->bSpawnModule)
		{
			SpawnModules.Add(Module);
		}

		if (Module->bUpdateModule)
		{
			UpdateModules.Add(Module);
		}
	}
}
