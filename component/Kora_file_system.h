#ifndef KORA_FILE_SYSTEM
#define KORA_FILE_SYSTEM


int (*kfs_mount)(const char* path);
int (*kfs_unmount)(const char* path);
int (*kfs_open)(const char* path, int mode);
int (*kfs_close)(int file_handle);
int (*kfs_read)(int file_handle, void* buffer, int size);
int (*kfs_write)(int file_handle, const void* data, int size);
int (*kfs_seek)(int file_handle, int offset, int whence);
int (*kfs_remove)(const char* path);
int (*kfs_mkdir)(const char* path);
int (*kfs_rmdir)(const char* path);
int (*kfs_stat)(const char* path, FileStat* stat_buf);




#endif

