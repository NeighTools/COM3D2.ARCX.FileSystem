#include "AllocFileMemory.h"
#include "logging.h"

FileMemory * AllocFileMemory::dispose(bool disposing)
{
	if(disposing)
		delete this;
	return this;
}

bool AllocFileMemory::close_file()
{
	LOG_ARCX(this, "FILE: Trying to close file");
	return false;
}

bool AllocFileMemory::seek(uint64_t dist, bool absolute)
{
	LOG_ARCX(this, "FILE: Seeking file");
	if (absolute)
		this->read_pos = dist;
	else
		this->read_pos += dist;

	if (this->read_pos > this->file->size)
		this->read_pos = this->file->size;
	return true;
}

uint64_t AllocFileMemory::read(void *dest, uint64_t length)
{
	LOG_ARCX(this, "FILE: Reading " << std::dec << length << " bytes");

	if (this->data == nullptr)
		this->data = reinterpret_cast<char*>(this->allocator->get_file_data(this->file));

	LOG_ARCX(this, "Data read!");
	
	auto unread = this->file->size - this->read_pos;

	if (unread == 0)
		return 0;

	auto result = length > unread ? unread : length;

	if(result > 0)
		memcpy(dest, this->data + this->read_pos, result);

	this->read_pos += result;

	return result;
}

uint64_t AllocFileMemory::read_from(void *buffer, uint64_t pos, uint64_t length)
{
	LOG_ARCX(this, "FILE: Reading " << std::dec << length << " bytes from " << std::dec << pos);

	if(this->data == nullptr)
		this->data = reinterpret_cast<char*>(this->allocator->get_file_data(this->file));
	LOG_ARCX(this, "Data read!");

	auto available_len = this->file->size - pos;
	auto result = length > available_len ? available_len : length;

	if(result > 0)
		memcpy(buffer, this->data + pos, result);

	return result;
}

bool AllocFileMemory::is_open()
{
	LOG_ARCX(this, "FILE: Checking open state");
	return this->file != nullptr;
}

void * AllocFileMemory::get_data_ptr()
{
	LOG_ARCX(this, "!!!FILE: Returning null data pointer!!!");
	return nullptr;
}

uint64_t AllocFileMemory::tell()
{
	LOG_ARCX(this, "FILE: Telling position");
	return this->read_pos;
}

uint64_t AllocFileMemory::length()
{
	LOG_ARCX(this, "FILE: Telling size");
	return this->file->size;
}

bool AllocFileMemory::set_file(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this, "!!!FILE: Tried to change file pointer!!!");
	return false;
}

bool AllocFileMemory::set_file2(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this, "!!!FILE: Tried to change file pointer[2]!!!");
	return false;
}

size_t AllocFileMemory::move_memory(void *dest, void *src, size_t len)
{
	memmove(dest, src, len);
	return len;
}

AllocFileMemory::~AllocFileMemory()
{
	LOG_ARCX(this, "Freeing up data!");
	if (data != nullptr)
		free(data);
	data = nullptr;
	LOG_ARCX(this, "Data freed!");
}

AllocFileMemory::AllocFileMemory(IAllocator *allocator, ArcXFile *file, std::ofstream* logger)
{
	this->allocator = allocator;
	this->file = file;
	this->logger = logger;
	this->read_pos = 0;
	this->data = nullptr;
}
