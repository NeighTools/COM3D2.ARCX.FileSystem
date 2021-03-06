#pragma once

#include "FileMemory.h"
#include "ArcXFileSystem.h"
#include <chrono>

/**
 * \brief A proxy for FileMemory that monitors file's usage.
 */
class ProxyFileMemory: public FileMemory  // NOLINT(cppcoreguidelines-special-member-functions)
{
protected:
	~ProxyFileMemory() = default;
public:
	ProxyFileMemory(ArcXArchiveData *data, FileMemory *nativeFile, std::string fileName);

	ProxyFileMemory * dispose(bool disposing) override;
	bool close_file() override;
	bool seek(uint64_t dist, bool absolute) override;
	uint64_t read(void *dest, uint64_t length) override;
	uint64_t read_from(void *buffer, uint64_t pos, uint64_t length) override;
	bool is_open() override;
	uint64_t tell() override;
	uint64_t length() override;
	bool set_file(void *data, uint64_t data_length, uint64_t file_offset) override;
	bool set_file2(void *data, uint64_t data_length, uint64_t file_offset) override;
	void * get_data_ptr() override;
	size_t move_memory(void *dest, void *src, size_t len) override;
private:
	FileMemory *nativeFile;
	ArcXArchiveData *data;
	std::string fileName;
	std::chrono::steady_clock::time_point creationTime;
	uint64_t reads;
	uint64_t seeks;
	uint64_t tells;
	uint64_t length_checks;
	uint64_t ptr_gets;
};
