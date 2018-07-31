#pragma once

#include "IAllocator.h"
#include <unordered_map>
#include <map>
#include <mutex>

struct ChunkSlot
{
	ArcXChunk *chunk;
	uint64_t start_slot;
	uint64_t reserved_slots;
};

class PoolAllocator : public IAllocator
{
public:
	PoolAllocator(uint64_t slot_count, uint64_t slot_size, std::ofstream *logger);

	~PoolAllocator();

	FileMemory *load_file(ArcXFile *file) override;
	void *get_file_data(ArcXFile *file) override;
private:
	std::unordered_map<ArcXChunk*, ChunkSlot*> allocated_chunks;
	std::map<uint64_t, ChunkSlot*> allocated_slots;
	std::list<ChunkSlot*> chunk_slots;
	uint64_t slots_count;
	uint64_t slot_size;
	uint64_t reserved_slot_count;
	void *pool;
	bool *reserved_flags;
	std::ofstream *logger;
	std::mutex mux;

	void* load_chunk(ArcXChunk *chunk);
	uint64_t expand(uint64_t need_slots);
	uint64_t free_space(uint64_t need_slots);
	uint64_t find_space(uint64_t need_slots);
};