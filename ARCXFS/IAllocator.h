#pragma once

#include "FileMemory.h"
#include <arcx.h>

class IAllocator
{
public:
	virtual ~IAllocator() = default;
	virtual FileMemory *load_file(ArcXFile *file) = 0;
	virtual void *get_file_data(ArcXFile *file) = 0;
};
