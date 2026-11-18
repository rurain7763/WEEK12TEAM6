#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleAccelerationBase.h"
#include "Serialization/StructuredArchive.h"
#include "Serialization/StructuredArchiveSlots.h"

void UParticleModuleAccelerationBase::Serialize(FStructuredArchive::FRecord Record)
{
	if (auto Slot = Record.TryEnterField("bAlwaysInWorldSpace", true))
	{
		Slot.GetValue() << bAlwaysInWorldSpace;
	}
}
