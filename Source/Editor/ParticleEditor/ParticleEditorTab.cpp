#include "EnginePCH.h"
#include "ParticleEditorTab.h"
#include "Component/ParticleSystemComponent.h"
#include "Core/PathUtils.h"
#include "Editor/Preview/PreviewViewportPanel.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Editor/Viewports/ViewportCameraMath.h"
#include "Engine/World.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "Particle/ParticleSystem.h"

namespace {
	constexpr float ParticlePreviewCameraDistance{ 10.0f };

	FEditorTabSettings MakeParticleEditorTabSettings(UParticleSystem* ParticleSystem) {
		FEditorTabSettings Settings{ "Particle Editor", "ParticleEditorPreviewDock", true, true };
		if (ParticleSystem) {
			const FString& AssetPath{ ParticleSystem->GetPath() };
			Settings.Title = "Particle Editor - " + PathUtils::ToUtf8(PathUtils::FromUtf8(AssetPath).filename()) + "###ParticleEditor:" + AssetPath;
			Settings.DockSpaceName += "##" + AssetPath;
		}

		return Settings;
	}
}

FParticleEditorTab::FParticleEditorTab(UParticleSystem* InParticleSystem)
	: FPreviewEditorTab{ MakeParticleEditorTabSettings(InParticleSystem), InParticleSystem ? "Particle Preview##ParticleEditor:" + InParticleSystem->GetPath() : "Particle Preview##ParticleEditor" },
	  mParticleSystem{ InParticleSystem } {
	ViewportPanel->GetViewport()->SetGridVisible(false);

	if (!mParticleSystem) {
		return;
	}

	AActor* PreviewActor{ GetWorld()->SpawnActor(AParticleActor::StaticClass()) };
	if (!PreviewActor) {
		return;
	}

	mPreviewComponent = Cast<UParticleSystemComponent>(PreviewActor->FindComponentByClass(UParticleSystemComponent::StaticClass()));
	if (mPreviewComponent) {
		mPreviewComponent->SetWorldLocation(FVector::ZeroVector, false);
		mPreviewComponent->SetTemplate(mParticleSystem);
		mPreviewComponent->BeginPlay();
	}
}

FParticleEditorTab::~FParticleEditorTab() {
	if (mPreviewComponent) {
		mPreviewComponent->EndPlay();
	}
}

const UObject* FParticleEditorTab::GetEditingAsset() const {
	return mParticleSystem;
}

FString FParticleEditorTab::GetEditingFilePath() const {
	return mParticleSystem ? mParticleSystem->GetAssetFilePath() : FString{};
}

void FParticleEditorTab::TickPreview(float DeltaTime) {
	if (mPendingCameraSetup && ViewportPanel->GetViewport()->IsVisible()) {
		InitializePreviewCamera();
	}

	if (mPreviewComponent) {
		mPreviewComponent->TickComponent(DeltaTime);
	}
}

void FParticleEditorTab::InitializePreviewCamera() {
	FEditorViewportClient* Client{ ViewportPanel->GetEditorClient() };
	FViewCamera Camera{ Client->GetCamera() };
	Camera.Transform.Rotation = ViewportCameraMath::MakeCameraRotation(45.0f, -20.0f);
	Camera.Transform.Location = FVector::ZeroVector - ViewportCameraMath::CameraForward(Camera.Transform.Rotation) * ParticlePreviewCameraDistance;
	Client->InitializeCamera(Camera, EMultipleViewportsCameraPreset::Perspective);
	mPendingCameraSetup = false;
}
