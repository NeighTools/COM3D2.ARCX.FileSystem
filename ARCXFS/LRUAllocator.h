#pragma once

#include "IAllocator.h"
#include "AllocFileMemory.h"
#include <unordered_map>
#include <mutex>

struct FileSlot
{
	void* data;
	ArcXFile* file;
	FileSlot *prev;
	FileSlot *next;

	void clear();
};

class LRUFilePointer;

class LRUAllocator : public IAllocator
{
public:
	LRUAllocator(uint32_t file_cap, std::ofstream* logger);
	FileMemory * load_file(ArcXFile *file) override;
	~LRUAllocator() override;
	void* get_file_data(ArcXFile* file) override;
private:
	std::unordered_map<ArcXFile*, FileSlot*> allocated_files;
	uint32_t file_cap;
	uint32_t allocated_file_count;
	FileSlot *slots;
	FileSlot *firstFile;
	FileSlot *lastFile;
	std::ofstream* logger;
	std::mutex mux;

	void tick(FileSlot *slot);
	void load_chunk(ArcXChunk *chunk);
	void expand(uint64_t min_size);

	friend class AllocFileMemory;
};