#include "LRUFileMemory.h"
#include "logging.h"

FileMemory * LRUFileMemory::dispose(bool disposing)
{
	if(disposing)
		delete this;
	return this;
}

bool LRUFileMemory::close_file()
{
	LOG_ARCX(this, "LRUFile: Trying to close file");
	return false;
}

bool LRUFileMemory::seek(uint64_t dist, bool absolute)
{
	LOG_ARCX(this, "LRUFile: Seeking file");
	if (absolute)
		this->read_pos = dist;
	else
		this->read_pos += dist;

	if (this->read_pos > this->file->size)
		this->read_pos = this->file->size;
	return true;
}

uint64_t LRUFileMemory::read(void *dest, uint64_t length)
{
	LOG_ARCX(this, "LRUFile: Reading " << std::dec << length << " bytes");

	auto data = reinterpret_cast<char*>(this->allocator->get_file_data(this->file));
	
	auto unread = this->file->size - this->read_pos;

	if (unread == 0)
		return 0;

	auto result = length > unread ? unread : length;

	if(result > 0)
		memcpy(dest, data + this->read_pos, result);

	this->read_pos += result;

	return result;
}

uint64_t LRUFileMemory::read_from(void *buffer, uint64_t pos, uint64_t length)
{
	LOG_ARCX(this, "LRUFile: Reading " << std::dec << length << " bytes from " << std::dec << pos);

	auto data = reinterpret_cast<char*>(this->allocator->get_file_data(this->file));

	auto available_len = this->file->size - pos;
	auto result = length > available_len ? available_len : length;

	if(result > 0)
		memcpy(buffer, data + pos, result);

	return result;
}

bool LRUFileMemory::is_open()
{
	LOG_ARCX(this, "LRUFile: Checking open state");
	return this->file != nullptr;
}

void * LRUFileMemory::get_data_ptr()
{
	LOG_ARCX(this, "!!!LRUFile: Returning null data pointer!!!");
	return nullptr;
}

uint64_t LRUFileMemory::tell()
{
	LOG_ARCX(this, "LRUFile: Telling position");
	return this->read_pos;
}

uint64_t LRUFileMemory::length()
{
	LOG_ARCX(this, "LRUFile: Telling size");
	return this->file->size;
}

bool LRUFileMemory::set_file(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this, "!!!LRUFile: Tried to change file pointer!!!");
	return false;
}

bool LRUFileMemory::set_file2(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this, "!!!LRUFile: Tried to change file pointer[2]!!!");
	return false;
}

size_t LRUFileMemory::move_memory(void *dest, void *src, size_t len)
{
	memmove(dest, src, len);
	return len;
}

LRUFileMemory::LRUFileMemory(LRUAllocator *allocator, ArcXFile *file, std::ofstream* logger)
{
	this->allocator = allocator;
	this->file = file;
	this->logger = logger;
}
