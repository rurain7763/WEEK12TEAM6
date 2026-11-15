#include "EnginePCH.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleLODLevel::Serialize(FStructuredArchive::FRecord Record)
{
	Record << SA_VALUE("Level", Level);
	Record << SA_VALUE("bEnable", bEnable);
	StructuredArchiveHelpers::SerializeArray(Record, "Modules", Modules);
	StructuredArchiveHelpers::SerializeObject(Record, "SpawnModule", SpawnModule);
	StructuredArchiveHelpers::SerializeObject(Record, "RequiredModule", RequiredModule);
}

void UParticleLODLevel::BuildModuleLists()
{
	SpawnModules.Reset();
	UpdateModules.Reset();

	for (UParticleModule* Module : Modules)
	{
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
