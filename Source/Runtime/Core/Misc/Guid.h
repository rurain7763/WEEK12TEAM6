#pragma once

#include "Core/EngineString.h"
#include "Core/Types.h"
#include <format>

struct FGuid
{
public:
	FString ToString() const
	{
		return std::format("{:08X}-{:08X}-{:08X}-{:08X}", A, B, C, D);
	}

	static FGuid NewGuid();

public:
	uint32 A;
	uint32 B;
	uint32 C;
	uint32 D;
};

