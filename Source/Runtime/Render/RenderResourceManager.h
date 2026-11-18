#pragma once

#include "Shader.h"
#include "ViewMode.h"
#include "Core/NameTypes.h"

struct FShaderDefine
{
	FString Name;
	FString Value;
};

// InputLayout의 SemanticName은 등록 호출이 끝날 때까지 유효해야 한다.
struct FShaderProgramDesc
{
	FString Path;
	FString VertexEntry = "mainVS";
	FString PixelEntry = "mainPS";
	TArray<FShaderDefine> Defines;
	TArray<D3D11_INPUT_ELEMENT_DESC> InputLayout;
};

class FRenderResourceManager
{
public:
	static FRenderResourceManager& Get()
	{
		static FRenderResourceManager* Instance = new FRenderResourceManager();
		return *Instance;
	}

	static void Init()
	{
		Get().ScanShaders("Resources/Shader");
		Get().LoadUberLitPermutations();
		Get().LoadTransparentUberShaders();
	}

	void ScanShaders(const fs::path& ShaderRoot);

	static FShaderProgram* RegisterProgram(FName Key, const FShaderProgramDesc& Desc);

	static void Shutdown();

	static FShaderProgram* GetShaderProgram(FName Key);

	static FShaderProgram* GetUberLitProgram(const FViewRenderMode& Mode, bool bUseSkinning = false);

private:
	static FString GetUberLitSuffix(const FViewRenderMode& Mode, bool bUseSkinning = false);

	void LoadOrCompileShader(const FString& Path, const FString& PermutationSuffix = "", const D3D_SHADER_MACRO* Defines = nullptr, const TArray<D3D11_INPUT_ELEMENT_DESC>& InputLayoutDescs = {});

	void LoadUberLitPermutations();
	void LoadTransparentUberShaders();

	struct FShaderProgramEntry
	{
		TUniquePtr<FVertexShader> VertexShader;
		TUniquePtr<FPixelShader> PixelShader;
		TUniquePtr<FShaderProgram> Program;
	};

	TMap<FName, FShaderProgramEntry, FNameHash> ShaderProgramMap;
};
