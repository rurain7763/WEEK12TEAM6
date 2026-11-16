#pragma once

#include "Editor/Preview/PreviewEditorTab.h"

class UParticleSystem;
class UParticleSystemComponent;
class FParticleEmitterPanel;
class FParticleModuleDetailsPanel;

class FParticleEditorTab : public FPreviewEditorTab {
private:
	struct FPropertySnapshot {
		UObject* mObject{};
		std::size_t mOffset{};
		TArray<uint8> mValue{};
	};

public:
	explicit FParticleEditorTab(UParticleSystem* InParticleSystem = nullptr);
	~FParticleEditorTab() override;

public:
	const UObject* GetEditingAsset() const override;
	FString GetEditingFilePath() const override;
	bool ConfirmClose() override;

private:
	void BuildDefaultLayout(ImGuiID DockSpaceRoot) override;
	void TickPreview(float DeltaTime) override;
	void InitializePreviewCamera();
	void RestartPreview();

	void OnPropertiesChanged();
	void DrawEditorToolbar();
	void DrawCloseConfirmation();
	bool SaveAsset();

	void CaptureSavedProperties();
	void CaptureObjectProperties(UObject* Object);
	void RestoreSavedProperties();

private:
	UParticleSystem* mParticleSystem{};
	UParticleSystemComponent* mPreviewComponent{};
	FParticleEmitterPanel* mEmitterPanel{};
	FParticleModuleDetailsPanel* mDetailsPanel{};
	TArray<FPropertySnapshot> mSavedProperties{};
	FString mStatus{};
	bool mPendingCameraSetup{ true };
	bool mPendingPreviewRestart{};
	bool mDirty{};
	bool mCloseRequested{};
	bool mCloseApproved{};
};
