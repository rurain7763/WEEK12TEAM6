#pragma once

#include "Core/EngineString.h"
#include <format>

struct FGuid;

struct FWindowsPlatformProcess
{
	static const std::filesystem::path& ExecutablePath();
	static const std::filesystem::path& BaseDir();
};

struct FWindowsPlatformMisc
{
	static bool CreateGuid(FGuid& Result);
	static void RaiseException(uint32 ExceptionCode);
};

struct FGuid
{
public:
	FString ToString() const
	{
		return std::format("{:08X}-{:08X}-{:08X}-{:08X}", A, B, C, D);
	}

	static FGuid NewGuid()
	{
		FGuid Result;
		FWindowsPlatformMisc::CreateGuid(Result);
		return Result;
	}

public:
	uint32 A;
	uint32 B;
	uint32 C;
	uint32 D;
};

struct FPaths
{
	static std::filesystem::path ProjectSavedDir();
};

struct FWindowsPlatformCrashContext
{

};

void RegisterUnhandledExceptionHandler();

