#ifndef FAT_H
#define FAT_H

#include <stdint.h>

#define fat_SEEK_SET 101
#define fat_SEEK_CUR 102
#define fat_SEEK_END 103

/** Creates a new disk image and FAT16 filesystem

@param disk_image File name for new disk image, C string ending in null byte
@param size Size in bytes, must be 4MiB-32MiB, binary power of two sizes
@return Disk image size for success, -1 for error
*/
int fat_mkfs(char* disk_image, unsigned int size);
/** Mounts a disk image containing a FAT16 filesystem

@param disk_image File name containing a disk image, C string ending in null byte
@return 0 for success, -1 for failure
*/
int fat_mount(char* disk_image);
/** Unmounts the currently mounted disk image

@return 0 for success, -1 for failure
*/
int fat_umount(void);
/** Open a file inside the currently mounted disk image

Write and append modes create a new file if it doesn't exist. Write mode
truncates file to 0 bytes if it already exists. Append mode sets offset in file
to be at the end of the file if it already exists.

@param name File name, C string ending in null byte
@param mode 'r' for read, 'w' for write, 'a' for append
@return File descriptor number or -1 for error
*/
int fat_open(char *name, char mode);
/** Close a file inside the currently mounted disk image

@param fd File descriptor
@return 0 for success, -1 for failure
*/
int fat_close(int fd);
/** Read from an already opened file

@param fd File descriptor
@param buf Buffer for file contents to be placed into
@param count Maximum number of bytes to read
@return Number of bytes read or -1 for error
*/
int fat_read(int fd, void *buf, unsigned int count);
/** Write from an already opened file

@param fd File descriptor
@param buf Buffer for file contents to come from
@param count Maximum number of bytes to write
@return Number of bytes written to file or -1 for error
*/
int fat_write(int fd, void *buf, unsigned int count);
/** Seek in already opened file

@param fd File descriptor
@param offset Offset in bytes based on whence
@param whence One of fat_SEEK_SET, fat_SEEK_CUR or fat_SEEK_END
@return New offset in file after seek or -1 for error
*/
int fat_lseek(int fd, unsigned int offset, int whence);
/** Remove a file

FAT doesn't have inodes, so this removes the file directly. If file is open, the
name will be removed by this function and the file contents removed when the
file is closed.
@param path File path, C string ending in null byte
@return 0 for success, -1 for error
*/
int fat_unlink(char *path);
/** Make a new directory

@param path File path, C string ending in null byte
@return 0 for success, -1 for error
*/
int fat_mkdir(char *path);
/** Remove a directory

The directory must already be empty of files
@param path Directory path, C string ending in null byte
@return 0 for success, -1 for error
*/
int fat_rmdir(char *path);

/** Finds the first empty position (name = 0x00 or name = deleted)
@param directory_sector Pointer to first sector of directory, maybe modified if
	file is in a later linked cluster
@return File entry number in directory or negative value for error
*/
int first_empty_location(int *directory_sector);

/** Unlink a single fat entry
@param current, current entry of fat to be unlinked
@return next cluster in the chain for a given cluster
*/
uint16_t unlink_entry(uint16_t current);

/** Unlink a fat chain
@param start, start entry of fat to be unlinked
@return 0 for success, otherwise error
*/
int unlink_chain(uint16_t start);

/** Finds the first entry in the fat table that is free and return its position
@return position of fat entry, -1 if fat table is full
*/
int first_free_fat_entry(void);

#endif //FAT_H
