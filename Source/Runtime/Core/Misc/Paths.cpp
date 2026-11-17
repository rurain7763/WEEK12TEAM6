#include "EnginePCH.h"
#include "Paths.h"
#include "Core/Windows/WindowsPlatformProcess.h"

std::filesystem::path FPaths::ProjectSavedDir()
{
	return FWindowsPlatformProcess::BaseDir() / "Saved";
}

