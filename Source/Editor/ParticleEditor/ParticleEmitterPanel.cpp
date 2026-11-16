#include "EnginePCH.h"
#include "ParticleEmitterPanel.h"
#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleSpawn.h"

#include <algorithm>
#include <cctype>

namespace {
	FString GetModuleLabel(const UParticleModule* Module) {
		const FString& ClassName{ Module->GetClass()->Name };
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

FParticleEmitterPanel::FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback)
	: mParticleSystem{ ParticleSystem },
	  mPanelName{ std::move(PanelName) },
	  mSelectionCallback{ std::move(SelectionCallback) },
	  mToolbarCallback{ std::move(ToolbarCallback) } {
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

void FParticleEmitterPanel::DrawModule(UParticleModule* Module, const char* Label, const ImVec4& Color) {
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
		ImGui::TextDisabled("LOD 0");
		ImGui::Separator();

		UParticleLODLevel* LOD{ Emitter ? Emitter->GetLODLevel(0) : nullptr };
		if (LOD) {
			DrawModule(LOD->RequiredModule, "Required", ImVec4{ 0.45f, 0.24f, 0.10f, 1.0f });
			DrawModule(LOD->SpawnModule, "Spawn", ImVec4{ 0.15f, 0.34f, 0.17f, 1.0f });
			ImGui::Separator();
			for (int32 ModuleIndex{}; ModuleIndex < LOD->Modules.Num(); ++ModuleIndex) {
				UParticleModule* Module{ LOD->Modules[ModuleIndex] };
				const FString ModuleLabel{ Module ? GetModuleLabel(Module) : "Missing module" };
				ImGui::PushID(ModuleIndex);
				DrawModule(Module, ModuleLabel.c_str(), ImVec4{ 0.16f, 0.25f, 0.37f, 1.0f });
				ImGui::PopID();
			}
		} else {
			ImGui::TextDisabled("No LOD 0");
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
		if (!mParticleSystem || mParticleSystem->Emitters.IsEmpty()) {
			ImGui::TextDisabled("This particle system has no emitters.");
		} else {
			if (ImGui::BeginChild("EmitterCanvas", ImVec2{}, ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar)) {
				const float Height{ std::max(120.0f, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ScrollbarSize) };
				for (int32 Index{}; Index < mParticleSystem->Emitters.Num(); ++Index) {
					if (Index > 0) {
						ImGui::SameLine();
					}
					DrawEmitter(Index, Height);
				}
			}
			ImGui::EndChild();
		}
	}
	ImGui::End();
}
