#include "EnginePCH.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Serialization/StructuredArchiveHelpers.h"
#include "Particle/ParticleEmitterInstance.h"
#include "Component/ParticleSystemComponent.h"
#include "Collision/HitResult.h"

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

void UParticleModuleEventGenerator::HandleParticleCollision(FParticleEmitterInstance& Owner, const FBaseParticle& Particle,
	int32 CollisionCount, int32 MaxCollisions, const FHitResult& Hit, const FVector& WorldVelocity) const
{
	if (!Owner.Owner)
	{
		return;
	}

	// 이미터 전체 충돌 횟수 (Frequency 판단용)
	Owner.EventPayload.CollisionTrackingCount++;

	for (const FParticleEventGeneratorInfo& Info : Events)
	{
		// 첫 충돌만 / 마지막 충돌만 (둘 다 켜면 첫 충돌 기준)
		if (Info.bFirstTimeOnly)
		{
			if (CollisionCount != 1) continue;
		}
		else if (Info.bLastTimeOnly)
		{
			if (CollisionCount < MaxCollisions) continue;
		}

		// 이미터 전체 충돌 N번에 1번
		if (Info.Frequency > 1 && (Owner.EventPayload.CollisionTrackingCount % Info.Frequency) != 0)
		{
			continue;
		}

		// 입자 주소 대신 충돌 당시 값 복사
		FParticleEventCollideData Data;
		Data.EventName = Info.EventName;
		Data.EmitterIndex = Owner.EmitterIndex;
		Data.ParticleRelativeTime = Particle.RelativeTime;
		Data.Location = Hit.Location;
		Data.Normal = Hit.Normal;
		Data.Direction = WorldVelocity.GetSafeNormal();
		Data.Velocity = WorldVelocity;
		Data.Component = Hit.HitComponent;

		Owner.Owner->ReportCollisionEvent(Data);
	}
}
