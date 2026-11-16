#include "EnginePCH.h"
#include "ParticleModuleDetailsPanel.h"
#include "Editor/Details/DetailsPanel.h"

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
			if (FDetailsPanel::DrawObjectProperties(mTarget) && mChangeCallback) {
				mChangeCallback();
			}
		}
	}
	ImGui::End();
}
