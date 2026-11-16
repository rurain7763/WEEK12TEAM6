#pragma once

#include "Editor/EditorUI/EditorPanel.h"
#include <functional>

class UObject;
class UParticleModule;
class UParticleSystem;
class UParticleEmitter;
class UClass;

class FParticleEmitterPanel : public IEditorPanel {
public:
	enum class EStructureAction {
		AddEmitter,
		DeleteEmitter,
		AddModule,
		DeleteModule
	};

	struct FStructureRequest {
		EStructureAction mAction{ EStructureAction::AddEmitter };
		UParticleEmitter* mEmitter{};
		UClass* mModuleClass{};
		UParticleModule* mModule{};
	};

	using FSelectionCallback = std::function<void(UObject*)>;
	using FToolbarCallback = std::function<void()>;
	using FStructureCallback = std::function<void(const FStructureRequest&)>;

public:
	FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback, FStructureCallback StructureCallback);

public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override;

	UObject* GetSelectedObject() const;
	void SelectObject(UObject* Object);

private:
	void DrawModule(UParticleEmitter* Emitter, UParticleModule* Module, const char* Label, const ImVec4& Color);
	void DrawEmitter(int32 EmitterIndex, float Height);
	void DrawAddModuleMenu(UParticleEmitter* Emitter);
	void DrawEmitterContextMenu(UParticleEmitter* Emitter);

private:
	UParticleSystem* mParticleSystem{};
	UObject* mSelectedObject{};
	FString mPanelName{};
	FSelectionCallback mSelectionCallback{};
	FToolbarCallback mToolbarCallback{};
	FStructureCallback mStructureCallback{};
};
