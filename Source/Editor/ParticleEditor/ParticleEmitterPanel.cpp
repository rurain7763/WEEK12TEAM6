#include "EnginePCH.h"
#include "ParticleEmitterPanel.h"
#include "ParticleSystemEditSession.h"
#include "Editor/HitoriEd/EditorDragDrop.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleSpawn.h"
#include "Particle/Modules/ParticleModuleTypeDataBase.h"
#include "Particle/Modules/ParticleModuleTypeDataBeam.h"
#include "Particle/Modules/ParticleModuleTypeDataRibbon.h"
#include "Particle/Modules/ParticleModuleTypeDataMesh.h"

#include <algorithm>
#include <cctype>

namespace {
	FString GetModuleLabel(const UClass* Class) {
		const FString& ClassName{ Class->Name };
		const FString Name{ ClassName.starts_with("UParticleModule") ? ClassName.substr(15) : ClassName };
		FString Label{};
		for (std::size_t Index{}; Index < Name.size(); ++Index) {
			if (Index > 0 && std::isupper(static_cast<unsigned char>(Name[Index])) && std::islower(static_cast<unsigned char>(Name[Index - 1]))) {
				Label += ' ';
			}
			Label += Name[Index];
		}
		return Label;
	}
}

FParticleEmitterPanel::FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback, FStructureCallback StructureCallback, FChangeCallback ChangeCallback)
	: mParticleSystem{ ParticleSystem },
	  mPanelName{ std::move(PanelName) },
	  mSelectionCallback{ std::move(SelectionCallback) },
	  mToolbarCallback{ std::move(ToolbarCallback) },
	  mStructureCallback{ std::move(StructureCallback) },
	  mChangeCallback{ std::move(ChangeCallback) } {
	mSelectedObject = mParticleSystem;
	if (mParticleSystem && !mParticleSystem->Emitters.IsEmpty()) {
		mSelectedObject = mParticleSystem->Emitters[0];
	}
}

bool FParticleEmitterPanel::Init() {
	return true;
}

void FParticleEmitterPanel::Tick(float) {
}

const char* FParticleEmitterPanel::GetPanelName() const {
	return mPanelName.c_str();
}

UObject* FParticleEmitterPanel::GetSelectedObject() const {
	return mSelectedObject;
}

UParticleEmitter* FParticleEmitterPanel::GetSelectedEmitter() const {
	if (!mParticleSystem || !mSelectedObject) {
		return nullptr;
	}

	for (UParticleEmitter* Emitter : mParticleSystem->Emitters) {
		if (mSelectedObject == Emitter) {
			return Emitter;
		}
		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(mSelectedLODIndex) : nullptr };
		if (LOD && (mSelectedObject == LOD->RequiredModule || mSelectedObject == LOD->SpawnModule || mSelectedObject == LOD->TypeDataModule || LOD->Modules.Find(Cast<UParticleModule>(mSelectedObject)) >= 0)) {
			return Emitter;
		}
	}
	return nullptr;
}

void FParticleEmitterPanel::SelectObject(UObject* Object) {
	mSelectedObject = Object;
	if (mSelectionCallback) {
		mSelectionCallback(Object);
	}
}

void FParticleEmitterPanel::SetSelectedLODIndex(int32 LODIndex) {
	if (!mParticleSystem || !mParticleSystem->LODDistances.IsValidIndex(LODIndex) || LODIndex == mSelectedLODIndex) {
		return;
	}

	UParticleEmitter* Emitter{ GetSelectedEmitter() };
	mSelectedLODIndex = LODIndex;
	if (Cast<UParticleModule>(mSelectedObject)) {
		SelectObject(Emitter ? static_cast<UObject*>(Emitter) : mParticleSystem);
	}
}

void FParticleEmitterPanel::DrawLODButton(const char* Label, const char* Tooltip, EStructureAction Action, int32 TargetIndex, bool Enabled) {
	const bool HigherLOD{ TargetIndex <= mSelectedLODIndex };
	const bool DrawArrow{ Action == EStructureAction::AddLOD && !ImGui::GetFont()->IsGlyphInFont(HigherLOD ? 0x25B2 : 0x25BC) };
	ImGui::PushID(Label);
	ImGui::BeginDisabled(!Enabled);
	if (ImGui::Button(DrawArrow ? "  Add LOD" : Label)) {
		mStructureCallback(FStructureRequest{ Action, nullptr, nullptr, nullptr, TargetIndex });
	}
	if (DrawArrow) {
		const ImVec2 Minimum{ ImGui::GetItemRectMin() };
		const float Radius{ ImGui::GetFontSize() * 0.3f };
		const float Direction{ HigherLOD ? -1.0f : 1.0f };
		const ImVec2 Center{ Minimum.x + ImGui::GetStyle().FramePadding.x + Radius, Minimum.y + ImGui::GetFrameHeight() * 0.5f };
		ImGui::GetWindowDrawList()->AddTriangleFilled(ImVec2{ Center.x, Center.y + Direction * Radius }, ImVec2{ Center.x - Radius, Center.y - Direction * Radius }, ImVec2{ Center.x + Radius, Center.y - Direction * Radius }, ImGui::GetColorU32(ImGuiCol_Text));
	}
	ImGui::SetItemTooltip("%s", Tooltip);
	ImGui::EndDisabled();
	ImGui::PopID();
}

void FParticleEmitterPanel::DrawLODToolbar() {
	const int32 LODCount{ mParticleSystem ? mParticleSystem->LODDistances.Num() : 0 };
	const bool CanEdit{ LODCount > 0 && static_cast<bool>(mStructureCallback) };
	const bool CanGoHigher{ CanEdit && mSelectedLODIndex > 0 };
	const bool CanGoLower{ CanEdit && mSelectedLODIndex + 1 < LODCount };
	const float Height{ ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f + ImGui::GetStyle().ScrollbarSize };
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
	if (ImGui::BeginChild("LODToolbar", ImVec2{ 0.0f, Height }, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
		DrawLODButton("Regenerate LODs", "Replace every lower LOD with an independent copy of LOD 0. LOD distances are preserved.", EStructureAction::RegenerateLODs, -1, CanEdit && LODCount > 1);
		ImGui::SameLine();
		DrawLODButton("<< Highest", "Select LOD 0 (highest detail).", EStructureAction::SelectLOD, 0, CanGoHigher);
		ImGui::SameLine();
		DrawLODButton("< Higher", "Select the previous, higher-detail LOD.", EStructureAction::SelectLOD, mSelectedLODIndex - 1, CanGoHigher);
		ImGui::SameLine();
		ImGui::BeginDisabled(!CanEdit);
		ImGui::SetNextItemWidth(100.0f);
		const FString SelectedLabel{ "LOD " + std::to_string(mSelectedLODIndex) };
		if (ImGui::BeginCombo("##SelectedLOD", SelectedLabel.c_str())) {
			for (int32 Index{}; Index < LODCount; ++Index) {
				const FString Label{ "LOD " + std::to_string(Index) };
				const bool Selected{ Index == mSelectedLODIndex };
				if (ImGui::Selectable(Label.c_str(), Selected) && !Selected) {
					mStructureCallback(FStructureRequest{ EStructureAction::SelectLOD, nullptr, nullptr, nullptr, Index });
				}
				if (Selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		DrawLODButton("Lower >", "Select the next, lower-detail LOD.", EStructureAction::SelectLOD, mSelectedLODIndex + 1, CanGoLower);
		ImGui::SameLine();
		DrawLODButton("Lowest >>", "Select the last, lowest-detail LOD.", EStructureAction::SelectLOD, LODCount - 1, CanGoLower);
		ImGui::SameLine();
		DrawLODButton("▲ Add LOD", "Copy the current LOD into a new level before it. LOD 0 remains the base level.", EStructureAction::AddLOD, mSelectedLODIndex, CanGoHigher);
		ImGui::SameLine();
		DrawLODButton("▼ Add LOD", "Copy the current LOD into a new level after it, for all emitters.", EStructureAction::AddLOD, mSelectedLODIndex + 1, CanEdit);
		ImGui::SameLine();
		DrawLODButton("Delete LOD", "Delete the current LOD from all emitters. LOD 0 cannot be deleted.", EStructureAction::DeleteLOD, -1, CanGoHigher);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void FParticleEmitterPanel::DrawModule(UParticleEmitter* Emitter, UParticleModule* Module, const char* Label, const ImVec4& Color) {
	ImGui::PushID(Module);
	const ImVec2 Min{ ImGui::GetCursorScreenPos() };
	const ImVec2 Max{ Min.x + ImGui::GetContentRegionAvail().x, Min.y + 28.0f };
	ImGui::GetWindowDrawList()->AddRectFilled(Min, Max, ImGui::ColorConvertFloat4ToU32(ImVec4{ Color.x, Color.y, Color.z, 0.55f }));

	ImGui::BeginDisabled(Module == nullptr);
	ImGui::PushStyleColor(ImGuiCol_Header, Color);
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4{ Color.x + 0.12f, Color.y + 0.12f, Color.z + 0.12f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4{ Color.x + 0.2f, Color.y + 0.2f, Color.z + 0.2f, 1.0f });
	if (ImGui::Selectable(Label, Module && mSelectedObject == Module, ImGuiSelectableFlags_None, ImVec2{ 0.0f, 28.0f })) {
		SelectObject(Module);
	}
	if (Module) {
		ImGui::SetItemTooltip("%s", Module->GetClass()->Name.c_str());
	}
	ImGui::PopStyleColor(3);
	ImGui::EndDisabled();
	DrawModuleDragDrop(Emitter, Module, Label);
	if (ImGui::BeginPopupContextItem("ModuleContext")) {
		if (mSelectedObject != Module) {
			SelectObject(Module);
		}
		DrawAddModuleMenu(Emitter);
		DrawTypeDataMenu(Emitter);
		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(mSelectedLODIndex) : nullptr };
		const bool CanDelete{ mStructureCallback && LOD && Module && LOD->Modules.Find(Module) >= 0 };
		if (ImGui::MenuItem("Delete Module", "Delete", false, CanDelete)) {
			mStructureCallback(FStructureRequest{ EStructureAction::DeleteModule, Emitter, nullptr, Module });
		}
		if (!CanDelete) {
			ImGui::SetItemTooltip("This module uses a fixed slot and cannot be deleted here.");
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
}

void FParticleEmitterPanel::DrawModuleDragDrop(UParticleEmitter* Emitter, UParticleModule* Module, const char* Label) {
	UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(mSelectedLODIndex) : nullptr };
	const int32 ModuleIndex{ LOD && Module ? LOD->Modules.Find(Module) : -1 };
	if (ModuleIndex < 0 || !mStructureCallback) {
		return;
	}
	const ImVec2 Min{ ImGui::GetItemRectMin() };
	const ImVec2 Max{ ImGui::GetItemRectMax() };
	if (ImGui::BeginDragDropSource()) {
		const FModuleDragPayload Payload{ Emitter, Module };
		ImGui::SetDragDropPayload(EditorDragDrop::ParticleModule, &Payload, sizeof(Payload));
		ImGui::TextUnformatted(Label);
		ImGui::EndDragDropSource();
	}
	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* ActivePayload{ ImGui::GetDragDropPayload() };
		if (ActivePayload && ActivePayload->IsDataType(EditorDragDrop::ParticleModule) && ActivePayload->DataSize == sizeof(FModuleDragPayload)) {
			const FModuleDragPayload& Dragged{ *static_cast<const FModuleDragPayload*>(ActivePayload->Data) };
			const int32 SourceIndex{ Dragged.mEmitter == Emitter ? LOD->Modules.Find(Dragged.mModule) : -1 };
			if (SourceIndex >= 0) {
				const bool InsertAfter{ ImGui::GetMousePos().y >= (Min.y + Max.y) * 0.5f };
				const int32 InsertionIndex{ ModuleIndex + (InsertAfter ? 1 : 0) };
				const int32 TargetIndex{ InsertionIndex - (SourceIndex < InsertionIndex ? 1 : 0) };
				if (const ImGuiPayload* Accepted{ ImGui::AcceptDragDropPayload(EditorDragDrop::ParticleModule, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect) }) {
					if (Accepted->IsPreview() && SourceIndex != TargetIndex) {
						const float LineY{ InsertAfter ? Max.y : Min.y };
						ImGui::GetWindowDrawList()->AddLine(ImVec2{ Min.x, LineY }, ImVec2{ Max.x, LineY }, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 3.0f);
					}
					if (Accepted->IsDelivery() && SourceIndex != TargetIndex) {
						mStructureCallback(FStructureRequest{ EStructureAction::MoveModule, Emitter, nullptr, Dragged.mModule, TargetIndex });
					}
				}
			}
		}
		ImGui::EndDragDropTarget();
	}
}

void FParticleEmitterPanel::DrawAddModuleMenu(UParticleEmitter* Emitter) {
	if (ImGui::BeginMenu("Add Module", Emitter && mStructureCallback)) {
		for (UClass* Class : FParticleSystemEditSession::GetAddableModuleClasses()) {
			const FString Label{ GetModuleLabel(Class) };
			if (ImGui::MenuItem(Label.c_str())) {
				mStructureCallback(FStructureRequest{ EStructureAction::AddModule, Emitter, Class });
			}
		}
		ImGui::EndMenu();
	}
}

void FParticleEmitterPanel::DrawTypeDataMenu(UParticleEmitter* Emitter) {
	if (ImGui::BeginMenu("Type Data", Emitter && mStructureCallback)) {
		DrawTypeDataMenuItem(Emitter, nullptr, "Sprite (No Type Data)");
		DrawTypeDataMenuItem(Emitter, UParticleModuleTypeDataMesh::StaticClass(), "Mesh");
		DrawTypeDataMenuItem(Emitter, UParticleModuleTypeDataBeam::StaticClass(), "Beam");
		DrawTypeDataMenuItem(Emitter, UParticleModuleTypeDataRibbon::StaticClass(), "Ribbon");
		ImGui::EndMenu();
	}
}

void FParticleEmitterPanel::DrawTypeDataMenuItem(UParticleEmitter* Emitter, UClass* ModuleClass, const char* Label) {
	UParticleLODLevel* LOD{ Emitter->GetLODLevel(mSelectedLODIndex) };
	UClass* CurrentClass{ LOD && LOD->TypeDataModule ? LOD->TypeDataModule->GetClass() : nullptr };
	const bool Selected{ CurrentClass == ModuleClass };
	if (ImGui::MenuItem(Label, nullptr, Selected, LOD && !Selected)) {
		mStructureCallback(FStructureRequest{ EStructureAction::SetTypeData, Emitter, ModuleClass });
	}
	ImGui::SetItemTooltip("Applies to all LODs of this emitter. Changing type resets Type Data properties.");
}

void FParticleEmitterPanel::DrawEmitterContextMenu(UParticleEmitter* Emitter) {
	if (mSelectedObject != Emitter) {
		SelectObject(Emitter);
	}
	DrawAddModuleMenu(Emitter);
	DrawTypeDataMenu(Emitter);
	ImGui::Separator();
	if (ImGui::MenuItem("Add Emitter", nullptr, false, mParticleSystem && mStructureCallback)) {
		mStructureCallback(FStructureRequest{ EStructureAction::AddEmitter });
	}
	if (ImGui::MenuItem("Delete Emitter", "Delete", false, Emitter && mStructureCallback)) {
		mStructureCallback(FStructureRequest{ EStructureAction::DeleteEmitter, Emitter });
	}
}

void FParticleEmitterPanel::DrawEmitter(int32 EmitterIndex, float Height) {
	UParticleEmitter* Emitter{ mParticleSystem->Emitters[EmitterIndex] };
	ImGui::PushID(Emitter);
	if (ImGui::BeginChild("Emitter", ImVec2{ 230.0f, Height }, ImGuiChildFlags_Borders)) {
		const FString Label{ Emitter ? Emitter->GetEmitterName() : "Missing emitter" };
		const float ToggleWidth{ ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x + ImGui::CalcTextSize("Off").x };
		const float HeaderWidth{ std::max(1.0f, ImGui::GetContentRegionAvail().x - ToggleWidth - ImGui::GetStyle().ItemSpacing.x) };
		ImGui::BeginDisabled(Emitter == nullptr);
		if (ImGui::Selectable(Label.c_str(), Emitter && mSelectedObject == Emitter, ImGuiSelectableFlags_None, ImVec2{ HeaderWidth, 32.0f })) {
			SelectObject(Emitter);
		}
		ImGui::EndDisabled();
		ImGui::SetItemTooltip("Drag to reorder emitters. Select and press Delete to remove.");
		DrawEmitterDragSource(Emitter, Label.c_str());
		if (ImGui::BeginPopupContextItem("EmitterContext")) {
			DrawEmitterContextMenu(Emitter);
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, (32.0f - ImGui::GetFrameHeight()) * 0.5f));
		bool Enabled{ Emitter && Emitter->IsEnabled() };
		ImGui::BeginDisabled(Emitter == nullptr);
		if (ImGui::Checkbox(Enabled ? "On###EmitterEnabled" : "Off###EmitterEnabled", &Enabled)) {
			Emitter->SetEnabled(Enabled);
			if (mChangeCallback) {
				mChangeCallback();
			}
		}
		ImGui::EndDisabled();
		ImGui::TextDisabled("LOD %d", mSelectedLODIndex);
		ImGui::Separator();

		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(mSelectedLODIndex) : nullptr };
		if (LOD) {
			if (LOD->TypeDataModule) {
				const FString TypeDataLabel{ GetModuleLabel(LOD->TypeDataModule->GetClass()) };
				DrawModule(Emitter, LOD->TypeDataModule, TypeDataLabel.c_str(), ImVec4{ 0.35f, 0.18f, 0.45f, 1.0f });
			}

			DrawModule(Emitter, LOD->RequiredModule, "Required", ImVec4{ 0.45f, 0.24f, 0.10f, 1.0f });
			DrawModule(Emitter, LOD->SpawnModule, "Spawn", ImVec4{ 0.15f, 0.34f, 0.17f, 1.0f });
			ImGui::Separator();
			for (int32 ModuleIndex{}; ModuleIndex < LOD->Modules.Num(); ++ModuleIndex) {
				UParticleModule* Module{ LOD->Modules[ModuleIndex] };
				const FString ModuleLabel{ Module ? GetModuleLabel(Module->GetClass()) : "Missing module" };
				ImGui::PushID(ModuleIndex);
				DrawModule(Emitter, Module, ModuleLabel.c_str(), ImVec4{ 0.16f, 0.25f, 0.37f, 1.0f });
				ImGui::PopID();
			}
		} else {
			ImGui::TextDisabled("No LOD %d", mSelectedLODIndex);
		}
		HandleBackgroundClick();
		if (ImGui::BeginPopupContextWindow("EmitterBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
			DrawEmitterContextMenu(Emitter);
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();
	DrawEmitterDropTarget(Emitter, EmitterIndex);
	ImGui::PopID();
}

void FParticleEmitterPanel::DrawEmitterDragSource(UParticleEmitter* Emitter, const char* Label) {
	if (!Emitter || !mStructureCallback) {
		return;
	}
	if (ImGui::BeginDragDropSource()) {
		if (mSelectedObject != Emitter) {
			SelectObject(Emitter);
		}
		const FEmitterDragPayload Payload{ mParticleSystem, Emitter };
		ImGui::SetDragDropPayload(EditorDragDrop::ParticleEmitter, &Payload, sizeof(Payload));
		ImGui::TextUnformatted(Label);
		ImGui::EndDragDropSource();
	}
}

void FParticleEmitterPanel::DrawEmitterDropTarget(UParticleEmitter* Emitter, int32 EmitterIndex) {
	if (!Emitter || !mStructureCallback || !ImGui::BeginDragDropTarget()) {
		return;
	}
	const ImGuiPayload* ActivePayload{ ImGui::GetDragDropPayload() };
	if (ActivePayload && ActivePayload->IsDataType(EditorDragDrop::ParticleEmitter) && ActivePayload->DataSize == sizeof(FEmitterDragPayload)) {
		const FEmitterDragPayload& Dragged{ *static_cast<const FEmitterDragPayload*>(ActivePayload->Data) };
		const int32 SourceIndex{ Dragged.mParticleSystem == mParticleSystem ? mParticleSystem->Emitters.Find(Dragged.mEmitter) : -1 };
		if (SourceIndex >= 0) {
			const ImVec2 Min{ ImGui::GetItemRectMin() };
			const ImVec2 Max{ ImGui::GetItemRectMax() };
			const bool InsertAfter{ ImGui::GetMousePos().x >= (Min.x + Max.x) * 0.5f };
			const int32 InsertionIndex{ EmitterIndex + (InsertAfter ? 1 : 0) };
			const int32 TargetIndex{ InsertionIndex - (SourceIndex < InsertionIndex ? 1 : 0) };
			if (const ImGuiPayload* Accepted{ ImGui::AcceptDragDropPayload(EditorDragDrop::ParticleEmitter, ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect) }) {
				if (Accepted->IsPreview() && SourceIndex != TargetIndex) {
					const float LineX{ InsertAfter ? Max.x : Min.x };
					ImGui::GetForegroundDrawList(ImGui::GetWindowViewport())->AddLine(ImVec2{ LineX, Min.y }, ImVec2{ LineX, Max.y }, ImGui::GetColorU32(ImGuiCol_DragDropTarget), 3.0f);
				}
				if (Accepted->IsDelivery() && SourceIndex != TargetIndex) {
					mStructureCallback(FStructureRequest{ EStructureAction::MoveEmitter, Dragged.mEmitter, nullptr, nullptr, TargetIndex });
				}
			}
		}
	}
	ImGui::EndDragDropTarget();
}

void FParticleEmitterPanel::HandleBackgroundClick() {
	if (mParticleSystem && mSelectedObject != mParticleSystem && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive() && !ImGui::GetDragDropPayload()) {
		SelectObject(mParticleSystem);
	}
}

void FParticleEmitterPanel::HandleDeleteShortcut() {
	if (!mParticleSystem || !mStructureCallback || !ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) || ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive() || ImGui::GetDragDropPayload() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) || !ImGui::IsKeyPressed(ImGuiKey_Delete, false)) {
		return;
	}
	if (UParticleEmitter* Emitter{ Cast<UParticleEmitter>(mSelectedObject) }) {
		if (mParticleSystem->Emitters.Find(Emitter) >= 0) {
			mStructureCallback(FStructureRequest{ EStructureAction::DeleteEmitter, Emitter });
		}
		return;
	}
	UParticleModule* Module{ Cast<UParticleModule>(mSelectedObject) };
	if (!Module) {
		return;
	}
	for (UParticleEmitter* Emitter : mParticleSystem->Emitters) {
		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(mSelectedLODIndex) : nullptr };
		if (LOD && LOD->Modules.Find(Module) >= 0) {
			mStructureCallback(FStructureRequest{ EStructureAction::DeleteModule, Emitter, nullptr, Module });
			return;
		}
	}
}

void FParticleEmitterPanel::OnRender() {
	if (ImGui::Begin(mPanelName.c_str())) {
		if (mToolbarCallback) {
			mToolbarCallback();
		}
		ImGui::Separator();
		DrawLODToolbar();
		if (!mParticleSystem) {
			ImGui::TextDisabled("No particle system is loaded.");
		} else {
			if (ImGui::BeginChild("EmitterCanvas", ImVec2{}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
				if (mParticleSystem->Emitters.IsEmpty()) {
					ImGui::TextDisabled("Right-click to add an emitter.");
				}
				const float Height{ std::max(120.0f, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ScrollbarSize) };
				for (int32 Index{}; Index < mParticleSystem->Emitters.Num(); ++Index) {
					if (Index > 0) {
						ImGui::SameLine();
					}
					DrawEmitter(Index, Height);
				}
				HandleBackgroundClick();
				if (ImGui::BeginPopupContextWindow("EmitterCanvasContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
					if (ImGui::MenuItem("Add Emitter", nullptr, false, static_cast<bool>(mStructureCallback))) {
						mStructureCallback(FStructureRequest{ EStructureAction::AddEmitter });
					}
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
		HandleDeleteShortcut();
	}
	ImGui::End();
}
