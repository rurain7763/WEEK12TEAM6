#pragma once

#include "Editor/Preview/PreviewEditorTab.h"

class UParticleSystem;
class UParticleSystemComponent;

class FParticleEditorTab : public FPreviewEditorTab {
public:
	explicit FParticleEditorTab(UParticleSystem* InParticleSystem = nullptr);
	~FParticleEditorTab() override;

public:
	const UObject* GetEditingAsset() const override;
	FString GetEditingFilePath() const override;

private:
	void TickPreview(float DeltaTime) override;
	void InitializePreviewCamera();

private:
	UParticleSystem* mParticleSystem{};
	UParticleSystemComponent* mPreviewComponent{};
	bool mPendingCameraSetup{ true };
};
