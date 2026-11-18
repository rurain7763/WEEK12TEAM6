#include "EnginePCH.h"
#include "ParticleModuleDetailsPanel.h"
#include "Editor/Details/DetailsPanel.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"

namespace {
	// 리플렉션 미지원 enum이라 직접 콤보로 그림
	bool DrawCollisionOptions(UParticleModuleCollision* Collision) {
		const char* Items[] = { "Kill", "Freeze" };
		int Index{ static_cast<int>(Collision->CompletionOption) };
		if (ImGui::Combo("Completion Option", &Index, Items, IM_ARRAYSIZE(Items))) {
			Collision->CompletionOption = static_cast<EParticleCollisionComplete>(Index);
			return true;
		}
		return false;
	}

	// 리플렉션 미지원 배열이라 규칙마다 직접 그림
	bool DrawEventGeneratorRules(UParticleModuleEventGenerator* Generator) {
		bool bChanged{ false };
		int32 RemoveIndex{ -1 };

		ImGui::SeparatorText("Events");
		for (int32 i = 0; i < Generator->Events.Num(); ++i) {
			FParticleEventGeneratorInfo& Info{ Generator->Events[i] };
			ImGui::PushID(i);

			char Buffer[256]{};
			strncpy_s(Buffer, Info.EventName.c_str(), sizeof(Buffer) - 1);
			if (ImGui::InputText("Event Name", Buffer, sizeof(Buffer))) {
				Info.EventName = Buffer;
				bChanged = true;
			}
			if (ImGui::DragInt("Frequency", &Info.Frequency, 1.0f, 1, 1000)) {
				bChanged = true;
			}
			bChanged |= ImGui::Checkbox("First Time Only", &Info.bFirstTimeOnly);
			bChanged |= ImGui::Checkbox("Last Time Only", &Info.bLastTimeOnly);

			// 순회 중 삭제하면 인덱스가 밀리므로 루프 후 삭제
			if (ImGui::Button("Remove")) {
				RemoveIndex = i;
			}
			ImGui::Separator();
			ImGui::PopID();
		}

		if (RemoveIndex >= 0) {
			Generator->Events.RemoveAt(RemoveIndex);
			bChanged = true;
		}
		if (ImGui::Button("Add Rule")) {
			Generator->Events.Add(FParticleEventGeneratorInfo{});
			bChanged = true;
		}
		return bChanged;
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

void FParticleModuleDetailsPanel::SetTarget(UObject* Object) {
	mTarget = Object;
}

void FParticleModuleDetailsPanel::OnRender() {
	if (ImGui::Begin(mPanelName.c_str())) {
		if (!mTarget) {
			ImGui::TextDisabled("Select an emitter or module to edit its properties.");
		} else {
			ImGui::TextUnformatted(mTarget->GetClass()->Name.c_str());
			ImGui::Separator();
			bool bChanged{ FDetailsPanel::DrawObjectProperties(mTarget) };
			if (UParticleModuleCollision* Collision{ Cast<UParticleModuleCollision>(mTarget) }) {
				bChanged |= DrawCollisionOptions(Collision);
			} else if (UParticleModuleEventGenerator* Generator{ Cast<UParticleModuleEventGenerator>(mTarget) }) {
				bChanged |= DrawEventGeneratorRules(Generator);
			}
			if (bChanged && mChangeCallback) {
				mChangeCallback();
			}
		}
	}
	ImGui::End();
}
