#include "CMFilePointer.h"
#include "logging.h"

CMFilePointer::CMFilePointer(ArcXArchiveData *data, std::wstring path)
{
	this->data = data;
	this->stream = std::ifstream(path, std::ios::binary);

	this->stream.seekg(0, std::ios::end);
	this->len = stream.tellg();
	this->stream.seekg(0, std::ios::beg);

	LOG_ARCX(this->data, "Created file pointer!");
}

CMFilePointer *CMFilePointer::dispose(bool disposing)
{
	LOG_ARCX(this->data, "Disposing!");

	this->stream.close();

	if (disposing)
		delete this;
	return this;
}

bool CMFilePointer::close_file()
{
	LOG_ARCX(this->data, "Closing file!");
	this->stream.close();
	return false;
}

bool CMFilePointer::seek(uint64_t dist, bool absolute)
{
	LOG_ARCX(this->data, "Seeking data!");
	this->stream.seekg(dist, absolute ? std::ios::beg : std::ios::cur);
	return true;
}

uint64_t CMFilePointer::read(void *dest, uint64_t length)
{
	LOG_ARCX(this->data, "Reading data!");
	this->stream.read((char*) dest, length);
	return this->stream.gcount();
}

uint64_t CMFilePointer::read_from(void *buffer, uint64_t pos, uint64_t length)
{
	LOG_ARCX(this->data, "Reading data from pos!");
	size_t prev = this->stream.tellg();
	this->stream.seekg(pos, std::ios::beg);
	this->stream.read((char*)buffer, length);
	size_t read = stream.gcount();
	this->stream.seekg(prev, std::ios::beg);
	return read;
}

bool CMFilePointer::is_open()
{
	LOG_ARCX(this->data, "Checking if open!");
	return this->stream.is_open();
}

uint64_t CMFilePointer::tell()
{
	LOG_ARCX(this->data, "Telling position");
	return this->stream.tellg();
}

void *CMFilePointer::next_file()
{
	LOG_ARCX(this->data, "Getting next file");
	return nullptr;
}

uint64_t CMFilePointer::unk1()
{
	LOG_ARCX(this->data, "Unknown1");
	return this->len;
}

uint64_t CMFilePointer::length()
{
	LOG_ARCX(this->data, "Getting length");
	return this->len;
}

bool CMFilePointer::set_file(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this->data, "Setting file");
	return false;
}

bool CMFilePointer::set_file2(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this->data, "Setting file2");
	return false;
}

size_t CMFilePointer::move_file_ptr(void *dest, void *src, size_t len)
{
	LOG_ARCX(this->data, "Moving file ptr");
	return 0;
}
