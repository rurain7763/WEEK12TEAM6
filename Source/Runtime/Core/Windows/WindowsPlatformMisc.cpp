#include "EnginePCH.h"
#include "WindowsPlatformMisc.h"
#include "Core/Misc/Guid.h"

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

