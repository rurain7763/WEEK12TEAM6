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
#include "Particle/ParticleLODLevel.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"

#include <algorithm>
#include <cmath>

namespace {
	constexpr float ParticlePreviewCameraDistance{ 10.0f };
	constexpr float ParticlePreviewMotionAngularSpeed{ 1.0f };

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
	: FPreviewEditorTab{ MakeParticleEditorTabSettings(InParticleSystem), InParticleSystem ? "Particle Preview##ParticleEditor:" + InParticleSystem->GetPath() : "Particle Preview##ParticleEditor", false },
	  mParticleSystem{ InParticleSystem },
	  mEditSession{ MakeUnique<FParticleSystemEditSession>(InParticleSystem) } {
	ViewportPanel->GetViewport()->SetGridVisible(false);
	ViewportPanel->SetGridControlVisible(false);
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
	}, [this]() {
		mDirty = true;
		mStatus.clear();
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
		mPreviewComponent->SetLODMethod(EParticleSystemLODMethod::DirectSet);
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

		const float PreviewDelta{ std::isfinite(DeltaTime) ? std::max(0.0f, DeltaTime) : 0.0f };
		UpdatePreviewMotion(PreviewDelta);
		mPreviewComponent->TickComponent(PreviewDelta);
		mPreviewTime += PreviewDelta;
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

void FParticleEditorTab::UpdatePreviewMotion(float DeltaTime) {
	FVector Location{ FVector::ZeroVector };
	if (mMotionEnabled) {
		const float ElapsedTime{ std::isfinite(DeltaTime) ? std::max(0.0f, DeltaTime) : 0.0f };
		mMotionAngle = std::fmod(mMotionAngle + ElapsedTime * ParticlePreviewMotionAngularSpeed, 2.0f * ViewportCameraMath::Pi);
		Location = FVector{ std::cos(mMotionAngle) * mMotionRadius, std::sin(mMotionAngle) * mMotionRadius, 0.0f };
	}
	mPreviewComponent->SetWorldLocation(Location, false);
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
	mMotionAngle = 0.0f;
	mPreviewTime = 0.0;
	UpdatePreviewMotion(0.0f);
	mPreviewComponent->SetLODLevel(mEditSession->GetSelectedLODIndex());
	mPreviewComponent->BeginPlay();
	mPreviewComponent->UpdateDynamicData();
	mPendingPreviewRestart = false;
}

void FParticleEditorTab::DrawPreviewToolbar() {
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ImGui::GetFrameHeight() * 0.5f);
	if (ImGui::Button("View")) {
		ImGui::OpenPopup("ParticlePreviewView");
	}
	ImGui::SameLine();
	if (ImGui::Button("Time")) {
		ImGui::OpenPopup("ParticlePreviewTime");
	}
	ImGui::PopStyleVar();

	if (ImGui::BeginPopup("ParticlePreviewView", ImGuiWindowFlags_NoMove)) {
		bool GridVisible{ ViewportPanel->GetViewport()->IsGridVisible() };
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Checkbox("Grid", &GridVisible)) {
			ViewportPanel->GetViewport()->SetGridVisible(GridVisible);
		}
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::TextUnformatted(GridVisible ? "On" : "Off");
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("ParticlePreviewTime", ImGuiWindowFlags_NoMove)) {
		ImGui::BeginDisabled(mPreviewComponent == nullptr);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		if (ImGui::Checkbox("Motion", &mMotionEnabled)) {
			mPendingPreviewRestart = true;
		}
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::TextUnformatted(mMotionEnabled ? "On" : "Off");
		ImGui::SetNextItemWidth(100.0f);
		if (ImGui::DragFloat("Motion Radius", &mMotionRadius, 0.05f, 0.0f, 1000.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp)) {
			mMotionRadius = std::isfinite(mMotionRadius) ? std::clamp(mMotionRadius, 0.0f, 1000.0f) : 0.0f;
			mPendingPreviewRestart = true;
		}
		ImGui::Separator();
		ImGui::Text("Time: %.2f s", mPreviewTime);
		ImGui::EndDisabled();
		ImGui::EndPopup();
	}
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
	if (mPendingStructureRequest.mAction == FParticleEmitterPanel::EStructureAction::SelectLOD) {
		if (mEditSession->SelectLOD(mPendingStructureRequest.mTargetIndex)) {
			mEmitterPanel->SetSelectedLODIndex(mEditSession->GetSelectedLODIndex());
			mPendingPreviewRestart = true;
		}
		mPendingStructureRequest = {};
		return;
	}

	UParticleSystem* Working{ mEditSession->GetWorkingCopy() };
	UObject* PreviousSelection{ mEmitterPanel->GetSelectedObject() };
	UParticleEmitter* PreviousEmitter{ mEmitterPanel->GetSelectedEmitter() };
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
		case FParticleEmitterPanel::EStructureAction::MoveEmitter:
			Changed = mEditSession->MoveEmitter(mPendingStructureRequest.mEmitter, mPendingStructureRequest.mTargetIndex, mStatus);
			if (Changed) {
				Selection = mPendingStructureRequest.mEmitter;
			}
			break;
		case FParticleEmitterPanel::EStructureAction::SetTypeData:
			Changed = mEditSession->SetEmitterTypeData(mPendingStructureRequest.mEmitter, mPendingStructureRequest.mModuleClass, mStatus);
			if (Changed) {
				UParticleLODLevel* LOD{ mPendingStructureRequest.mEmitter->GetLODLevel(mEditSession->GetSelectedLODIndex()) };
				Selection = LOD->TypeDataModule ? static_cast<UObject*>(LOD->TypeDataModule) : mPendingStructureRequest.mEmitter;
			}
			break;
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
		case FParticleEmitterPanel::EStructureAction::MoveModule:
			Changed = mEditSession->MoveModule(mPendingStructureRequest.mEmitter, mPendingStructureRequest.mModule, mPendingStructureRequest.mTargetIndex, mStatus);
			if (Changed) {
				Selection = mPendingStructureRequest.mModule;
			}
			break;
		case FParticleEmitterPanel::EStructureAction::AddLOD:
			Changed = mEditSession->AddLOD(mPendingStructureRequest.mTargetIndex, mStatus);
			Selection = PreviousEmitter ? static_cast<UObject*>(PreviousEmitter) : Working;
			break;
		case FParticleEmitterPanel::EStructureAction::DeleteLOD:
			Changed = mEditSession->DeleteLOD(mStatus);
			Selection = PreviousEmitter ? static_cast<UObject*>(PreviousEmitter) : Working;
			break;
		case FParticleEmitterPanel::EStructureAction::RegenerateLODs:
			Changed = mEditSession->RegenerateLODs(mStatus);
			Selection = PreviousEmitter ? static_cast<UObject*>(PreviousEmitter) : Working;
			break;
		case FParticleEmitterPanel::EStructureAction::SelectLOD:
			break;
		}
	} catch (const std::exception& Error) {
		mStatus = "Could not update the particle system: " + FString{ Error.what() };
	}

	mPendingStructureRequest = {};
	mEmitterPanel->SetSelectedLODIndex(mEditSession->GetSelectedLODIndex());
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
