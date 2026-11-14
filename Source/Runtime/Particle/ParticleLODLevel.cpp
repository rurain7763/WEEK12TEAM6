#include "EnginePCH.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"


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
