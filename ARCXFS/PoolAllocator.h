#pragma once

#include "IAllocator.h"

class PoolAllocator : public IAllocator
{
public:
	FileMemory * load_file(ArcXFile *file) override;
};