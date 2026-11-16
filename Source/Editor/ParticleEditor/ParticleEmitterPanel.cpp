#include "EnginePCH.h"
#include "ParticleEmitterPanel.h"
#include "ParticleSystemEditSession.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleSpawn.h"

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

FParticleEmitterPanel::FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback, FStructureCallback StructureCallback)
	: mParticleSystem{ ParticleSystem },
	  mPanelName{ std::move(PanelName) },
	  mSelectionCallback{ std::move(SelectionCallback) },
	  mToolbarCallback{ std::move(ToolbarCallback) },
	  mStructureCallback{ std::move(StructureCallback) } {
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

void FParticleEmitterPanel::SelectObject(UObject* Object) {
	mSelectedObject = Object;
	if (mSelectionCallback) {
		mSelectionCallback(Object);
	}
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
	if (ImGui::BeginPopupContextItem("ModuleContext")) {
		if (mSelectedObject != Module) {
			SelectObject(Module);
		}
		DrawAddModuleMenu(Emitter);
		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(0) : nullptr };
		const bool CanDelete{ mStructureCallback && LOD && Module && LOD->Modules.Find(Module) >= 0 };
		if (ImGui::MenuItem("Delete Module", nullptr, false, CanDelete)) {
			mStructureCallback(FStructureRequest{ EStructureAction::DeleteModule, Emitter, nullptr, Module });
		}
		if (!CanDelete) {
			ImGui::SetItemTooltip("Required and Spawn modules must remain in the emitter.");
		}
		ImGui::EndPopup();
	}
	ImGui::PopID();
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

void FParticleEmitterPanel::DrawEmitterContextMenu(UParticleEmitter* Emitter) {
	if (mSelectedObject != Emitter) {
		SelectObject(Emitter);
	}
	DrawAddModuleMenu(Emitter);
	ImGui::Separator();
	if (ImGui::MenuItem("Add Emitter", nullptr, false, mParticleSystem && mStructureCallback)) {
		mStructureCallback(FStructureRequest{ EStructureAction::AddEmitter });
	}
	if (ImGui::MenuItem("Delete Emitter", nullptr, false, Emitter && mStructureCallback)) {
		mStructureCallback(FStructureRequest{ EStructureAction::DeleteEmitter, Emitter });
	}
}

void FParticleEmitterPanel::DrawEmitter(int32 EmitterIndex, float Height) {
	UParticleEmitter* Emitter{ mParticleSystem->Emitters[EmitterIndex] };
	ImGui::PushID(EmitterIndex);
	if (ImGui::BeginChild("Emitter", ImVec2{ 230.0f, Height }, ImGuiChildFlags_Borders)) {
		const FString Label{ "Emitter " + std::to_string(EmitterIndex + 1) };
		ImGui::BeginDisabled(Emitter == nullptr);
		if (ImGui::Selectable(Label.c_str(), Emitter && mSelectedObject == Emitter, ImGuiSelectableFlags_None, ImVec2{ 0.0f, 32.0f })) {
			SelectObject(Emitter);
		}
		ImGui::EndDisabled();
		if (ImGui::BeginPopupContextItem("EmitterContext")) {
			DrawEmitterContextMenu(Emitter);
			ImGui::EndPopup();
		}
		ImGui::TextDisabled("LOD 0");
		ImGui::Separator();

		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(0) : nullptr };
		if (LOD) {
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
			ImGui::TextDisabled("No LOD 0");
		}
		if (ImGui::BeginPopupContextWindow("EmitterBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
			DrawEmitterContextMenu(Emitter);
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();
	ImGui::PopID();
}

void FParticleEmitterPanel::OnRender() {
	if (ImGui::Begin(mPanelName.c_str())) {
		if (mToolbarCallback) {
			mToolbarCallback();
		}
		ImGui::Separator();
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
				if (ImGui::BeginPopupContextWindow("EmitterCanvasContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup)) {
					if (ImGui::MenuItem("Add Emitter", nullptr, false, static_cast<bool>(mStructureCallback))) {
						mStructureCallback(FStructureRequest{ EStructureAction::AddEmitter });
					}
					ImGui::EndPopup();
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}
