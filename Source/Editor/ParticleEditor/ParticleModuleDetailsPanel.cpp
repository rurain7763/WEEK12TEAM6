#include "EnginePCH.h"
#include "ParticleModuleDetailsPanel.h"
#include "Editor/Details/DetailsPanel.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleSystem.h"
#include "Particle/Modules/ParticleModuleCollision.h"
#include "Particle/Modules/ParticleModuleEventGenerator.h"
#include "Particle/Modules/ParticleModuleRequired.h"
#include "Particle/Modules/ParticleModuleVelocityCone.h"

#include <cmath>
#include <limits>

namespace {
	void DrawDirectionProjection(const char* Label, float Horizontal, float Vertical, const char* Axes) {
		ImGui::BeginGroup();
		ImGui::TextUnformatted(Label);
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const ImVec2 Center(Origin.x + 48.f, Origin.y + 48.f);
		ImDrawList* Draw = ImGui::GetWindowDrawList();
		const ImU32 Grid = ImGui::GetColorU32(ImGuiCol_Border);
		const ImU32 Color = ImGui::GetColorU32(ImGuiCol_PlotHistogram);
		Draw->AddCircle(Center, 38.f, Grid);
		Draw->AddLine(ImVec2(Center.x - 38.f, Center.y), ImVec2(Center.x + 38.f, Center.y), Grid);
		Draw->AddLine(ImVec2(Center.x, Center.y - 38.f), ImVec2(Center.x, Center.y + 38.f), Grid);
		const ImVec2 Tip(Center.x + Horizontal * 38.f, Center.y - Vertical * 38.f);
		Draw->AddLine(Center, Tip, Color, 2.f);
		const float Length = std::sqrt(Horizontal * Horizontal + Vertical * Vertical);
		if (Length > 0.01f) {
			const float X = Horizontal / Length, Y = -Vertical / Length;
			Draw->AddTriangleFilled(Tip, ImVec2(Tip.x - X * 8.f - Y * 4.f, Tip.y - Y * 8.f + X * 4.f), ImVec2(Tip.x - X * 8.f + Y * 4.f, Tip.y - Y * 8.f - X * 4.f), Color);
		} else {
			Draw->AddCircleFilled(Center, 3.f, Color);
		}
		ImGui::Dummy(ImVec2(96.f, 96.f));
		ImGui::TextDisabled("%s", Axes);
		ImGui::EndGroup();
	}

	bool DrawConeDirection(UParticleModuleVelocityCone* Cone) {
		FVector Value = Cone->GetDirection();
		FVector Forward, Right, Up;
		FVector::BuildOrthonormalBasis(Value, Forward, Right, Up);
		bool Changed = false;
		ImGui::SeparatorText("Cone Direction (Emitter Local Space)");
		ImGui::TextDisabled("Yaw: +X toward +Y. Elevation: toward +Z.");
		float Yaw = std::atan2(Forward.Y, Forward.X) * 180.f / PI;
		float Elevation = std::atan2(Forward.Z, std::sqrt(Forward.X * Forward.X + Forward.Y * Forward.Y)) * 180.f / PI;
		bool AngleChanged = ImGui::SliderFloat("Yaw (deg)", &Yaw, -180.f, 180.f, "%.1f");
		AngleChanged |= ImGui::SliderFloat("Elevation (deg)", &Elevation, -90.f, 90.f, "%.1f");
		if (AngleChanged) {
			const float Y = FMath::DegreesToRadians(Yaw), E = FMath::DegreesToRadians(Elevation);
			Value = FVector(std::cos(E) * std::cos(Y), std::cos(E) * std::sin(Y), std::sin(E));
			Changed = true;
		}
		const char* Labels[] = { "+X", "-X", "+Y", "-Y", "+Z", "-Z" };
		const FVector Axes[] = { FVector(1, 0, 0), FVector(-1, 0, 0), FVector(0, 1, 0), FVector(0, -1, 0), FVector(0, 0, 1), FVector(0, 0, -1) };
		for (int Index = 0; Index < IM_ARRAYSIZE(Labels); ++Index) {
			if (Index > 0) ImGui::SameLine();
			if (ImGui::Button(Labels[Index])) { Value = Axes[Index]; Changed = true; }
		}
		if (ImGui::TreeNode("Vector components (advanced)")) {
			Changed |= ImGui::DragFloat3("XYZ", Value.V, 0.01f);
			if (ImGui::Button("Normalize Direction")) {
				FVector::BuildOrthonormalBasis(Value, Forward, Right, Up);
				Value = Forward;
				Changed = true;
			}
			ImGui::TreePop();
		}
		FVector::BuildOrthonormalBasis(Value, Forward, Right, Up);
		DrawDirectionProjection("Top (XY)", Forward.X, Forward.Y, "Right: +X / Up: +Y");
		ImGui::SameLine();
		DrawDirectionProjection("Side (XZ)", Forward.X, Forward.Z, "Right: +X / Up: +Z");
		if (!std::isfinite(Value.X) || !std::isfinite(Value.Y) || !std::isfinite(Value.Z) || Value.SizeSquared() == 0.f)
			ImGui::TextDisabled("Invalid direction: preview and emission use +X.");
		if (Changed) Cone->SetDirection(Value);
		return Changed;
	}

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
			if (UParticleModuleVelocityCone* Cone{ Cast<UParticleModuleVelocityCone>(mTarget) }) {
				Changed |= DrawConeDirection(Cone);
			} else if (UParticleSystem* ParticleSystem{ Cast<UParticleSystem>(mTarget) }) {
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
