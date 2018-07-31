#pragma once

#include "FileMemory.h"
#include <arcx.h>

class IAllocator
{
public:
	virtual ~IAllocator() = default;
	virtual FileMemory *load_file(ArcXFile *file) = 0;
};
