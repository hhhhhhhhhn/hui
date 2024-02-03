#ifndef HLIB_HFS_H
#define HLIB_HFS_H

#include "core.h"
#include "hstring.h"

FILE* hfs_open_file(str path, bool read, bool writing);
void hfs_close_file(FILE* file);

#if PLATFORM == PLATFORM_LINUX
#include <dirent.h>
typedef struct {
	DIR* dir;
} HFSDir;
#else
#error "hfs is not implemented for your platform"
#endif

typedef struct {
	HFSDir dir;
	bool   ok;
} HFSDirResult;

#define PATH_CAP 256
typedef struct {
	str path;
	char path_data[PATH_CAP];
	bool is_dir;
} HFSDirEntry;

HFSDirResult hfs_open_dir(str path);
void hfs_close_dir(HFSDir dir);
bool hfs_dir_next(HFSDir* dir, HFSDirEntry* entry);

#endif
