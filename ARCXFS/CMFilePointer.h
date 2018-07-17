#pragma once

#include "ArcXFileSystem.h"
#include <cstdint>
#include <fstream>

/*
 * A native file pointer for cm3d2.dll
 * 
 * Mimicks the vtable of FileMemory
 */
class CMFilePointer
{
public:
	CMFilePointer(ArcXArchiveData *data, std::wstring path);

	virtual CMFilePointer *dispose(bool disposing);
	virtual bool close_file();
	virtual bool seek(uint64_t dist, bool absolute);
	virtual uint64_t read(void *dest, uint64_t length);
	virtual uint64_t read_from(void *buffer, uint64_t pos, uint64_t length);
	virtual bool is_open();
	virtual uint64_t tell();
	virtual void *next_file();
	virtual uint64_t unk1();
	virtual uint64_t length();
	virtual bool set_file(void *data, uint64_t data_length, uint64_t file_offset);
	virtual bool set_file2(void *data, uint64_t data_length, uint64_t file_offset);
	virtual size_t move_file_ptr(void *dest, void *src, size_t len);
private:
	ArcXArchiveData *data;
	std::ifstream stream;
	size_t len;
};
