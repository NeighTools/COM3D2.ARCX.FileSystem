#pragma once

#include <vector>

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

// Third vtable

DEF_FUNC(CreateList, std::vector<std::string*> *, FileSystemArchiveNative *fs, std::vector<std::string*> *list, char *
	path, ListType list_type);
DEF_FUNC(GetFile, void*, FileSystemArchiveNative *fs, char *file_str);

// Pointers to original functions in case we need them
struct OriginalFunctions
{
	SetBaseDirectory_t SetBaseDirectory;
	AddArchive_t AddArchive;
	AddAutoPathForAllFolder_t AddAutoPathForAllFolder;
	AddAutoPath_t AddAutoPath;
	CreateList_t CreateList;
	GetFile_t GetFile;
	GetFileWide_t GetFileWide;
};