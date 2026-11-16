#include "EnginePCH.h"
#include "ParticleEditorTab.h"
#include "ParticleEmitterPanel.h"
#include "ParticleModuleDetailsPanel.h"
#include "Component/ParticleSystemComponent.h"
#include "Core/PathUtils.h"
#include "Editor/Preview/PreviewViewportPanel.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Editor/Viewports/ViewportCameraMath.h"
#include "Engine/World.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Serialization/JsonArchive.h"

#include <cstring>
#include <limits>

namespace {
	constexpr float ParticlePreviewCameraDistance{ 10.0f };

	FEditorTabSettings MakeParticleEditorTabSettings(UParticleSystem* ParticleSystem) {
		FEditorTabSettings Settings{ "Particle Editor", "ParticleEditorDock", true, true };
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
	const FString PanelSuffix{ "##ParticleEditor:" + (mParticleSystem ? mParticleSystem->GetPath() : FString{}) };
	mEmitterPanel = AddPanel<FParticleEmitterPanel>(mParticleSystem, "Emitters" + PanelSuffix, [this](UObject* Object) {
		mDetailsPanel->SetTarget(Object);
		mDetailsPanel->SetOpen(true);
	}, [this]() {
		DrawEditorToolbar();
	});
	mDetailsPanel = AddPanel<FParticleModuleDetailsPanel>("Module Details" + PanelSuffix, [this]() {
		OnPropertiesChanged();
	});
	mDetailsPanel->SetTarget(mEmitterPanel->GetSelectedObject());
	CaptureSavedProperties();

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
	if (mCloseApproved) {
		*GetOpenFlag() = false;
		return;
	}

	if (mPendingCameraSetup && ViewportPanel->GetViewport()->IsVisible()) {
		InitializePreviewCamera();
	}

	if (mPreviewComponent) {
		if (mPendingPreviewRestart) {
			RestartPreview();
		}

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

void FParticleEditorTab::BuildDefaultLayout(ImGuiID DockSpaceRoot) {
	ImGuiID EmitterNode{ DockSpaceRoot };
	ImGuiID PreviewNode{ ImGui::DockBuilderSplitNode(EmitterNode, ImGuiDir_Left, 0.35f, nullptr, &EmitterNode) };
	const ImGuiID DetailsNode{ ImGui::DockBuilderSplitNode(PreviewNode, ImGuiDir_Down, 0.48f, nullptr, &PreviewNode) };
	ImGui::DockBuilderDockWindow(ViewportPanel->GetPanelName(), PreviewNode);
	ImGui::DockBuilderDockWindow(mDetailsPanel->GetPanelName(), DetailsNode);
	ImGui::DockBuilderDockWindow(mEmitterPanel->GetPanelName(), EmitterNode);
}

void FParticleEditorTab::RestartPreview() {
	mPreviewComponent->EndPlay();
	mPreviewComponent->BeginPlay();
	mPreviewComponent->UpdateDynamicData();
	mPendingPreviewRestart = false;
}

void FParticleEditorTab::OnPropertiesChanged() {
	mDirty = true;
	mPendingPreviewRestart = true;
	mStatus.clear();
}

void FParticleEditorTab::DrawEditorToolbar() {
	ImGui::BeginDisabled(!mParticleSystem || GetEditingFilePath().empty());
	if (ImGui::Button("Save")) {
		SaveAsset();
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(mPreviewComponent == nullptr);
	if (ImGui::Button("Restart")) {
		mPendingPreviewRestart = true;
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::TextDisabled("%s", mDirty ? "Unsaved changes" : "Saved");
	if (!mStatus.empty()) {
		ImGui::TextWrapped("%s", mStatus.c_str());
	}
	DrawCloseConfirmation();
}

bool FParticleEditorTab::SaveAsset() {
	if (!mParticleSystem || GetEditingFilePath().empty()) {
		return false;
	}

	const std::filesystem::path Destination{ PathUtils::FromUtf8(GetEditingFilePath()) };
	std::filesystem::path Temporary{ Destination };
	Temporary += L".tmp";
	std::error_code Error{};
	for (int32 Index{ 1 }; std::filesystem::exists(Temporary, Error); ++Index) {
		Temporary = Destination.wstring() + L".tmp" + std::to_wstring(Index);
	}
	if (Error) {
		mStatus = "Could not prepare the particle system file for saving.";
		return false;
	}
	if (!FJsonArchive::SaveParticleSystem(mParticleSystem, PathUtils::ToUtf8(Temporary)) || !MoveFileExW(Temporary.c_str(), Destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
		std::filesystem::remove(Temporary, Error);
		mStatus = "Could not save the particle system.";
		return false;
	}

	CaptureSavedProperties();
	mDirty = false;
	mStatus = "Particle system saved.";
	return true;
}

bool FParticleEditorTab::ConfirmClose() {
	if (!mDirty || mCloseApproved) {
		return true;
	}

	mEmitterPanel->SetOpen(true);
	mCloseRequested = true;
	return false;
}

void FParticleEditorTab::DrawCloseConfirmation() {
	if (mCloseRequested) {
		ImGui::OpenPopup("Unsaved Particle System");
		mCloseRequested = false;
	}
	if (ImGui::BeginPopupModal("Unsaved Particle System", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::TextUnformatted("Save changes before closing?");
		if (ImGui::Button("Save and Close") && SaveAsset()) {
			mCloseApproved = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard")) {
			RestoreSavedProperties();
			mCloseApproved = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
		}
		if (!mStatus.empty()) {
			ImGui::TextWrapped("%s", mStatus.c_str());
		}
		ImGui::EndPopup();
	}
}

void FParticleEditorTab::CaptureObjectProperties(UObject* Object) {
	if (!Object) {
		return;
	}

	for (UClass* Class{ Object->GetClass() }; Class; Class = Class->Super) {
		for (const FProperty& Property : Class->GetProperties()) {
			if (!Property.HasAnyFlags(CPF_Edit) || Property.HasAnyFlags(CPF_EditConst) || Property.Type == EPropertyType::String || Property.Type == EPropertyType::Unknown || Property.Size > static_cast<std::size_t>(std::numeric_limits<int32>::max())) {
				continue;
			}

			FPropertySnapshot Snapshot{};
			Snapshot.mObject = Object;
			Snapshot.mOffset = Property.Offset;
			Snapshot.mValue.SetNum(static_cast<int32>(Property.Size));
			std::memcpy(Snapshot.mValue.GetData(), reinterpret_cast<const uint8*>(Object) + Property.Offset, Property.Size);
			mSavedProperties.Add(std::move(Snapshot));
		}
	}
}

void FParticleEditorTab::CaptureSavedProperties() {
	mSavedProperties.Empty();
	if (!mParticleSystem) {
		return;
	}

	for (UParticleEmitter* Emitter : mParticleSystem->Emitters) {
		if (!Emitter) {
			continue;
		}
		CaptureObjectProperties(Emitter);
		for (UParticleLODLevel* LOD : Emitter->LODLevels) {
			if (!LOD) {
				continue;
			}
			CaptureObjectProperties(LOD->RequiredModule);
			CaptureObjectProperties(LOD->SpawnModule);
			for (UParticleModule* Module : LOD->Modules) {
				CaptureObjectProperties(Module);
			}
		}
	}
}

void FParticleEditorTab::RestoreSavedProperties() {
	for (const FPropertySnapshot& Snapshot : mSavedProperties) {
		std::memcpy(reinterpret_cast<uint8*>(Snapshot.mObject) + Snapshot.mOffset, Snapshot.mValue.GetData(), static_cast<std::size_t>(Snapshot.mValue.Num()));
	}
	mDirty = false;
}
