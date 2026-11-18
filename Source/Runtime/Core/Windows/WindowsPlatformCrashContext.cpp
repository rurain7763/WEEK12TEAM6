#include "EnginePCH.h"
#include "WindowsPlatformCrashContext.h"
#include "Core/Misc/Guid.h"
#include "Core/Misc/Paths.h"
#include <DbgHelp.h>

bool WriteMinidump(HANDLE Process, DWORD ThreadId, const FWindowsPlatformCrashContext& InContext, const TCHAR* Path, LPEXCEPTION_POINTERS ExceptionInfo)
{
	HANDLE FileHandle = CreateFileW(Path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION DumpExceptionInfo = {};
	DumpExceptionInfo.ThreadId = ThreadId;
	DumpExceptionInfo.ExceptionPointers = ExceptionInfo;
	DumpExceptionInfo.ClientPointers = FALSE;

	MINIDUMP_TYPE MinidumpType = MiniDumpNormal;

	bool bShouldBeFullCrashDump = InContext.IsFullCrashDump();
	if (bShouldBeFullCrashDump)
	{
		MinidumpType = (MINIDUMP_TYPE)(MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithThreadInfo);
	}

	BOOL Result = MiniDumpWriteDump(Process, GetProcessId(Process), FileHandle, MinidumpType, &DumpExceptionInfo, NULL, NULL);

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

	FWindowsPlatformCrashContext CrashContext;
	CrashContext.ProcessHandle = GetCurrentProcess();
	CrashContext.Type = ECrashContextType::Crash;
	CrashContext.CrashDumpMode = ECrashDumpMode::Default;
	CrashContext.CrashedThreadId = GetCurrentThreadId();
	CrashContext.CopyPlatformSpecificFiles(CrashFolder, ExceptionInfo);

	return EXCEPTION_EXECUTE_HANDLER;
}

void RegisterUnhandledExceptionHandler()
{
	::SetUnhandledExceptionFilter(EngineUnhandledExceptionFilter);
}

void FWindowsPlatformCrashContext::CopyPlatformSpecificFiles(const std::filesystem::path& OutputDirectory, void* Context)
{
	LPEXCEPTION_POINTERS ExceptionInfo = (LPEXCEPTION_POINTERS)Context;
	std::filesystem::path MinidumpFileName = OutputDirectory / HEMiniDumpName;
	WriteMinidump(ProcessHandle, CrashedThreadId, *this, MinidumpFileName.c_str(), ExceptionInfo);
}
