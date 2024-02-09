#include "core.h"
#include "hstring.h"
#include "hfs.h"
#include <string.h>

FILE* hfs_open_file(str path, bool read, bool writing) {
	char path_cstr[PATH_CAP];
	str_to_cstr_buf(path, path_cstr, sizeof(path_cstr)); // TODO: Remove allocation
	FILE* result;
	if (read && writing) {
		result = fopen(path_cstr, "rb+");
	} else if (read) {
		result = fopen(path_cstr, "rb");
	} else if (writing) {
		result = fopen(path_cstr, "wb");
	} else {
		panic("Cannot open file for neither reading nor writing");
	}

	return result;
}

void hfs_close_file(FILE* file) {
	fclose(file);
}

#if PLATFORM == PLATFORM_LINUX
#include <dirent.h>
#include <sys/stat.h>

HFSDirResult hfs_open_dir(str path) {
	HFSDirResult result = {0};
	char path_cstr[PATH_CAP];
	str_to_cstr_buf(path, path_cstr, sizeof(path_cstr)); // TODO: Remove allocation
	result.dir.dir = opendir(path_cstr);

	if (result.dir.dir == NULL) { // TODO: Write error to a variable
		return result;
	}

	result.ok = 1;
	return result;
}

void hfs_close_dir(HFSDir dir) {
	i32 status = closedir(dir.dir);
	assert(status != -1);
}

bool hfs_dir_next(HFSDir* dir, HFSDirEntry* entry) {
	assert(dir->dir != NULL);
	struct dirent* dirent_entry = readdir(dir->dir);
	if(dirent_entry == NULL) {
		return false;
	}
	while (strcmp(dirent_entry->d_name, ".") == 0 || strcmp(dirent_entry->d_name, "..") == 0) {
		dirent_entry = readdir(dir->dir);
		if(dirent_entry == NULL) {
			return false;
		}
	}

	usize len = strlen(dirent_entry->d_name);
	assert(len < PATH_CAP);

	memcpy(entry->path_data, dirent_entry->d_name, len);

	entry->path = str_new(entry->path_data, len);

	struct stat statbuf;
	if (stat(dirent_entry->d_name, &statbuf) != 0) {
		return false; // TODO: Proper error handling
	}
	entry->is_dir = S_ISDIR(statbuf.st_mode);

	return true;
}
#endif
