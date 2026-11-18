#include "EnginePCH.h"
#include "ParticleModuleDetailsPanel.h"
#include "Editor/Details/DetailsPanel.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleSystem.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Particle/Modules/ParticleModuleRequired.h"

#include <cmath>
#include <limits>

namespace {
	// 리플렉션 미지원 enum이라 직접 콤보로 그림
	bool DrawCollisionOptions(UParticleModuleCollision* Collision) {
		const char* Items[]{ "Kill", "Freeze" };
		int Index{ static_cast<int>(Collision->CompletionOption) };
		if (ImGui::Combo("Completion Option", &Index, Items, IM_ARRAYSIZE(Items))) {
			Collision->CompletionOption = static_cast<EParticleCollisionComplete>(Index);
			return true;
		}
		return false;
	}

	// 리플렉션 미지원 배열이라 규칙마다 직접 그림
	bool DrawEventGeneratorRules(UParticleModuleEventGenerator* Generator) {
		bool Changed{};
		int32 RemoveIndex{ -1 };

		ImGui::SeparatorText("Events");
		for (int32 Index{}; Index < Generator->Events.Num(); ++Index) {
			FParticleEventGeneratorInfo& Info{ Generator->Events[Index] };
			ImGui::PushID(Index);

			char Buffer[256]{};
			strncpy_s(Buffer, Info.EventName.c_str(), sizeof(Buffer) - 1);
			if (ImGui::InputText("Event Name", Buffer, sizeof(Buffer))) {
				Info.EventName = Buffer;
				Changed = true;
			}
			if (ImGui::DragInt("Frequency", &Info.Frequency, 1.0f, 1, 1000)) {
				Changed = true;
			}
			Changed |= ImGui::Checkbox("First Time Only", &Info.bFirstTimeOnly);
			Changed |= ImGui::Checkbox("Last Time Only", &Info.bLastTimeOnly);

			// 순회 중 삭제하면 인덱스가 밀리므로 루프 후 삭제
			if (ImGui::Button("Remove")) {
				RemoveIndex = Index;
			}
			ImGui::Separator();
			ImGui::PopID();
		}

		if (RemoveIndex >= 0) {
			Generator->Events.RemoveAt(RemoveIndex);
			Changed = true;
		}
		if (ImGui::Button("Add Rule")) {
			Generator->Events.Add(FParticleEventGeneratorInfo{});
			Changed = true;
		}
		return Changed;
	}
}

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

void FParticleModuleDetailsPanel::SetTarget(UObject* Object, int32 LODIndex) {
	mTarget = Object;
	mLODIndex = LODIndex;
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
			bool MaterialChanged{};
			if (UParticleSystem* ParticleSystem{ Cast<UParticleSystem>(mTarget) }) {
				Changed |= DrawParticleSystemProperties(ParticleSystem);
			} else if (UParticleModuleCollision* Collision{ Cast<UParticleModuleCollision>(mTarget) }) {
				Changed |= DrawCollisionOptions(Collision);
			} else if (UParticleModuleEventGenerator* Generator{ Cast<UParticleModuleEventGenerator>(mTarget) }) {
				Changed |= DrawEventGeneratorRules(Generator);
			} else if (UParticleModuleRequired* Required{ Cast<UParticleModuleRequired>(mTarget) }) {
				MaterialChanged = FDetailsPanel::DrawParticleMaterialProperties(Required, false);
			} else if (UParticleEmitter* Emitter{ Cast<UParticleEmitter>(mTarget) }) {
				if (UParticleLODLevel* LOD{ Emitter->GetLODLevel(mLODIndex) }; LOD && LOD->RequiredModule) {
					MaterialChanged = FDetailsPanel::DrawParticleMaterialProperties(LOD->RequiredModule, true);
				}
			}
			if ((Changed || MaterialChanged) && mChangeCallback) {
				mChangeCallback(Changed);
			}
		}
	}
	ImGui::End();
}
