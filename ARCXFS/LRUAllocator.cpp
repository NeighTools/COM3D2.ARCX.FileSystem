#include "LRUAllocator.h"
#include "logging.h"

void FileSlot::clear()
{
	std::free(this->data);
	this->data = nullptr;
	this->file = nullptr;
}

LRUAllocator::LRUAllocator(uint32_t file_cap, std::ofstream* logger)
{
	this->file_cap = file_cap;
	this->allocated_files.reserve(file_cap);
	this->logger = logger;
	
	this->slots = reinterpret_cast<FileSlot*>(calloc(file_cap, sizeof(FileSlot)));
	for(uint64_t i = 1; i < file_cap - 1; i++)
	{
		auto slot = &this->slots[i];
		slot->next = &this->slots[i + 1];
		slot->prev = &this->slots[i - 1];
	}
	this->slots[file_cap - 1].next = &this->slots[0];
	this->slots[file_cap - 1].prev = &this->slots[file_cap - 2];
	this->slots[0].prev = &this->slots[file_cap - 1];
	this->slots[0].next = &this->slots[1];
	
	this->firstFile = nullptr;
	this->lastFile = nullptr;

	LOG_ARCX(this, "LRU: Initialized");
}

void LRUAllocator::load_chunk(ArcXChunk *chunk)
{
	//TODO: Add mutex to loading


	if (chunk->contained_files_count > this->file_cap)
		expand(chunk->contained_files_count);

	LOG_ARCX(this, "LRU: Loading chunk #" << std::dec << chunk->id);

	auto chunk_data = reinterpret_cast<char *>(ARCX_read_chunk(chunk));

	LOG_ARCX(this, "LRU: Got chunk data at " << std::hex << (void*)chunk_data);

	std::vector<ArcXFile*> unallocated;

	for(uint64_t i = 0; i < chunk->contained_files_count; i++)
	{
		auto file = chunk->contained_files[i];

		if (this->allocated_files.find(file) == this->allocated_files.end())
			unallocated.push_back(file);
	}

	uint64_t free_space = this->file_cap - this->allocated_file_count;

	LOG_ARCX(this, "LRU: Allocated files: " << std::dec << this->allocated_file_count << "; cap: " << this->file_cap);

	LOG_ARCX(this, "LRU: Need to allocate " << std::dec << unallocated.size() << " files. Currently free space: " << std::dec << free_space);

	if(free_space < unallocated.size())
	{
		uint64_t to_free = unallocated.size() - free_space;
		
		LOG_ARCX(this, "LRU: Freeing up " << std::dec << to_free << " oldest files");

		FileSlot *slot = this->firstFile;

		while(to_free-- > 0)
		{
			auto old_file = slot->file;
			this->allocated_files.erase(old_file);
			this->allocated_file_count--;
			slot->clear();
			slot = slot->next;
		}
		this->firstFile = slot;
	}

	FileSlot *free_slot = nullptr;
	if (this->lastFile == nullptr)
	{
		this->firstFile = this->slots;
		this->lastFile = this->slots;
		free_slot = this->slots;
	}
	else
		free_slot = this->lastFile->next;

	for (auto file : unallocated)
	{
		if (free_slot->file != nullptr)
		{
			LOG_ARCX(this, "LRU: !!ERROR!! Allocating to allocated slot! Queue is now corrupted!");
		}
		free_slot->file = file;
		free_slot->data = malloc(sizeof(char) * file->size);
		memcpy(free_slot->data, chunk_data + file->offset, file->size);
		this->allocated_files[file] = free_slot;
		this->allocated_file_count++;
		free_slot = free_slot->next;
	}

	this->lastFile = free_slot->prev;
	
	free(chunk_data);
}

void LRUAllocator::expand(uint64_t min_size)
{
	uint64_t new_size = min_size;

	LOG_ARCX(this, "LRU: Expanding file cap to " << std::dec << new_size);

	for (auto&& allocated_file : this->allocated_files)
		allocated_file.second->clear();

	this->allocated_files.clear();

	std::free(this->slots);

	this->slots = reinterpret_cast<FileSlot *>(calloc(new_size, sizeof(FileSlot)));
	this->file_cap = new_size;

	for (uint64_t i = 1; i < this->file_cap - 1; i++)
	{
		auto slot = &this->slots[i];

		slot->prev = &this->slots[i - 1];
		slot->next = &this->slots[i + 1];
	}
	this->slots[this->file_cap - 1].next = &this->slots[0];
	this->slots[this->file_cap - 1].prev = &this->slots[this->file_cap - 2];
	this->slots[0].prev = &this->slots[this->file_cap - 1];
	this->slots[0].next = &this->slots[1];

	this->firstFile = nullptr;
	this->lastFile = nullptr;
	this->allocated_file_count = 0;
	this->allocated_files.reserve(new_size);
}

void * LRUAllocator::get_file_data(ArcXFile *file)
{
	LOG_ARCX(this, "LRU: Getting data for " << narrow(file->file_name));

	mux.lock();
	
	auto res = this->allocated_files.find(file);

	FileSlot* result;

	if (res != this->allocated_files.end())
	{
		LOG_ARCX(this, "LRU: Found file in cache");
		result = res->second;
	} else
	{
		LOG_ARCX(this, "LRU: File not in cache! Loading chunk!");
		load_chunk(file->chunk);
		LOG_ARCX(this, "LRU: File loaded!");
		result = this->allocated_files[file];
	}
	
	tick(result);

	mux.unlock();

	LOG_ARCX(this, "LRU: Returning data for file " << narrow(result->file->file_name));

	return result->data;
}

FileMemory * LRUAllocator::load_file(ArcXFile *file)
{
	return new AllocFileMemory(this, file, this->logger);
}

LRUAllocator::~LRUAllocator()
{
	this->allocated_files.clear();
	free(this->slots);
}

void LRUAllocator::tick(FileSlot * slot)
{
	LOG_ARCX(this, "LRU: Tick!");

	if (slot == this->lastFile)
		return;

	slot->next->prev = slot->prev;
	slot->prev->next = slot->next;

	if (slot == this->firstFile)
		this->firstFile = slot->next;
		
	slot->next = this->lastFile->next;
	slot->prev = this->lastFile;
	this->lastFile->next->prev = slot;
	this->lastFile->next = slot;
	this->lastFile = slot;
}

