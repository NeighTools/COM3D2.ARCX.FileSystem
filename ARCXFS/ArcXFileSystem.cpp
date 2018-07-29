#include <string>
#include "ArcXFileSystem.h"
#include "logging.h"
#include "WindowsFile.h"
#include "ProxyFileMemory.h"
#include <unordered_set>

namespace fs = std::experimental::filesystem;

void SetBaseDirectory(FileSystemArchiveNative *fs, char *path)
{
	auto self = GET_ARCX_THIS(fs, 0);
	LOG_ARCX(self, "Setting base directory to " << path);

	self->original_funcs->SetBaseDirectory(fs, path);
}

void AddArchive(FileSystemArchiveNative *fs, char *path)
{
	auto self = GET_ARCX_THIS(fs, 0);

	if(!fs::exists(path))
	{
		self->original_funcs->AddArchive(fs, path);
		return;
	}

	auto t1 = std::chrono::steady_clock::now();
	const auto container = ARCX_open(path);
	std::chrono::steady_clock::time_point t2;

	if(container != NULL)
	{
		self->containers.push_back(container);

		for(uint64_t i = 0; i < container->file_count; i++)
		{
			auto file = &container->files[i];
			self->files[std::wstring(file->file_name)] = file;
		}
		t2 = std::chrono::steady_clock::now();
	}else
	{
		t1 = std::chrono::steady_clock::now();
		self->original_funcs->AddArchive(fs, path);
		t2 = std::chrono::steady_clock::now();
	}

	LOG_ARCX(self, "Adding archive, " << path << ", " << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());
}

void AddAutoPathForAllFolder(FileSystemArchiveNative *fs)
{
	auto self = GET_ARCX_THIS(fs, 0);
	LOG_ARCX(self, "Add auto path for all folder!");

	self->original_funcs->AddAutoPathForAllFolder(fs);
}

void AddAutoPath(FileSystemArchiveNative *fs, char *path)
{
	auto self = GET_ARCX_THIS(fs, 0);
	LOG_ARCX(self, "Adding auto path to, " << path << ",");


	self->original_funcs->AddAutoPath(fs, path);
}

std::vector<std::string> *CreateList(FileSystemArchiveNative *fs, std::vector<std::string> *list, char *file_path,
                                      ListType list_type)
{
	auto self = GET_ARCX_THIS(fs, 4);

	auto result = self->original_funcs->CreateList(fs, list, file_path, list_type);

	auto t1 = std::chrono::steady_clock::now();
	auto path = widen(file_path);

	if (!path.empty())
		path += L'\\';

	for (auto pair : self->files)
	{
		if (std::equal(path.begin(), path.end(), pair.first.begin()))
			result->push_back(narrow(pair.first));
	}
	auto t2 = std::chrono::steady_clock::now();


	LOG_ARCX(self, "Created list for " << file_path << " with " << std::dec << result->size() << " items in " << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());

	if (!result->empty())
	{
		LOG_ARCX(self, "Has file: " << result->front());
	}

	return result;
}

void *GetFile(FileSystemArchiveNative *fs, char *file_str)
{
	auto self = GET_ARCX_THIS(fs, 4);
	LOG_ARCX(self, "FILE: " << file_str);

	FileMemory* file = reinterpret_cast<FileMemory *>(self->original_funcs->GetFile(fs, file_str));
	if (file != nullptr)
		return new ProxyFileMemory(self, file, file_str);
	return file;
}


void *GetFileWide(FileSystemArchiveNative *fs, wchar_t *path)
{
	auto self = GET_ARCX_THIS(fs, 2);
	LOG_ARCX(self, "FILE (WIDE): " << narrow(path));

	// TODO: Remove test code

	/*fs::path p("FS_Proxy");
	p /= path;

	LOG_ARCX(self, "Looking for " << fs::canonical(p));

	if(fs::exists(p))
	{
		LOG_ARCX(self, "Found proxy file! Returning that!");
		return new WindowsFile(self, p);
	}*/

	FileMemory *res = reinterpret_cast<FileMemory*>(self->original_funcs->GetFileWide(fs, path));
	if(res != nullptr)
		return new ProxyFileMemory(self, res, narrow(path));
	return res;
}
