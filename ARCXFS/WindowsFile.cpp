#include "logging.h"
#include "WindowsFile.h"

WindowsFile::WindowsFile(ArcXArchiveData *data, std::wstring path)
{
	this->data = data;
	this->stream = std::ifstream(path, std::ios::binary);

	this->stream.seekg(0, std::ios::end);
	this->len = stream.tellg();
	this->stream.seekg(0, std::ios::beg);

	LOG_ARCX(this->data, "Created file pointer!");
}

WindowsFile *WindowsFile::dispose(bool disposing)
{
	LOG_ARCX(this->data, "Disposing!");

	this->stream.close();

	if (disposing)
		delete this;
	return this;
}

bool WindowsFile::close_file()
{
	LOG_ARCX(this->data, "Closing file!");
	this->stream.close();
	return false;
}

bool WindowsFile::seek(uint64_t dist, bool absolute)
{
	LOG_ARCX(this->data, "Seeking data!");
	this->stream.seekg(dist, absolute ? std::ios::beg : std::ios::cur);
	return true;
}

uint64_t WindowsFile::read(void *dest, uint64_t length)
{
	LOG_ARCX(this->data, "Reading data!");
	this->stream.read((char*) dest, length);
	return this->stream.gcount();
}

uint64_t WindowsFile::read_from(void *buffer, uint64_t pos, uint64_t length)
{
	LOG_ARCX(this->data, "Reading data from pos!");
	size_t prev = this->stream.tellg();
	this->stream.seekg(pos, std::ios::beg);
	this->stream.read((char*)buffer, length);
	size_t read = stream.gcount();
	this->stream.seekg(prev, std::ios::beg);
	return read;
}

bool WindowsFile::is_open()
{
	LOG_ARCX(this->data, "Checking if open!");
	return this->stream.is_open();
}

uint64_t WindowsFile::tell()
{
	LOG_ARCX(this->data, "Telling position");
	return this->stream.tellg();
}

uint64_t WindowsFile::length()
{
	LOG_ARCX(this->data, "Getting length");
	return this->len;
}

bool WindowsFile::set_file(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this->data, "Setting file");
	return false;
}

bool WindowsFile::set_file2(void *data, uint64_t data_length, uint64_t file_offset)
{
	LOG_ARCX(this->data, "Setting file2");
	return false;
}

void * WindowsFile::get_data_ptr()
{
	LOG_ARCX(this->data, "Getting raw data ptr file");
	return nullptr;
}

size_t WindowsFile::move_memory(void *dest, void *src, size_t len)
{
	LOG_ARCX(this->data, "Moving file ptr");
	return 0;
}
