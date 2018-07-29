#include "ProxyFileMemory.h"
#include "logging.h"

#define LOG_ARCX2(data, msg) *(data->logger) << msg << std::endl;

ProxyFileMemory::ProxyFileMemory(ArcXArchiveData *data, FileMemory * nativeFile, std::string fileName)
{
	this->data = data;
	this->nativeFile = nativeFile;
	this->fileName = fileName;
	this->creationTime = std::chrono::steady_clock::now();
	this->reads = 0;
	this->seeks = 0;
	this->tells = 0;
	this->length_checks = 0;
	this->ptr_gets = 0;
	//LOG_ARCX(this->data, "Created proxy file pointer!");
}

ProxyFileMemory * ProxyFileMemory::dispose(bool disposing)
{
	this->nativeFile->dispose(disposing);
	
	if(disposing)
	{
		auto t = std::chrono::steady_clock::now();
		//LOG_ARCX2(this->data, "$" << this->fileName << ";" << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(t - this->creationTime).count() << ";" << this->reads << ";" << this->ptr_gets << ";" << this->seeks << ";" << this->tells << ";" << this->length_checks);
	}

	if (disposing)
		delete this;
	return this;
}

bool ProxyFileMemory::close_file()
{
	//LOG_ARCX(this->data, "PROXY: Closing file!");
	return this->nativeFile->close_file();
}

bool ProxyFileMemory::seek(uint64_t dist, bool absolute)
{
	//LOG_ARCX(this->data, "PROXY: Seeking to " << std::dec << dist << ". Absolute: " << absolute);
	this->seeks++;
	return this->nativeFile->seek(dist, absolute);
}

uint64_t ProxyFileMemory::read(void * dest, uint64_t length)
{
	//LOG_ARCX2(this->data, "PROXY: (" << this->fileName << ") Reading " << std::dec << length << " bytes into " << std::hex << dest);
	this->reads++;
	return this->nativeFile->read(dest, length);
}

uint64_t ProxyFileMemory::read_from(void * buffer, uint64_t pos, uint64_t length)
{
	//LOG_ARCX(this->data, "PROXY: Reading " << std::dec << length << " bytes from " << std::dec << pos << " into " << std::hex << buffer);
	this->reads++;
	return this->nativeFile->read_from(buffer, pos, length);
}

bool ProxyFileMemory::is_open()
{
	//LOG_ARCX(this->data, "PROXY: Checking open status");
	return this->nativeFile->is_open();
}

uint64_t ProxyFileMemory::tell()
{
	//LOG_ARCX(this->data, "PROXY: Telling position");
	this->tells++;
	return this->nativeFile->tell();
}

uint64_t ProxyFileMemory::length()
{
	this->length_checks++;

	auto len = this->nativeFile->length();

	//LOG_ARCX2(this->data, "PROXY: len=" << std::dec << len);

	return len;
}

bool ProxyFileMemory::set_file(void * data, uint64_t data_length, uint64_t file_offset)
{
	//LOG_ARCX2(this->data, "PROXY: Reinitializing file pointer to different file");

	return this->nativeFile->set_file(data, data_length, file_offset);
}

bool ProxyFileMemory::set_file2(void * data, uint64_t data_length, uint64_t file_offset)
{
	//LOG_ARCX2(this->data, "PROXY: Reinitializing file pointer to different file [2]");

	return this->nativeFile->set_file2(data, data_length, file_offset);
}

void * ProxyFileMemory::get_data_ptr()
{
	//LOG_ARCX2(this->data, "PROXY: (" << this->fileName << ") Getting raw data ptr");
	this->ptr_gets++;
	return this->nativeFile->get_data_ptr();
}

size_t ProxyFileMemory::move_memory(void * dest, void * src, size_t len)
{
	//LOG_ARCX(this->data, "PROXY: memcpy");

	return this->nativeFile->move_memory(dest, src, len);
}
