#include <string>
#include "ArcXFileSystem.h"
#include "logging.h"
#include "ProxyFileMemory.h"
#include <unordered_set>
#include <optional>

namespace fs = std::experimental::filesystem;

inline wchar_t *lower(wchar_t const* str)
{
	auto size = LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, str, -1, nullptr, 0, nullptr, nullptr, 0);
	wchar_t* res = new wchar_t[size];
	LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, str, -1, res, size, nullptr, nullptr, 0);
	return res;
}

inline std::optional<std::wstring_view> get_path_name(std::wstring_view const& str)
{
	auto last = str.find_last_of(L'\\');
	if (last != std::wstring::npos)
		return str.substr(0, last);
	return {};
}

void SetBaseDirectory(FileSystemArchiveNative *fs, char *path)
{
	auto self = GET_ARCX_THIS(fs, 0);
	LOG_ARCX(self, "Setting base directory to " << path);

	self->original_funcs->SetBaseDirectory(fs, path);
}

std::wstring get_file_name(std::wstring_view& path)
{
	auto pos = path.find_last_of(L'\\');
	if (pos == std::wstring::npos)
		return std::wstring(path);
	return std::wstring(path.substr(pos + 1));
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
			std::wstring_view file_path = file->file_name;
			auto name = get_file_name(file_path);
			//LOG_ARCX(self, narrow(name) << " @ " << narrow_view(file_path));
			self->files_by_path[file_path] = file;
			self->files_by_name[name] = file;
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

bool FileExists(FileSystemArchiveNative* fs, char *path)
{
	auto self = GET_ARCX_THIS(fs, 4);

	std::wstring path_wstr = widen(path);
	auto low = lower(path_wstr.c_str());
	path_wstr = low;
	free(low);

	if (self->files_by_name.find(path_wstr) != self->files_by_name.end())
		return true;
	if (self->files_by_path.find(path_wstr) != self->files_by_path.end())
		return true;
	
	return self->original_funcs->FileExists(fs, path);
}

bool FileExistsWide(FileSystemArchiveNative *fs, wchar_t* path)
{
	auto self = GET_ARCX_THIS(fs, 2);

	LOG_ARCX(self, "WIDE: Checking if file exists: " << narrow(path));

	return self->original_funcs->FileExistsWide(fs, path);
}

std::vector<std::wstring> *CreateListWide(FileSystemArchiveNative *fs, std::vector<std::wstring> *vec, wchar_t* path, ListType list_type)
{
	auto self = GET_ARCX_THIS(fs, 2);


	auto t1 = std::chrono::steady_clock::now();

	auto result = self->original_funcs->CreateListWide(fs, vec, path, list_type);

	std::wstring path_str(path);

	if(list_type == AllFile)
	{
		if (!path_str.empty())
			path_str += L'\\';

		// Keep it yolo
		// We don't implement full CreateList for two reasons:
		// 1) Speed: proper implementation would require to add additional string checks (i.e. "has extension"; is deeper than 1 level)
		// 2) Usefulness: Currently COM/CM don't use any other ListType than AllFile
		for (auto pair : self->files_by_path)
		{
			if (std::equal(path_str.begin(), path_str.end(), pair.first.begin()))
				result->push_back(std::wstring(pair.first));
		}
	} else if(list_type == AllFolder)
	{
		std::unordered_set<std::wstring> folder_set;

		for (const auto files_by_path : self->files_by_path)
		{
			auto p = get_path_name(files_by_path.first);
			if (p.has_value())
				folder_set.insert(std::wstring(p.value()));
		}

		for (auto const& folder : folder_set)
			result->push_back(folder);

		folder_set.clear();
	}
	
	auto t2 = std::chrono::steady_clock::now();

	LOG_ARCX(self, "Created list for " << narrow(path) << " of type " << std::dec << list_type <<  " with " << std::dec << result->size() << " items in " << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us");

	if (!result->empty())
	{
		LOG_ARCX(self, "Has file: " << narrow(result->front()));
	}

	//LOG_ARCX(self, "WIDE: Created list for " << narrow(path) << " of type " << std::dec << list_type << " with " << result->size() << " items");

	return result;
}

std::vector<std::string> *CreateList(FileSystemArchiveNative *fs, std::vector<std::string> *list, char *file_path,
                                      ListType list_type)
{
	auto self = GET_ARCX_THIS(fs, 4);

	//auto t1 = std::chrono::steady_clock::now();
	auto result = self->original_funcs->CreateList(fs, list, file_path, list_type);

	//auto path = widen(file_path);

	//if (!path.empty())
	//	path += L'\\';

	//// Keep it yolo
	//// We don't implement full CreateList for two reasons:
	//// 1) Speed: proper implementation would require to add additional string checks (i.e. "has extension"; is deeper than 1 level)
	//// 2) Usefulness: Currently COM/CM don't use any other ListType than AllFile
	//for (auto pair : self->files_by_path)
	//{
	//	if (std::equal(path.begin(), path.end(), pair.first.begin()))
	//		result->push_back(narrow_view(pair.first));
	//}
	//auto t2 = std::chrono::steady_clock::now();


	//LOG_ARCX(self, "Created list for " << file_path << " of type " << std::dec << list_type <<  " with " << std::dec << result->size() << " items in " << std::dec << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us");

	//if (!result->empty())
	//{
	//	LOG_ARCX(self, "Has file: " << result->front());
	//}

	return result;
}




void *GetFile(FileSystemArchiveNative *fs, char *file_str)
{
	auto self = GET_ARCX_THIS(fs, 4);
	LOG_ARCX(self, "FILE: " << file_str);


	std::wstring path = widen(file_str);
	auto lower_str = lower(path.c_str());
	path = lower_str;
	free(lower_str);

	ArcXFile *file_ptr = nullptr;

	auto res = self->files_by_name.find(path);
	if (res != self->files_by_name.end())
		file_ptr = res->second;
	else
	{
		auto res2 = self->files_by_path.find(path);
		if (res2 != self->files_by_path.end())
			file_ptr = res2->second;
	}

	if (file_ptr != nullptr)
	{
		return self->allocator->load_file(file_ptr);
	}

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

	auto lower_str = lower(path);
	std::wstring path_wstr = lower_str;
	free(lower_str);

	ArcXFile *file_ptr = nullptr;

	auto res = self->files_by_name.find(path_wstr);

	if (res != self->files_by_name.end())
		file_ptr = res->second;
	else
	{
		auto res2 = self->files_by_path.find(path_wstr);
		if (res2 != self->files_by_path.end())
			file_ptr = res2->second;
	}


	if(file_ptr != nullptr)
	{
		return self->allocator->load_file(file_ptr);
	}

	FileMemory *file = reinterpret_cast<FileMemory*>(self->original_funcs->GetFileWide(fs, path));
	if(file != nullptr)
		return new ProxyFileMemory(self, file, narrow(path));

	return file;
}
