#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include <functional>

class UObject;

class FParticleModuleDetailsPanel : public IEditorPanel {
public:
	using FChangeCallback = std::function<void()>;

public:
	FParticleModuleDetailsPanel(FString PanelName, FChangeCallback ChangeCallback);

public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override;

	void SetTarget(UObject* Object);

private:
	FString mPanelName{};
	UObject* mTarget{};
	FChangeCallback mChangeCallback{};
};
