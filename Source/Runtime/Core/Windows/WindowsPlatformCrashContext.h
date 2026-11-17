#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include <Windows.h>
#include <filesystem>

enum class ECrashContextType
{
	Crash,
	Assert,
	Ensure
};

enum class ECrashDumpMode
{
	Default,
	FullDump
};

struct FWindowsPlatformCrashContext
{
	const FString HEMiniDumpName = "Minidump.dmp";

	HANDLE ProcessHandle;
	ECrashContextType Type;
	ECrashDumpMode CrashDumpMode;
	uint32 CrashedThreadId;

	void CopyPlatformSpecificFiles(const std::filesystem::path& OutputDirectory, void* Context);
	
	inline bool IsFullCrashDump() const
	{
		return CrashDumpMode == ECrashDumpMode::FullDump;
	}
};

void RegisterUnhandledExceptionHandler();

