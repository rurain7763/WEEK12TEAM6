#include "EnginePCH.h"
#include "Guid.h"
#include "Core/Windows/WindowsPlatformMisc.h"

FGuid FGuid::NewGuid()
{
    FGuid Result;
    FWindowsPlatformMisc::CreateGuid(Result);
    return Result;
}
