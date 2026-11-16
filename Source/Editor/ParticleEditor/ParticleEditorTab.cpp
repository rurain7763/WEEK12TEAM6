#include "EnginePCH.h"
#include "ParticleEditorTab.h"
#include "ParticleEmitterPanel.h"
#include "ParticleModuleDetailsPanel.h"
#include "ParticleSystemEditSession.h"
#include "Component/ParticleSystemComponent.h"
#include "Core/PathUtils.h"
#include "Editor/Preview/PreviewViewportPanel.h"
#include "Editor/Viewports/EditorViewportClient.h"
#include "Editor/Viewports/ViewportCameraMath.h"
#include "Engine/World.h"
#include "GameFramework/Actor/ParticleActor.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleModule.h"

#include <algorithm>

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
	  mParticleSystem{ InParticleSystem },
	  mEditSession{ MakeUnique<FParticleSystemEditSession>(InParticleSystem) } {
	ViewportPanel->GetViewport()->SetGridVisible(false);
	UParticleSystem* WorkingCopy{ mEditSession->GetWorkingCopy() };
	const FString PanelSuffix{ "##ParticleEditor:" + (mParticleSystem ? mParticleSystem->GetPath() : FString{}) };
	mEmitterPanel = AddPanel<FParticleEmitterPanel>(WorkingCopy, "Emitters" + PanelSuffix, [this](UObject* Object) {
		mDetailsPanel->SetTarget(Object);
		mDetailsPanel->SetOpen(true);
	}, [this]() {
		DrawEditorToolbar();
	}, [this](const FParticleEmitterPanel::FStructureRequest& Request) {
		mPendingStructureRequest = Request;
		mHasPendingStructureRequest = true;
	});
	mDetailsPanel = AddPanel<FParticleModuleDetailsPanel>("Module Details" + PanelSuffix, [this]() {
		OnPropertiesChanged();
	});
	mDetailsPanel->SetTarget(mEmitterPanel->GetSelectedObject());
	mStatus = mEditSession->GetError();

	if (!WorkingCopy) {
		return;
	}

	AActor* PreviewActor{ GetWorld()->SpawnActor(AParticleActor::StaticClass()) };
	if (!PreviewActor) {
		return;
	}

	mPreviewComponent = Cast<UParticleSystemComponent>(PreviewActor->FindComponentByClass(UParticleSystemComponent::StaticClass()));
	if (mPreviewComponent) {
		mPreviewComponent->SetWorldLocation(FVector::ZeroVector, false);
		mPreviewComponent->SetTemplate(WorkingCopy);
		mPreviewComponent->BeginPlay();
	}
}

FParticleEditorTab::~FParticleEditorTab() {
	if (mPreviewComponent) {
		mPreviewComponent->EndPlay();
		mPreviewComponent->SetTemplate(nullptr);
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
	ApplyPendingStructureChange();

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

void FParticleEditorTab::ApplyPendingStructureChange() {
	if (!mHasPendingStructureRequest) {
		return;
	}
	mHasPendingStructureRequest = false;
	UParticleSystem* Working{ mEditSession->GetWorkingCopy() };
	UObject* PreviousSelection{ mEmitterPanel->GetSelectedObject() };
	mEmitterPanel->SelectObject(nullptr);
	if (mPreviewComponent) {
		mPreviewComponent->EndPlay();
		mPendingPreviewRestart = true;
	}

	UObject* Selection{ PreviousSelection };
	bool Changed{};
	try {
		switch (mPendingStructureRequest.mAction) {
		case FParticleEmitterPanel::EStructureAction::AddEmitter:
			Selection = mEditSession->AddEmitter(mStatus);
			Changed = Selection != nullptr;
			break;
		case FParticleEmitterPanel::EStructureAction::DeleteEmitter: {
			const int32 Index{ Working ? Working->Emitters.Find(mPendingStructureRequest.mEmitter) : -1 };
			Changed = mEditSession->DeleteEmitter(mPendingStructureRequest.mEmitter, mStatus);
			if (Changed) {
				Selection = Working->Emitters.IsEmpty() ? nullptr : Working->Emitters[static_cast<uint32>(std::min(Index, Working->Emitters.Num() - 1))];
			}
			break;
		}
		case FParticleEmitterPanel::EStructureAction::AddModule:
			Selection = mEditSession->AddModule(mPendingStructureRequest.mEmitter, mPendingStructureRequest.mModuleClass, mStatus);
			Changed = Selection != nullptr;
			break;
		case FParticleEmitterPanel::EStructureAction::DeleteModule:
			Changed = mEditSession->DeleteModule(mPendingStructureRequest.mEmitter, mPendingStructureRequest.mModule, mStatus);
			if (Changed) {
				Selection = mPendingStructureRequest.mEmitter;
			}
			break;
		}
	} catch (const std::exception& Error) {
		mStatus = "Could not update the particle system: " + FString{ Error.what() };
	}

	mPendingStructureRequest = {};
	mEmitterPanel->SelectObject(Changed ? Selection : PreviousSelection);
	if (Changed) {
		OnPropertiesChanged();
	}
}

void FParticleEditorTab::DrawEditorToolbar() {
	ImGui::BeginDisabled(!mEditSession->GetWorkingCopy() || GetEditingFilePath().empty());
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
	if (!mEditSession->Save(mStatus)) {
		return false;
	}

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
