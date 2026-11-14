#include "EnginePCH.h"
#include "ParticleActor.h"
#include "Particle/Tests/ParticleSetupSmokeTest.h"
#include "Particle/Tests/ParticlePlaybackTestSettings.h"

AParticleActor::AParticleActor()
{
	ParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>("ParticleSystemComponent");
	ParticleComponent->SetupAttachment(GetRootComponent());
}

AParticleActor::~AParticleActor()
{
	// AActor deletes owned components later, so clear instances before their settings.
	ClearTestPlayback();
}

void AParticleActor::BeginPlay()
{
	if (HasActorBegunPlay())
		return;

	TestPlaybackSettings = std::make_unique<FParticlePlaybackTestSettings>(this);
	ParticleComponent->SetTemplate(TestPlaybackSettings->System.get());
	// AActor::BeginPlay starts the component and registers its normal world tick.
	Super::BeginPlay();

	HTR_LOG(Info, "[ParticlePlaybackTest] Local=green, World=magenta; each 10/s, lifetime 5s. Move the actor during play to compare.");
	// Independent assertions do not replace the persistent playback component.
	ParticleSetupSmokeTest::Run(this);
}

void AParticleActor::EndPlay()
{
	Super::EndPlay();
	ClearTestPlayback();
}

void AParticleActor::ClearTestPlayback()
{
	if (ParticleComponent)
	{
		ParticleComponent->ClearEmitterInstances();
		ParticleComponent->SetTemplate(nullptr);
	}
	TestPlaybackSettings.reset();
}
