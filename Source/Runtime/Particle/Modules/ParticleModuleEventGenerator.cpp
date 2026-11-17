#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Serialization/StructuredArchiveHelpers.h"

void UParticleModuleEventGenerator::Serialize(FStructuredArchive::FRecord Record)
{
	StructuredArchiveHelpers::SerializeArray(Record, "Events", Events,
		[](FStructuredArchive::FRecord& EventRecord, FParticleEventGeneratorInfo& Event)
		{
			EventRecord << SA_VALUE("EventName", Event.EventName);
			EventRecord << SA_VALUE("Frequency", Event.Frequency);
			EventRecord << SA_VALUE("bFirstTimeOnly", Event.bFirstTimeOnly);
			EventRecord << SA_VALUE("bLastTimeOnly", Event.bLastTimeOnly);
		});
}
