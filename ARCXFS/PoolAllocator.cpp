#include "PoolAllocator.h"
#include "AllocFileMemory.h"
#include "logging.h"

#define SLOT2PTR(slot) (char*)this->pool + (slot) * this->slot_size
#define SLOT2PTR_P(pool, slot) (char*)pool + (slot) * this->slot_size

PoolAllocator::PoolAllocator(uint64_t slot_count, uint64_t slot_size, std::ofstream *logger)
{
	this->slots_count = slot_count;
	this->slot_size = slot_size;
	this->reserved_slot_count = 0;
	this->logger = logger;

	LOG_ARCX(this, "Pool: Creating pool of " << std::dec << slot_count * slot_size << " bytes!");

	uint64_t data_to_reserve = sizeof(bool) * slot_count + slot_count * slot_size;

	void *pool = malloc(data_to_reserve);

	this->reserved_flags = reinterpret_cast<bool*>(pool);

	for (uint64_t i = 0; i < this->slots_count; i++)
		this->reserved_flags[i] = false;

	this->pool = (bool*)pool + slot_count;
}

PoolAllocator::~PoolAllocator()
{
	for (auto chunk_slot : this->chunk_slots)
		delete chunk_slot;

	free(this->reserved_flags);
}

FileMemory * PoolAllocator::load_file(ArcXFile *file)
{
	return new AllocFileMemory(this, file, this->logger);
}

void* PoolAllocator::load_chunk(ArcXChunk *chunk)
{
	uint64_t need_slots = chunk->uncompressed_length / this->slot_size + (chunk->uncompressed_length % this->slot_size != 0 ? 1 : 0);
	
	LOG_ARCX(this, "Pool: Loading new chunk! Needs " << std::dec << need_slots << " slots.");
	LOG_ARCX(this, "Pool: Reserved slots: " << this->reserved_slot_count << "; total slots: " << this->slots_count);
	LOG_ARCX(this, "Allocated chunks (slot list): " << std::dec << this->allocated_slots.size());
	LOG_ARCX(this, "Allocated chunks (chunk list): " << std::dec << this->allocated_chunks.size());
	LOG_ARCX(this, "Allocated chunks (chunk linked list): " << std::dec << this->chunk_slots.size());

	uint64_t start_slot;

	if (need_slots > this->slots_count)										// Pool too small; allocate to fit need_slots
		start_slot = expand(need_slots);	
	else if (need_slots > this->slots_count - this->reserved_slot_count)	// Not enough free space; free space (and defrag if needed)
		start_slot = free_space(need_slots);
	else																	// There is free space; find it (defrag if need be)
		start_slot = find_space(need_slots);

	LOG_ARCX(this, "Pool: Going to place chunk into slot " << std::dec << start_slot);

	for(uint64_t i = 0; i < need_slots; i++)
		this->reserved_flags[start_slot + i] = true;

	this->reserved_slot_count += need_slots;

	ChunkSlot *slot = new ChunkSlot;
	slot->start_slot = start_slot;
	slot->reserved_slots = need_slots;
	slot->chunk = chunk;

	this->allocated_chunks[chunk] = slot;

	if(this->allocated_slots.find(start_slot) != this->allocated_slots.end())
	{
		LOG_ARCX(this, "$WRN: Slot is already occupied!");
	}

	this->allocated_slots[start_slot] = slot;
	this->chunk_slots.push_back(slot);

	void* chunk_data = SLOT2PTR(start_slot);
	ARCX_read_chunk_buf(chunk, chunk_data, chunk->uncompressed_length);

	return chunk_data;
}

uint64_t PoolAllocator::expand(uint64_t need_slots)
{
	uint64_t new_size = need_slots + this->reserved_slot_count;

	LOG_ARCX(this, "Pool: Expanding! New size: " << std::dec << new_size);

	void* new_data = malloc(sizeof(bool) * new_size + new_size * this->slot_size);

	bool* new_reserved_flags = static_cast<bool*>(new_data);
	void* new_pool = static_cast<bool*>(new_data) + new_size;

	uint64_t cur_slot = 0;

	this->allocated_slots.clear();

	for (auto chunk_slot : this->chunk_slots)
	{
		memcpy(SLOT2PTR_P(new_pool, cur_slot), SLOT2PTR(chunk_slot->start_slot), chunk_slot->reserved_slots * this->slot_size);
		chunk_slot->start_slot = cur_slot;
		this->allocated_slots[chunk_slot->start_slot] = chunk_slot;
		cur_slot += chunk_slot->reserved_slots;
	}

	for(uint64_t i = 0; i < new_size; i++)
		new_reserved_flags[i] = i < this->reserved_slot_count;

	free(this->reserved_flags);

	this->reserved_flags = new_reserved_flags;
	this->pool = new_pool;
	this->slots_count = new_size;

	return cur_slot;
}

uint64_t PoolAllocator::free_space(uint64_t need_slots)
{
	int64_t to_free = need_slots - (this->slots_count - this->reserved_slot_count);

	LOG_ARCX(this, "Pool: Freeing up " << to_free << " slots!");

	while(to_free > 0)
	{
		auto chunk_slot = this->chunk_slots.front();
		this->chunk_slots.pop_front();

		this->reserved_slot_count -= chunk_slot->reserved_slots;
		to_free -= chunk_slot->reserved_slots;
		
		for(uint64_t i = 0; i < chunk_slot->reserved_slots; i++)
			this->reserved_flags[i + chunk_slot->start_slot] = false;

		this->allocated_chunks.erase(chunk_slot->chunk);
		this->allocated_slots.erase(chunk_slot->start_slot);

		delete chunk_slot;
	}

	LOG_ARCX(this, "Pool: Freed up slots!");

	return find_space(need_slots);
}


uint64_t PoolAllocator::find_space(uint64_t need_slots)
{
	LOG_ARCX(this, "Pool: Searching for free " << need_slots << " slots");

	uint64_t free_space_start = 0;
	uint64_t free_space_count = 0;
	for(uint64_t i = 0; i < this->slots_count; i++)
	{
		if(this->reserved_flags[i])
		{
			if(free_space_count != 0)
			{
				if (free_space_count >= need_slots)
					return free_space_start;

				free_space_start = 0;
				free_space_count = 0;
			}
		} else
		{
			if(free_space_count == 0)
				free_space_start = i;
			free_space_count++;
		}
	}

	if (free_space_count >= need_slots)
		return free_space_start;

	LOG_ARCX(this, "Pool: Defragmenting pool");

	// If no free slot strip found, defragment the whole thing

	LOG_ARCX(this, "Allocated chunks (slot list): " << std::dec << this->allocated_slots.size());
	LOG_ARCX(this, "Allocated chunks (chunk list): " << std::dec << this->allocated_chunks.size());
	LOG_ARCX(this, "Allocated chunks (chunk linked list): " << std::dec << this->chunk_slots.size());

	auto first = this->allocated_slots.begin()->second;
	if(first->start_slot != 0)
	{
		memcpy(SLOT2PTR(0), SLOT2PTR(first->start_slot), first->reserved_slots * this->slot_size);
		first->start_slot = 0;
	}

	for(auto slot = this->allocated_slots.begin(); slot != this->allocated_slots.end(); ++slot)
	{
		auto cur = slot->second;
		auto n = std::next(slot);
		if (n == this->allocated_slots.end())
			break;
		auto next = n->second;

		LOG_ARCX(this, "Cur: " << std::hex << cur << "; next: " << std::hex << next);

		memcpy(SLOT2PTR(cur->start_slot + cur->reserved_slots), SLOT2PTR(next->start_slot), next->reserved_slots * this->slot_size);
		next->start_slot = cur->start_slot + cur->reserved_slots;
	}

	this->allocated_slots.clear();

	for (auto chunk_slot : this->chunk_slots)
		this->allocated_slots[chunk_slot->start_slot] = chunk_slot;

	for(uint64_t i = 0; i < this->slots_count; i++)
		this->reserved_flags[i] = i < this->reserved_slot_count;

	return this->reserved_slot_count;
}

void * PoolAllocator::get_file_data(ArcXFile *file)
{
	LOG_ARCX(this, "Pool: Getting file data");

	mux.lock();

	LOG_ARCX(this, "Pool: Inside mutex!");

	auto res = this->allocated_chunks.find(file->chunk);

	void* chunk;

	if (res == this->allocated_chunks.end())
		chunk = load_chunk(file->chunk);
	else
		chunk = SLOT2PTR(res->second->start_slot);

	LOG_ARCX(this, "Pool: Allocating data!");
	void* result = malloc(file->size);

	memcpy(result, (char*)chunk + file->offset, file->size);

	LOG_ARCX(this, "Pool: Data copied!");

	mux.unlock();

	LOG_ARCX(this, "Pool: Mutex unlocked!");

	return result;
}
