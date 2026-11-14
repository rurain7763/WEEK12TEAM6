#pragma once

#include "UObject/Class.h"
#include "Component/PrimitiveComponent.h"

class UFXSystemComponent : public UPrimitiveComponent
{
	DECLARE_CLASS(UFXSystemComponent, UPrimitiveComponent)

public:
	virtual ~UFXSystemComponent() = default;
};
