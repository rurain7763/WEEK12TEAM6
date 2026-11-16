#include "EnginePCH.h"
#include "WindowsPlatformCrashContext.h"
#include <DbgHelp.h>

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

bool FWindowsPlatformMisc::CreateGuid(FGuid& Result)
{
	if (CoCreateGuid((GUID*)&Result) == S_OK)
	{
		return true;
	}

	return false;
}

void FWindowsPlatformMisc::RaiseException(uint32 ExceptionCode)
{
	::RaiseException(ExceptionCode, 0, 0, NULL);
}

std::filesystem::path FPaths::ProjectSavedDir()
{
	return FWindowsPlatformProcess::BaseDir() / "Saved";
}

bool WriteMinidump(HANDLE Process, DWORD ThreadId, const TCHAR* Path, LPEXCEPTION_POINTERS ExceptionInfo)
{
	MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo = {};
	DumpExceptionInfo.ThreadId = ThreadId;
	DumpExceptionInfo.ExceptionPointers = ExceptionInfo;
	DumpExceptionInfo.ClientPointers = FALSE;

	HANDLE FileHandle = CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	BOOL Result = MiniDumpWriteDump(Process, GetProcessId(Process), FileHandle, MiniDumpNormal, &DumpExceptionInfo, NULL, NULL);

	CloseHandle(FileHandle);

	return Result == TRUE;
}

LONG WINAPI EngineUnhandledExceptionFilter(LPEXCEPTION_POINTERS ExceptionInfo)
{
	FGuid Guid = FGuid::NewGuid();

	std::filesystem::path CrashFolder = FPaths::ProjectSavedDir() / "Crashes" / Guid.ToString();
	
	try
	{
		std::filesystem::create_directories(CrashFolder);
	}
	catch (std::error_code error)
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	std::filesystem::path DumpFilePath = CrashFolder / "Minidump.dmp";

	if (!WriteMinidump(GetCurrentProcess(), GetCurrentThreadId(), DumpFilePath.c_str(), ExceptionInfo))
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

void RegisterUnhandledExceptionHandler()
{
	::SetUnhandledExceptionFilter(EngineUnhandledExceptionFilter);
}

