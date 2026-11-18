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
		MoveEmitter,
		SetTypeData,
		AddModule,
		DeleteModule,
		MoveModule,
		SelectLOD,
		AddLOD,
		DeleteLOD,
		RegenerateLODs
	};

	struct FStructureRequest {
		EStructureAction mAction{ EStructureAction::AddEmitter };
		UParticleEmitter* mEmitter{};
		UClass* mModuleClass{};
		UParticleModule* mModule{};
		int32 mTargetIndex{ -1 };
	};

	using FSelectionCallback = std::function<void(UObject*)>;
	using FToolbarCallback = std::function<void()>;
	using FStructureCallback = std::function<void(const FStructureRequest&)>;
	using FChangeCallback = std::function<void()>;

private:
	struct FEmitterDragPayload {
		UParticleSystem* mParticleSystem{};
		UParticleEmitter* mEmitter{};
	};

	struct FModuleDragPayload {
		UParticleEmitter* mEmitter{};
		UParticleModule* mModule{};
	};

public:
	FParticleEmitterPanel(UParticleSystem* ParticleSystem, FString PanelName, FSelectionCallback SelectionCallback, FToolbarCallback ToolbarCallback, FStructureCallback StructureCallback, FChangeCallback ChangeCallback = {});

public:
	bool Init() override;
	void Tick(float DeltaTime) override;
	void OnRender() override;
	const char* GetPanelName() const override;

	UObject* GetSelectedObject() const;
	UParticleEmitter* GetSelectedEmitter() const;
	void SelectObject(UObject* Object);
	void SetSelectedLODIndex(int32 LODIndex);

private:
	void DrawLODToolbar();
	void DrawLODButton(const char* Label, const char* Tooltip, EStructureAction Action, int32 TargetIndex, bool Enabled);
	void DrawModule(UParticleEmitter* Emitter, UParticleModule* Module, const char* Label, const ImVec4& Color);
	void DrawModuleDragDrop(UParticleEmitter* Emitter, UParticleModule* Module, const char* Label);
	void DrawEmitter(int32 EmitterIndex, float Height);
	void DrawEmitterDragSource(UParticleEmitter* Emitter, const char* Label);
	void DrawEmitterDropTarget(UParticleEmitter* Emitter, int32 EmitterIndex);
	void DrawAddModuleMenu(UParticleEmitter* Emitter);
	void DrawTypeDataMenu(UParticleEmitter* Emitter);
	void DrawTypeDataMenuItem(UParticleEmitter* Emitter, UClass* ModuleClass, const char* Label);
	void DrawEmitterContextMenu(UParticleEmitter* Emitter);
	void HandleBackgroundClick();
	void HandleDeleteShortcut();

private:
	UParticleSystem* mParticleSystem{};
	UObject* mSelectedObject{};
	FString mPanelName{};
	FSelectionCallback mSelectionCallback{};
	FToolbarCallback mToolbarCallback{};
	FStructureCallback mStructureCallback{};
	FChangeCallback mChangeCallback{};
	int32 mSelectedLODIndex{};
};
