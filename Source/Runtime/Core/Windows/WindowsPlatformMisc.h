#pragma once

#include "Core/Types.h"

struct FGuid;

struct FWindowsPlatformMisc
{
    static bool CreateGuid(FGuid& Result);
    static void RaiseException(uint32 ExceptionCode);
};
