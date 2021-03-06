#pragma once

#include <vector>
#include <arcx.h>
#include <map>
#include <unordered_map>
#include "IAllocator.h"

// Size od the filesystem object in quadwords
#define FS_ARCHIVE_OBJECT_BASE_SIZE_QWORDS 0x10

// Given the pointer to the filesystem object and the vtable offset of the hooked function,
// Returns the pointer to the additional ARCX data structure
#define GET_ARCX_THIS(fs, vtable_index) \
	(reinterpret_cast<ArcXArchiveData*>(*(fs - vtable_index + FS_ARCHIVE_OBJECT_BASE_SIZE_QWORDS)))

// A helper that creates a function signature and a function pointer type with the same signature
#define DEF_FUNC(name, return_type, ...) \
	return_type name(__VA_ARGS__); \
	typedef return_type (*name##_t)(__VA_ARGS__);

enum ListType
{
	TopFolder,
	AllFolder,
	TopFile,
	AllFile
};

struct OriginalFunctions;

// This struct is the additional data we can append to a file system
struct ArcXArchiveData
{
	void **base;
	std::ofstream *logger;
	OriginalFunctions *original_funcs;
	std::vector<ArcXContainer*> containers;
	std::unordered_map<std::wstring, ArcXFile*> files_by_name;
	std::unordered_map<std::wstring_view, ArcXFile*> files_by_path;
	IAllocator *allocator;
	// TODO: Add data
};

using FileSystemArchiveNative = void*;

// First vtable

DEF_FUNC(SetBaseDirectory, void, FileSystemArchiveNative *fs, char *path);
DEF_FUNC(AddArchive, void, FileSystemArchiveNative *fs, char *path);
DEF_FUNC(AddAutoPathForAllFolder, void, FileSystemArchiveNative *fs);
DEF_FUNC(AddAutoPath, void, FileSystemArchiveNative *fs, char *path);

// Second vtable

DEF_FUNC(GetFileWide, void*, FileSystemArchiveNative *fs, wchar_t *path);
DEF_FUNC(FileExistsWide, bool, FileSystemArchiveNative *fs, wchar_t *path);
DEF_FUNC(CreateListWide, std::vector<std::wstring> *, FileSystemArchiveNative *fs, std::vector<std::wstring> *vec, wchar_t *path, ListType list_type);

// Third vtable

DEF_FUNC(CreateList, std::vector<std::string> *, FileSystemArchiveNative *fs, std::vector<std::string> *list, char *
	path, ListType list_type);
DEF_FUNC(GetFile, void*, FileSystemArchiveNative *fs, char *file_str);
DEF_FUNC(FileExists, bool, FileSystemArchiveNative *fs, char *file_str);

// Pointers to original functions in case we need them
struct OriginalFunctions
{
	SetBaseDirectory_t SetBaseDirectory;
	AddArchive_t AddArchive;
	AddAutoPathForAllFolder_t AddAutoPathForAllFolder;
	AddAutoPath_t AddAutoPath;
	CreateList_t CreateList;
	GetFile_t GetFile;
	FileExists_t FileExists;
	GetFileWide_t GetFileWide;
	FileExistsWide_t FileExistsWide;
	CreateListWide_t CreateListWide;
};