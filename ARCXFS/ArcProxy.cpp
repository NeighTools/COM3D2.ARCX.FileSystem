// ArcX.FSProxy.cpp : Defines the exported functions for the DLL application.
//

#include "logging.h"
#include <iostream>
#include "ArcXFileSystem.h"
#include "hook.h"

#define DllExport __declspec(dllexport)

struct FSArchive
{
	void *fs_object;
	int fs_type;
};

static void (*OriginalCreateArchive)(FSArchive *dest) = nullptr;

static bool v_table_initialized = false;
static void **FSArchiveVTable = nullptr;
static void **FSArchiveIOVTable = nullptr;
static void **FSArchiveWideFsTable = nullptr;
static OriginalFunctions original_functions;
static HANDLE heap;

inline void init_virutal_tables(void **fs_object)
{
#define CPY_VTABLE(dest, index, size) \
	auto dest##_temp = reinterpret_cast<void**>(fs_object[index]);\
	dest = new void*[size]; \
	memcpy(dest, dest##_temp, size * sizeof(void*));

#define SET(dest, index, func) \
	original_functions.func = reinterpret_cast<func##_t>(dest##_temp[index]); \
	dest[index] = &func;

	CPY_VTABLE(FSArchiveVTable, 0, 15);

	SET(FSArchiveVTable, 5, SetBaseDirectory);
	SET(FSArchiveVTable, 9, AddArchive);
	SET(FSArchiveVTable, 10, AddAutoPathForAllFolder);
	SET(FSArchiveVTable, 13, AddAutoPath);

	CPY_VTABLE(FSArchiveIOVTable, 4, 3);

	SET(FSArchiveIOVTable, 0, GetFile);
	SET(FSArchiveIOVTable, 2, CreateList);

	CPY_VTABLE(FSArchiveWideFsTable, 2, 3);

	SET(FSArchiveWideFsTable, 0, GetFileWide);
}

void create_fs_archive_hook(FSArchive *dest)
{
	LOG("Creating file system. Struct address: " << std::hex << dest);
	OriginalCreateArchive(dest);

	if (dest->fs_object == nullptr)
		return;

	auto fs_obj = reinterpret_cast<void**>(dest->fs_object);

	LOG("Filesystem object at: " << std::hex << fs_obj);

	if (!v_table_initialized)
	{
		LOG("Loading virtual tables!");
		init_virutal_tables(fs_obj);
		v_table_initialized = true;
		LOG("Virtual tables loaded and copied over!");
	}

	auto new_obj = new void*[FS_ARCHIVE_OBJECT_BASE_SIZE_QWORDS + 1];
	memcpy(new_obj, fs_obj, FS_ARCHIVE_OBJECT_BASE_SIZE_QWORDS * 8);

	new_obj[0] = FSArchiveVTable;
	new_obj[2] = FSArchiveWideFsTable;
	new_obj[4] = FSArchiveIOVTable;

	const auto arcx_data = new ArcXArchiveData;
	arcx_data->base = new_obj;
	arcx_data->original_funcs = &original_functions;
	arcx_data->logger = LogStream;

	new_obj[FS_ARCHIVE_OBJECT_BASE_SIZE_QWORDS] = arcx_data;

	LOG("Trying to free previous object!");
	HeapFree(heap, 0, fs_obj);
	LOG("Object freed!");

	dest->fs_object = new_obj;
}


void* WINAPI hookGetProcAddress(HMODULE hmodule, char const* name)
{
	if(lstrcmpiA(name, "DLL_FileSystem_CreateFileSystemArchive") == 0)
	{
		OriginalCreateArchive = reinterpret_cast<void(*)(FSArchive*)>(GetProcAddress(hmodule, name));
		patch_cur_iat(GetModuleHandleA("mono"), "kernel32.dll", &hookGetProcAddress, &GetProcAddress);
		return &create_fs_archive_hook;
	}
	return GetProcAddress(hmodule, name);
}

/*
 * Installs the proxy by hooking cm3d2.dll
 */
extern "C" void DllExport InstallArcX()
{
	INIT_LOG(".");
	LOG("ArcX proxy started!");

	heap = GetProcessHeap();

	if (!patch_cur_iat(GetModuleHandleA("mono"), "kernel32.dll", &GetProcAddress, &hookGetProcAddress))
	{
		LOG("Failed to enable hook!");
		return;
	}

	LOG("Hook done!");
}
