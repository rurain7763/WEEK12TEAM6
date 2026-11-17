#pragma once

#include "Editor/Preview/PreviewEditorTab.h"
#include "ParticleEmitterPanel.h"

class UParticleSystem;
class UParticleSystemComponent;
class FParticleModuleDetailsPanel;
class FParticleSystemEditSession;

class FParticleEditorTab : public FPreviewEditorTab {
public:
	explicit FParticleEditorTab(UParticleSystem* InParticleSystem = nullptr);
	~FParticleEditorTab() override;

public:
	const UObject* GetEditingAsset() const override;
	FString GetEditingFilePath() const override;
	bool ConfirmClose() override;

private:
	void BuildDefaultLayout(ImGuiID DockSpaceRoot) override;
	void DrawPreviewToolbar() override;
	void TickPreview(float DeltaTime) override;
	void InitializePreviewCamera();
	void UpdatePreviewMotion(float DeltaTime);
	void RestartPreview();
	void ApplyPendingStructureChange();

	void OnPropertiesChanged();
	void DrawEditorToolbar();
	void DrawCloseConfirmation();
	bool SaveAsset();

private:
	UParticleSystem* mParticleSystem{};
	TUniquePtr<FParticleSystemEditSession> mEditSession{};
	UParticleSystemComponent* mPreviewComponent{};
	FParticleEmitterPanel* mEmitterPanel{};
	FParticleModuleDetailsPanel* mDetailsPanel{};
	FString mStatus{};
	FParticleEmitterPanel::FStructureRequest mPendingStructureRequest{};
	bool mHasPendingStructureRequest{};
	bool mPendingCameraSetup{ true };
	bool mPendingPreviewRestart{};
	bool mMotionEnabled{};
	float mMotionRadius{ 3.0f };
	float mMotionAngle{};
	bool mDirty{};
	bool mCloseRequested{};
	bool mCloseApproved{};
};
