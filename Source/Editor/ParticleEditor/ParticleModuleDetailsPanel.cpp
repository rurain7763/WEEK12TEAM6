#include "EnginePCH.h"
#include "ParticleModuleDetailsPanel.h"
#include "Editor/Details/DetailsPanel.h"
#include "Particle/ParticleSystem.h"

#include <cmath>
#include <limits>

FParticleModuleDetailsPanel::FParticleModuleDetailsPanel(FString PanelName, FChangeCallback ChangeCallback)
	: mPanelName{ std::move(PanelName) },
	  mChangeCallback{ std::move(ChangeCallback) } {
}

bool FParticleModuleDetailsPanel::Init() {
	return true;
}

void FParticleModuleDetailsPanel::Tick(float) {
}

const char* FParticleModuleDetailsPanel::GetPanelName() const {
	return mPanelName.c_str();
}

void FParticleModuleDetailsPanel::SetTarget(UObject* Object) {
	mTarget = Object;
}

bool FParticleModuleDetailsPanel::DrawParticleSystemProperties(UParticleSystem* ParticleSystem) {
	ImGui::Text("Emitter Count: %d", ParticleSystem->Emitters.Num());
	ImGui::Text("LOD Count: %d", ParticleSystem->LODDistances.Num());

	bool Changed{};
	int32 Method{ static_cast<int32>(ParticleSystem->LODMethod) };
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::Combo("LOD Method", &Method, "Automatic\0Direct Set\0")) {
		ParticleSystem->LODMethod = static_cast<EParticleSystemLODMethod>(Method);
		Changed = true;
	}

	float CheckTime{ ParticleSystem->LODDistanceCheckTime };
	ImGui::SetNextItemWidth(100.0f);
	if (ImGui::DragFloat("LOD Check Time (s)", &CheckTime, 0.01f, 0.0f, std::numeric_limits<float>::max(), "%.2f", ImGuiSliderFlags_AlwaysClamp) && std::isfinite(CheckTime) && CheckTime != ParticleSystem->LODDistanceCheckTime) {
		ParticleSystem->LODDistanceCheckTime = CheckTime;
		Changed = true;
	}

	for (int32 Index{}; Index < ParticleSystem->LODDistances.Num(); ++Index) {
		const FString Label{ "LOD " + std::to_string(Index) + " Distance" };
		float Distance{ ParticleSystem->LODDistances[Index] };
		ImGui::BeginDisabled(Index == 0);
		ImGui::SetNextItemWidth(100.0f);
		const bool Edited{ ImGui::DragFloat(Label.c_str(), &Distance, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f", ImGuiSliderFlags_AlwaysClamp) };
		ImGui::EndDisabled();
		if (Edited && Index > 0 && std::isfinite(Distance) && Distance > ParticleSystem->LODDistances[Index - 1] && (Index + 1 == ParticleSystem->LODDistances.Num() || Distance < ParticleSystem->LODDistances[Index + 1]) && Distance != ParticleSystem->LODDistances[Index]) {
			ParticleSystem->LODDistances[Index] = Distance;
			Changed = true;
		}
	}
	return Changed;
}

void FParticleModuleDetailsPanel::OnRender() {
	if (ImGui::Begin(mPanelName.c_str())) {
		if (!mTarget) {
			ImGui::TextDisabled("Select an emitter or module, or click the background to edit the particle system.");
		} else {
			ImGui::TextUnformatted(mTarget->GetClass()->Name.c_str());
			ImGui::Separator();
			bool Changed{ FDetailsPanel::DrawObjectProperties(mTarget) };
			if (UParticleSystem* ParticleSystem{ Cast<UParticleSystem>(mTarget) }) {
				Changed |= DrawParticleSystemProperties(ParticleSystem);
			}
			if (Changed && mChangeCallback) {
				mChangeCallback();
			}
		}
	}
	ImGui::End();
}
