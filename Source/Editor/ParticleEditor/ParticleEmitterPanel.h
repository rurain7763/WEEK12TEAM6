#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include <functional>

class UObject;
class UParticleModule;
class UParticleSystem;

class FParticleEmitterPanel : public IEditorPanel {
public:
	using FSelectionCallback = std::function<void(UObject*)>;
	using FToolbarCallback = std::function<void()>;

public:
	FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback);

public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override;

	UObject* GetSelectedObject() const;

private:
	void SelectObject(UObject* Object);
	void DrawModule(UParticleModule* Module, const char* Label, const ImVec4& Color);
	void DrawEmitter(int32 EmitterIndex, float Height);

private:
	UParticleSystem* mParticleSystem{};
	UObject* mSelectedObject{};
	FString mPanelName{};
	FSelectionCallback mSelectionCallback{};
	FToolbarCallback mToolbarCallback{};
};
