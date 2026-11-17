#include "EnginePCH.h"
#include "WindowsPlatformProcess.h"

const std::filesystem::path& FWindowsPlatformProcess::ExecutablePath()
{
	static std::filesystem::path Result;

	if (Result.empty())
	{
		constexpr int32 MaxBufferLength = 512;

		WCHAR Buffer[MaxBufferLength] = {};
		DWORD Length = GetModuleFileNameW(nullptr, Buffer, MaxBufferLength);

		if (Length > 0 && Length < MaxBufferLength)
		{
			Result = Buffer;
		}
	}

	return Result;
}

const std::filesystem::path& FWindowsPlatformProcess::BaseDir()
{
	static std::filesystem::path Result;

	if (Result.empty())
	{
		Result = ExecutablePath().parent_path();
	}

	return Result;
}

