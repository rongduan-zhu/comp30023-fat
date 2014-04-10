#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include <stdbool.h>
#include <libgen.h>
#include <ctype.h>
#include <assert.h>
#include "fat.h"
#include "fatstruct.h"
#include "fathelper.h"
#include "disk.h"
#include "utils.h"

#define NUM_HANDLES 4
/* Remember to delete this line! */
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0) //as bzero is not supported on windows

typedef struct fat_filehandle
{
	int file_sector; //disk sector file entry is in
	int file_offset; //number of the file entry within the sector
	unsigned int fp; // location of the file pointer (bytes from start of file)
	char mode; // 'r' for read, 'w' for write, 'a' for append
	bool open; // is the file handle in use?
	bool unlink; // should the file be unlinked when closed?
} fat_filehandle_t;

void print_directory_sector(int sector);

fat_filehandle_t file_handles[NUM_HANDLES];
uint8_t boot_sector[DISK_BLOCK_SIZE];
bool mounted = false;

int fat_mkfs(char* name, unsigned int size)
{
	struct_check();
	make_disk(name, size);
	open_disk(name);
	int disk_size_bytes = disk_size();
	int total_sectors = disk_size_bytes / DISK_BLOCK_SIZE;

	debug_printf("making boot sector\n");
	fat_bs_t bs_struct;
	char jump_bytes[3] = {0x00, 0x00, 0x00};
	memcpy(&bs_struct.jump, jump_bytes, sizeof(bs_struct.jump));
	char oem_label[] = "CSYS2014";
	memcpy(&bs_struct.oem, oem_label, sizeof(bs_struct.oem));
	bs_struct.bpb.bytes_sector = DISK_BLOCK_SIZE;
	//2MiB - 2GiB
	if(disk_size_bytes < MINIMUM_DISK_SIZE ||
		disk_size_bytes > MAXIMUM_DISK_SIZE)
	{
		debug_printf("invalid disk size");
		return -1;
	}
	int sectors_per_cluster = 1;
	if(disk_size_bytes > 1<<22) //4MiB
		sectors_per_cluster = 2;
	if(disk_size_bytes > 1<<24) //16MiB
		sectors_per_cluster = 4;
	bs_struct.bpb.sectors_cluster = (uint8_t)sectors_per_cluster;
	int reserved_sectors = 1; //just the boot sector
	bs_struct.bpb.reserved_sectors = (uint16_t)reserved_sectors;
	bs_struct.bpb.fats = 2;
	bs_struct.bpb.root_entries = 512;
	if(total_sectors > 65535)
		bs_struct.bpb.sectors_volume = 0;
	else
		bs_struct.bpb.sectors_volume = (uint16_t)total_sectors;
	bs_struct.bpb.mdt = 0xf8;
	int non_reserved_sectors = total_sectors - reserved_sectors;
	//integer division, round up
	int non_reserved_clusters =
		(non_reserved_sectors + sectors_per_cluster - 1) / sectors_per_cluster;
	int entries_per_fat_sector = DISK_BLOCK_SIZE / sizeof(uint16_t);
	//again, integer division, round up
	bs_struct.bpb.sectors_fat =
		(uint16_t)((non_reserved_clusters + entries_per_fat_sector - 1) /
		entries_per_fat_sector);
	bs_struct.bpb.sectors_track = 0;
	bs_struct.bpb.heads = 0;
	bs_struct.bpb.hidden_sectors = 0;
	if(total_sectors > 65535)
		bs_struct.bpb.huge_sectors_volume = (uint32_t)total_sectors;
	else
		bs_struct.bpb.huge_sectors_volume = 0;
	bs_struct.ebpb.drive_num = 0;
	bs_struct.ebpb.nt_flags = 0;
	bs_struct.ebpb.signature = 0x29;
	bs_struct.ebpb.volume_id = 0x1234abcd;
	char vol_label[] = "VOL LABEL  ";
	memcpy(&bs_struct.ebpb.volume_label, &vol_label,
		sizeof(bs_struct.ebpb.volume_label));
	char fat_type[] = "FAT16   ";
	memcpy(&bs_struct.ebpb.fat_type_label, &fat_type,
		sizeof(bs_struct.ebpb.volume_label));
	bzero(&bs_struct.boot, sizeof(bs_struct.boot));
	uint8_t boot_sig[2] = {0x55, 0xaa};
	memcpy(&bs_struct.signature, &boot_sig, 2);
	uint8_t boot_sect[DISK_BLOCK_SIZE];
	memcpy(&boot_sect, &bs_struct, DISK_BLOCK_SIZE);
	write_block(0, &boot_sect);
	debug_printf("boot sector written\n");

	/* file allocation table */
	debug_printf("making FAT\n");
	int fat_entries =
		((int)bs_struct.bpb.sectors_fat * (int)bs_struct.bpb.bytes_sector) /
		(int)sizeof(uint16_t);
	uint16_t fat[fat_entries];
	fat[0] = (uint16_t)(0xff + (bs_struct.bpb.mdt << 8));
	fat[1] = 0xffff;
	for(int i = 2; i < fat_entries; ++i)
	{
		fat[i] = 0x0000;
	}
	uint8_t fat_bytes[bs_struct.bpb.bytes_sector * bs_struct.bpb.sectors_fat];
	bzero(&fat_bytes, (size_t)(bs_struct.bpb.bytes_sector * bs_struct.bpb.sectors_fat));
	memcpy(&fat_bytes, &fat, (size_t)fat_entries * sizeof(uint16_t));
	int fat1_start = bs_struct.bpb.reserved_sectors;
	int fat2_start = fat1_start + bs_struct.bpb.sectors_fat;
	for(int i = 0; i < bs_struct.bpb.sectors_fat; ++i)
	{
		//write the first and second FAT in one go - they're identical
		write_block(fat1_start + i, &fat_bytes[i * bs_struct.bpb.bytes_sector]);
		write_block(fat2_start + i,	&fat_bytes[i * bs_struct.bpb.bytes_sector]);
	}
	debug_printf("FATs written\n");

	/* root directory */
	debug_printf("making root directory\n");
	int root_dir_size = bs_struct.bpb.root_entries * (int)sizeof(fat_file_t);
	uint8_t root_dir_bytes[root_dir_size];
	//zeroing the whole thing will make the first byte of the filename field zero
	//indicating no files exist
	bzero(&root_dir_bytes, (size_t)root_dir_size);
	int root_dir_start = bs_struct.bpb.reserved_sectors +
		bs_struct.bpb.fats * bs_struct.bpb.sectors_fat;
	//integer division, round up
	int root_sectors =
		(root_dir_size + bs_struct.bpb.bytes_sector - 1) / bs_struct.bpb.bytes_sector;
	for(int i = 0; i < root_sectors; ++i)
	{
		if(write_block(root_dir_start + i,
			&root_dir_bytes[i * bs_struct.bpb.bytes_sector]) < 0 )
		{
			return -1;
		}
	}
	debug_printf("root directory written\n");
	close_disk();
	return size;
}

void print_directory_sector(int sector)
{
	uint8_t dir_sector[bytes_sector()];
	read_block(sector, &dir_sector);
	fat_file_t dir_files[dir_entries_sector()];
	memcpy(&dir_files, &dir_sector, (size_t)bytes_sector());
	for(int i = 0; i < dir_entries_sector(); ++i)
	{
		if(dir_files[i].name[0] == 0x00)
		{
			printf("file %d name starts with null byte", i);
			continue;
		}
		else if(dir_files[i].name[0] == deleted_file)
		{
			printf("file %d deleted\n", i);
			continue;
		}
		else if(is_lfn(dir_files[i].attr))
		{
			printf("file %d used for LFN\n", i);
			continue;
		}
		printf("%.8s.%.3s", dir_files[i].name, dir_files[i].ext);
		printf(" at cluster %x size %u", dir_files[i].first_cluster,
			dir_files[i].size);
		printf(" attr ");
		print_attributes(dir_files[i].attr);
		printf("\n");
	}
	return;
}

int fat_mount(char* disk_image)
{
	if(mounted)
	{
		debug_printf("already mounted\n");
		return -1;
	}
	//initialise filehandles
	for(int i = 0; i < NUM_HANDLES; ++i)
	{
		file_handles[i].open = false;
	}
	//read boot sector
	if(open_disk(disk_image) < 0)
	{
		debug_printf("unable to open disk\n");
		return -1;
	}
	if(read_block(0, &boot_sector) < 0)
	{
		debug_printf("unable to read boot sector\n");
		return -1;
	}
	//sanity check
	fat_bs_t bs_struct;
	memcpy(&bs_struct, &boot_sector, sizeof(boot_sector));
	if(bs_struct.signature[0] != 0x55 || bs_struct.signature[1] != 0xaa)
	{
		debug_printf("incorrect signature\n");
		close_disk();
		return -1;
	}
	char fat_type[] = "FAT16   ";
	if(memcmp(&bs_struct.ebpb.fat_type_label, &fat_type,
		sizeof(bs_struct.ebpb.fat_type_label)) != 0)
	{
		debug_printf("different FAT type to expected\n");
		close_disk();
		return -1;
	}
	if(bs_struct.bpb.bytes_sector != DISK_BLOCK_SIZE)
	{
		debug_printf("incorrect bytes per sector\n");
		close_disk();
		return -1;
	}
	print_bs();
	mounted = true;
	return 0;
}

int fat_umount()
{
	if(!mounted)
	{
		debug_printf("disk not mounted\n");
		return -1;
	}
	bzero(&boot_sector, (size_t)bytes_sector());
	for(int i = 0; i < NUM_HANDLES; ++i)
	{
		file_handles[i].open = false;
	}
	if(close_disk() < 0)
	{
		return -1;
	}
	mounted = false;
	return 0;
}

int fat_open(char *name, char mode)
{
	if(!mounted)
	{
		debug_printf("disk not mounted\n");
		return -1;
	}
	/* allow mode to be 'w' or 'a' as well */
	if(mode != 'r' && mode != 'w' && mode != 'a')
	{
		debug_printf("invalid mode\n");
		return -1;
	}
	//find an unused handle
	int handle = -1;
	for(int i = 0; i < NUM_HANDLES; ++i)
	{
		if(file_handles[i].open == false)
		{
			handle = i;
			break;
		}
	}
	if(handle == -1)
	{
		debug_printf("all file handles in use\n");
		return -1;
	}
	//traverse directories, find the file
	char namecopy1[MAX_PATH_LEN + 1];
	strncpy(namecopy1, name, MAX_PATH_LEN);
	namecopy1[MAX_PATH_LEN] = '\0';
	char namecopy2[MAX_PATH_LEN + 1];
	strncpy(namecopy2, name, MAX_PATH_LEN);
	namecopy2[MAX_PATH_LEN] = '\0';
	char* dname = dirname(namecopy1);
	char* bname = basename(namecopy2);
	debug_printf("directory name: %s\n", dname);
	debug_printf("file name: %s\n", bname);
	int directory_sector = dir_lookup(dname);
	if(directory_sector < 0)
	{
		debug_printf("directory does not exist\n");
		return -1;
	}
	int file_entry_number = file_lookup(bname, &directory_sector);
	if(file_entry_number < 0 && mode == 'r')
	{
		//file needs to exist for read mode
		debug_printf("file does not exist\n");
		return -1;
	}
	if(file_entry_number < 0 && (mode =='w' || mode == 'a'))
	{
		/* need to create new file */
		//Sets up file name and extension
		debug_printf("file does not exist. Creating new file\n");
		unsigned char fname[FAT_FILE_LEN + 1];
		unsigned char fext[FAT_EXT_LEN + 1];
		fname[FAT_FILE_LEN] = '\0';
		fext[FAT_EXT_LEN] = '\0';
		to_upper((unsigned char *) bname, strlen(bname));
		name_to_83(bname, fname, fext);

		//creates a new fat file
		fat_file_t new_file_entry;
		//finds the first empty location in the directory
		file_entry_number = first_empty_location(&directory_sector);


		int new_cluster;
		//if no new space, but in root directory, exit
		if (directory_sector < start_of_data() && file_entry_number < 0) {
			debug_printf("unable to create new file, root directory is full\n");
			return -1;
		}
		//if run out of space, allocate new cluster
		if (file_entry_number < 0) {
			debug_printf("allocating new cluster for directory\n");
			new_cluster = first_free_fat_entry();
			if (new_cluster == -1) {
				return -1;
			}
			write_fat_entry((uint16_t) new_cluster, last_cluster);
			write_fat_entry((uint16_t) sector_to_data_cluster(directory_sector),
				(uint16_t) new_cluster);
			file_entry_number = 0;
		}

		//sets up the members of struct
		new_file_entry.attr.dir = 0;
		memcpy((void *) new_file_entry.name, (void *) fname, FAT_FILE_LEN);
		memcpy((void *) new_file_entry.ext, (void *) fext, FAT_EXT_LEN);
		new_file_entry.create_time_fine = fine_time_now();
		new_file_entry.create_time = time_now();
		new_file_entry.create_date = date_now();
		new_file_entry.lm_time = new_file_entry.create_time;
		new_file_entry.lm_date = new_file_entry.create_date;
		new_file_entry.first_cluster = 0;
		new_file_entry.size = (uint32_t) 0;

		//write it to disk
		write_file_entry(new_file_entry, directory_sector, file_entry_number);
		debug_printf("successfully wrote to disk\n");
	}
	//read the file structure
	fat_file_t f_entry;
	read_file_entry(&f_entry, directory_sector, file_entry_number);
	//truncate file if in write mode
	if(mode == 'w' && f_entry.size > 0)
	{
		/* existing file needs to be truncated in write mode */
		//unlink chain
		int unlink_chain_success, fat_write_success;
		unlink_chain_success = unlink_chain(f_entry.first_cluster);
		assert(unlink_chain_success == 0);
		//set first cluster value to eof/0xffff
		fat_write_success = write_fat_entry(f_entry.first_cluster, last_cluster);
		assert(fat_write_success == 0);

		//resets the file size to 0
		f_entry.size = (uint16_t) 0;
		f_entry.first_cluster = 0;
		write_file_entry(f_entry, directory_sector, file_entry_number);

	}
	//set up file handle
	debug_printf("using file handle %d\n", handle);
	file_handles[handle].file_sector = directory_sector;
	file_handles[handle].file_offset = file_entry_number;
	if(mode == 'a')
	{
		/*initialise file pointer to end of file*/
		//Although uint32_t should be same as unsigned int but
		//still type cast to be safe.
		file_handles[handle].fp = (unsigned int) f_entry.size;
	}
	else
	{
		file_handles[handle].fp = 0;
	}
	file_handles[handle].mode = mode;
	file_handles[handle].open = true;
	file_handles[handle].unlink = false;
	return handle;
}

int fat_close(int fd)
{
	if(!mounted)
	{
		debug_printf("disk not mounted\n");
		return -1;
	}
	if(fd < 0 || fd >= NUM_HANDLES)
	{
		debug_printf("invalid file descriptor\n");
		return -1;
	}
	if(!file_handles[fd].open)
	{
		debug_printf("file not open\n");
		return -1;
	}
	//usually flush() would get called here
	//but this implementation doesn't have any buffer/cache
	//don't need to do that
	fat_file_t f_entry;
	read_file_entry(&f_entry, file_handles[fd].file_sector,
		file_handles[fd].file_offset);
	file_handles[fd].open = false;
	if(file_handles[fd].unlink == true)
	{
		debug_printf("unlinking file on close\n");
		/* find first cluster of file */
		/* work along FAT chain, mark each cluster as free */
		int unlink_chain_success;
		unlink_chain_success = unlink_chain(f_entry.first_cluster);
		assert(unlink_chain_success == 0);
	}
	return 0;
}

int fat_read(int fd, void *buf, unsigned int count)
{
	if(!mounted)
	{
		debug_printf("not mounted\n");
		return -1;
	}
	if(fd < 0 || fd >= NUM_HANDLES)
	{
		debug_printf("invalid file handle\n");
		return -1;
	}
	if(!file_handles[fd].open)
	{
		debug_printf("file not open\n");
		return -1;
	}
	if(file_handles[fd].mode != 'r')
	{
		debug_printf("wrong file mode\n");
		return -1;
	}
	if(count == 0)
	{
		return 0;
	}
	int bytes_cluster = bytes_sector() * sectors_cluster();
	int read_start_cluster = (int)file_handles[fd].fp / bytes_cluster;
	fat_file_t f_entry;
	read_file_entry(&f_entry, file_handles[fd].file_sector,
		file_handles[fd].file_offset);
	if(file_handles[fd].fp >= f_entry.size)
	{
		return 0;
	}
	uint16_t current_cluster = f_entry.first_cluster;
	for(int i = 0; i < read_start_cluster; ++i)
	{
		int next_c = next_cluster(current_cluster);
		if(next_c <= max_cluster && next_c >= min_cluster)
		{
			current_cluster = (uint16_t)next_c;
		}
		else
		{
			//file length says it should have more clusters
			//but the FAT chain isn't long enough
			exit_error("invalid cluster reference in FAT");
		}
	}
	int offset_in_cluster = (int)file_handles[fd].fp % bytes_cluster;
	int bytes_read = 0;
	int bytes_to_read = (int)count;
	uint8_t *memptr = buf;
	int remaining_in_file = (int)f_entry.size - (int)file_handles[fd].fp;
	while(bytes_to_read > 0 && remaining_in_file > 0)
	{
		uint8_t cluster[bytes_cluster];
		int first_sector = data_cluster_to_sector(current_cluster);
		for(int i = 0; i < sectors_cluster(); ++i)
		{
			read_block(first_sector + i, &cluster[i * bytes_sector()]);
		}
		int remaining_in_cluster = bytes_cluster - offset_in_cluster;
		int readable = imin(bytes_to_read,
			imin(remaining_in_file, remaining_in_cluster));
		memcpy(memptr + bytes_read, &cluster[offset_in_cluster], (size_t)readable);
		bytes_read += readable;
		bytes_to_read -= readable;
		remaining_in_cluster -= readable;
		remaining_in_file -= readable;
		file_handles[fd].fp += (unsigned int)readable;
		if(bytes_to_read == 0 || remaining_in_file == 0)
		{
			break;
		}
		//read as much as possible from that cluster
		//update current_cluster to get the next one
		int next_c = next_cluster(current_cluster);
		if(next_c <= max_cluster && next_c >= min_cluster)
		{
			current_cluster = (uint16_t)next_c;
			offset_in_cluster = 0;
		}
		else
		{
			exit_error("invalid cluster reference in FAT");
		}
	}
	return bytes_read;
}

int fat_lseek(int fd, unsigned int offset, int whence)
{
	if(!mounted)
	{
		debug_printf("not mounted\n");
		return -1;
	}
	if(fd < 0 || fd >= NUM_HANDLES)
	{
		debug_printf("invalid file handle\n");
		return -1;
	}
	if(!file_handles[fd].open)
	{
		debug_printf("file not open\n");
		return -1;
	}
	if(!(whence == fat_SEEK_SET || whence == fat_SEEK_CUR || whence == fat_SEEK_END))
	{
		debug_printf("invalid whence\n");
		return -1;
	}
	fat_file_t f_entry;
	read_file_entry(&f_entry, file_handles[fd].file_sector,
		file_handles[fd].file_offset);
	int file_size = f_entry.size;
	int new_fp = 0;
	if(whence == fat_SEEK_SET)
	{
		new_fp = offset;
	}
	else if(whence == fat_SEEK_CUR)
	{
		new_fp = file_handles[fd].fp + offset;
	}
	else if(whence == fat_SEEK_END)
	{
		new_fp = file_size + offset;
	}
	if(new_fp > file_size)
	{
		if(file_handles[fd].mode == 'r')
		{
			debug_printf("tried to seek off end of file\n");
			return -1;
		}
		//extend the file so that the file pointer is at the end of file
		uint8_t zeros[bytes_sector()];
		bzero(&zeros, (size_t)bytes_sector());
		int extension_needed = new_fp - file_size;
		while(extension_needed > 0)
		{
			int this_write = bytes_sector();
			if(extension_needed < bytes_sector())
			{
				this_write = extension_needed;
			}
			fat_write(fd, &zeros, this_write);
			extension_needed -= this_write;
		}
	}
	//update the actual file pointer
	file_handles[fd].fp = new_fp;
	return (int)file_handles[fd].fp;
}

int fat_write(int fd, void *buf, unsigned int count)
{
	(void)fd;
	(void)buf;
	(void)count;
	if(fd < 0 || fd >= NUM_HANDLES)
	{
		debug_printf("fat_write: invalid file handle\n");
		return -1;
	}
	if (buf == NULL)
	{
		debug_printf("fat_write: null argument \n");
		return -1;
	}
	if (count == 0)
	{
		debug_printf("fat_write: count is 0\n");
		return 0;
	}
	/* check input arguments for errors */
	/* locate the first cluster of the file */
	/* handle situation where file size is zero and no cluster has been
	allocated - allocate the first cluster */
	/* calculate how many clusters into the file the filepointer is and follow
	FAT chain to reach current cluster*/
	/* calculate filepointer location in current cluster */
	/* while more data remains to write */
	/*{*/
		/* if there is any data before filepointer in current cluster, read it
		into memory*/

		/* calculate number of bytes that can be written into current cluster */
		/* copy bytes into cluster-sized memory buffer */
		/* write cluster to disk */
		/* update counters - bytes left to write, file size, file pointer */
		/* update timestamps */
		/* write timestamps and file size in file entry to disk */
		/* if cluster is full, find next cluster */
		/* allocate a new cluster if necessary */
	/*}*/

	if (!mounted)
	{
		debug_printf("fat_write: not mounted\n");
		return -1;
	}
	if(!file_handles[fd].open)
	{
		debug_printf("fat_write: file not open\n");
		return -1;
	}
	if(file_handles[fd].mode != 'w' && file_handles[fd].mode != 'a')
	{
		debug_printf("fat_write: wrong file mode\n");
		return -1;
	}

	int bytes_cluster = bytes_sector() * sectors_cluster();
	int write_start_cluster = (int)file_handles[fd].fp / bytes_cluster;
	fat_file_t f_entry;
	read_file_entry(&f_entry, file_handles[fd].file_sector,
		file_handles[fd].file_offset);
	uint16_t current_cluster = f_entry.first_cluster;

	//If size and current cluster is set to 0, means it is an empty file
	//Allocate a cluster for it
	if (f_entry.size == 0 && current_cluster == 0)
	{
		debug_printf("Empty file, allocating a cluster\n");
		int free_entry, fat_write_success;
		//find first free entry in fat
		free_entry = first_free_fat_entry();
		if (free_entry == -1) {
			return -1;
		}
		debug_printf("free entry found at %d\n", free_entry);
		//then mark the free fat entry.
		fat_write_success = write_fat_entry((uint16_t) free_entry, last_cluster);
		//it should not fail
		assert(fat_write_success == 0);
	}

	//find which cluster the file pointer is in by traverse the fat chain
	for (int i = 0; i < write_start_cluster; ++i) {
		int next_c = next_cluster(current_cluster);
		if (next_c <= max_cluster && next_c >= min_cluster) {
			current_cluster = (uint16_t)next_c;
		} else {
			//file length says it should have more clusters
			//but the FAT chain isn't long enough
			exit_error("invalid cluster reference in FAT");
		}
	}

	//calculate file pointer offset current cluster
	int offset_in_cluster = (int) file_handles[fd].fp % bytes_cluster,
		bytes_written = 0,
		bytes_to_write = (int) count;
	char *temp_buf = (char *) malloc_wrapper(bytes_cluster);
	debug_printf("starting the writing process\n");
	debug_printf("%d bytes to go\n", (int) count);

	//while there are data left to write, write them until all data has
	//been written to disk
	while (bytes_to_write > 0)
	{
		//check if there pointer is at start of cluster, if not read
		//existing data in cluster into temporary buffer
		if (offset_in_cluster > 0) {
			int first_sector,
				read_block_successful;
			//make offset into cluster a 0 based index
			--offset_in_cluster;
			first_sector = data_cluster_to_sector(current_cluster);
			//read each sector that already exist into temporary buffer
			for (int i = 0; i < sectors_cluster(); ++i) {
				read_block_successful = read_block(first_sector + i,
													&temp_buf[i * bytes_sector()]);
				if (read_block_successful == -1) {
					debug_printf("unable to read existing data into memory buffer\n");
					return -1;
				}
			}
		}

		//work out the number of bytes that can be written into temporary
		//buffer after dumping exisiting data (if there is any) in cluster
		//into temporary buffer.
		int remaining_in_cluster = bytes_cluster - offset_in_cluster;
		int bytes_remaining = imin(bytes_to_write, remaining_in_cluster);
		//initialize buffer to 0s and then copy content from buffer into
		//temporary buffer
		memset(temp_buf, 0, bytes_cluster);
		memcpy(temp_buf + offset_in_cluster,
			(char*) buf + bytes_written,
			(size_t) bytes_remaining);
		debug_printf("successfully copied buffer into temporary buffer ready \
			to be written to disk\n");
		//write temporary buffer onto disk image
		int sector_number = data_cluster_to_sector(current_cluster),
			sectors_per_cluster = sectors_cluster(),
			write_block_successful;
		for (int i = 0; i < sectors_per_cluster; ++i)
		{
			//write the cluster sector by sector into disk
			write_block_successful = write_block(sector_number + i,
												&temp_buf[i * bytes_sector()]);
			if (write_block_successful == -1) {
				debug_printf("unable to write buffer to sector %d\n", sector_number + i);
				return -1;
			}
		}

		//housekeeping: update file descriptor information. Do this straight
		//after so the time will be accurate as possible
		f_entry.lm_time = time_now();
		f_entry.lm_date = date_now();
		f_entry.size += (uint32_t)bytes_remaining;

		//writes the updated information back to disk.
		int write_entry_success;
		write_entry_success = write_file_entry(f_entry,
												file_handles[fd].file_sector,
												file_handles[fd].file_offset);
		if (write_entry_success != 0)
		{
			debug_printf("Oh no, failed to sync file descriptor with actual data.\
				\nfailed to write file entry\n");
			return -1;
		}

		//check if there is anymore bytes to write, if none, then ta-da,
		//everything was successful
		if (bytes_to_write == 0) {
			return bytes_written;
		}

		//if there are more data, then get the next cluster
		int next_c;
		next_c = next_cluster(current_cluster);
		//checks if it is last cluster, if it is, then allocate a new cluster
		if (next_c == last_cluster) {
			int new_data_cluster,
				write_fat_successful;
			new_data_cluster = first_free_fat_entry();
			if (new_data_cluster < 0) {
				debug_printf("unable to allocate new cluster. Disk is full\n")
				return -1;
			}

			// make current cluster point to newly allocated cluster
			write_fat_successful = write_fat_entry(current_cluster, (uint16_t) new_data_cluster);
			if (write_fat_successful != 0) {
				debug_printf("unable to link old fat to new fat. \
					possible corrupted fat table\n");
				return -1;
			}
			//assigns current cluster to the new cluster
			current_cluster = (uint16_t) new_data_cluster;
			// make newly allocated cluster have value of EOF
			write_fat_successful = write_fat_entry(current_cluster, last_cluster);
			if (write_fat_successful != 0) {
				debug_printf("unable to make new fat point to EOF\n");
				return -1;
			}
			offset_in_cluster = 0;
		} else {
			if(next_c <= max_cluster && next_c >= min_cluster)
			{
				current_cluster = (uint16_t)next_c;
				offset_in_cluster = 0;
			} else {
				exit_error("invalid cluster reference in FAT");
			}
		}

		//housekeeping: sync counters
		bytes_written += bytes_remaining;
		bytes_to_write -= bytes_remaining;
		remaining_in_cluster -= bytes_remaining;
		file_handles[fd].fp += (unsigned int)bytes_remaining;
	}

	//have a return here to be safe. That been said, should never
	//reach this line.
	return bytes_written;
}

int fat_unlink(char *path)
{
	(void)path;
	/* check input arguments for errors */
	/* traverse directories, find the file and the directory it's in */
	/* check if there are any open file handles on the file */
	/* if there are, set the unlink-on-close flag */
	/* check that the "file" isn't actually a directory */
	/* work along FAT chain, mark each cluster as free */
	/* mark file entry as deleted, write entry to disk */

	if (path == NULL) {
		debug_printf("fat_unlink: Invalid path.\n");
		return -1;
	}

	char namecopy1[MAX_PATH_LEN + 1];
	strncpy(namecopy1, path, MAX_PATH_LEN);
	namecopy1[MAX_PATH_LEN] = '\0';
	char namecopy2[MAX_PATH_LEN + 1];
	strncpy(namecopy2, path, MAX_PATH_LEN);
	namecopy2[MAX_PATH_LEN] = '\0';
	char* dname = dirname(namecopy1);
	char* bname = basename(namecopy2);
	debug_printf("parent directory name: %s\n", dname);
	debug_printf("file name: %s\n", bname);
	int directory_sector = dir_lookup(dname);
	if (directory_sector < 0)
	{
		debug_printf("directory does not exist\n");
		return -1;
	}

	//Look for the file
	int file_entry_number;
	file_entry_number = file_lookup(bname, &directory_sector);
	if (file_entry_number < 0)
	{
		debug_printf("fat_unlink: the file does not exist\n");
		// file does not exist
		return -1;
	}

	//look for open file handles on the file. If there is, mark
	//it so it can be unlinked when close
	bool has_file_handle;
	has_file_handle = false;
	for (int i = 0; i < NUM_HANDLES; ++i) {
		if(file_handles[i].file_sector == directory_sector) {
			file_handles[i].unlink = true;
			has_file_handle = true;
			break;
		}
	}

	//open the file entry from disk, checks if it is a directory, if not
	//then mark size to 0, name to deleted, and start cluster to 0
	fat_file_t f_entry;
	read_file_entry(&f_entry, directory_sector, file_entry_number);\
	if (f_entry.attr.dir == 1) {
		debug_printf("unable to unlink directory. Please use fat_rmdir\n");
		return -1;
	}
	//If the file is not opened, then unlink it. Otherwise delegate it to
	//fat_close
	if (!has_file_handle) {
		unlink_chain(f_entry.first_cluster);
	}
	//mark file as deleted, write updated entry back to disk
	int write_entry_success;
	f_entry.name[0] = deleted_file;
	write_entry_success = write_file_entry(f_entry, directory_sector, file_entry_number);
	return -1;
}

int fat_mkdir(char *path)
{
	(void)path;
	/* check input arguments for errors */
	/* traverse directories, find the parent directory it's in */
	/* check there isn't a file or directory with the same name already */
	/* create a new file, then set its directory bit to true */
	/* allocate a cluster for the directory */
	/* make a memory buffer for the directory cluster, fill with zeros */
	/* create the . and .. entries in the new directory */
	/* write directory cluster to disk */
	char namecopy1[MAX_PATH_LEN + 1];
	strncpy(namecopy1, path, MAX_PATH_LEN);
	namecopy1[MAX_PATH_LEN] = '\0';
	char namecopy2[MAX_PATH_LEN + 1];
	strncpy(namecopy2, path, MAX_PATH_LEN);
	namecopy2[MAX_PATH_LEN] = '\0';
	char* dname = dirname(namecopy1);
	char* bname = basename(namecopy2);
	debug_printf("parent directory name: %s\n", dname);
	debug_printf("new directory name: %s\n", bname);
	int directory_sector = dir_lookup(dname);
	if(directory_sector < 0)
	{
		debug_printf("directory does not exist\n");
		return -1;
	}

	//If parent dir is in root directory, then set cluster number
	//to zero, otherwise set it to the parents cluster number
	//Doing this early because directory_sector might be modified
	//by file_lookup
	int parent_cluster;
	if (directory_sector < start_of_data()) {
		parent_cluster = 0;
	} else {
		parent_cluster = sector_to_data_cluster(directory_sector);
	}

	//checks if file already exists or not
	int file_entry_number;
	file_entry_number = file_lookup(bname, &directory_sector);
	if (file_entry_number >= 0) {
		debug_printf("new directory already exists.\n");
		return -1;
	}

	//finds first free cluster
	int new_cluster_data;
	new_cluster_data = first_free_fat_entry();
	if (new_cluster_data < 0) {
		debug_printf("Cannot create new directory\n \
			Disk/FAT is full.");
		return -1;
	}
	//set the next cluster value to EOF
	write_fat_entry(new_cluster_data, last_cluster);

	//finds first free entry in parent directory to store new
	//directory
	int new_cluster_parent;
	file_entry_number = first_empty_location(&directory_sector);
	//if new directory is in root directory, but root directory is full
	//then don't do anything
	if (directory_sector < start_of_data()
		&& file_entry_number < 0)	{
		debug_printf("Cannot create directory, root directory is full\n");
		return -1;
	}
	debug_printf("Cluster for parent folder is full. Allocating new cluster\n");
	//if all clusters are full, allocate new cluster
	if (file_entry_number < 0) {
		debug_printf("allocating new cluster for parent directory\n");
		new_cluster_parent = first_free_fat_entry();
		//if no cluster is left, then undo the write fat entry for
		//data
		if (new_cluster_parent == -1) {
			write_fat_entry(new_cluster_data, free_cluster);
			return -1;
		}
		write_fat_entry((uint16_t) new_cluster_parent, last_cluster);
		write_fat_entry((uint16_t) sector_to_data_cluster(directory_sector),
			(uint16_t) new_cluster_parent);
		//empty new cluster for parent
		empty_cluster((uint16_t) new_cluster_parent);
		//sets the directory_sector to point to first sector of last cluster
		directory_sector = data_cluster_to_sector((uint16_t) new_cluster_parent);
		file_entry_number = 0;
	}

	//if successfully allocated cluster for data, initialize the cluster
	//to 0s
	empty_cluster(new_cluster_data);

	fat_file_t file_entry;
	unsigned char fname[FAT_FILE_LEN + 1],
				  fext[FAT_EXT_LEN + 1];
	fname[FAT_FILE_LEN] = '\0';
	fext[FAT_EXT_LEN] = '\0';
	to_upper((unsigned char *) bname, strlen(bname));
	name_to_83(bname, fname, fext);
	debug_printf("Writing file descriptors for new file in parent directory\n");
	//sets up new file entry and write it to parent directory
	file_entry = make_file_descriptor(fname, fext, 1, new_cluster_data, 0);
	write_file_entry(file_entry, directory_sector, file_entry_number);
	//sets up current directory entry and write it to data area of new
	//file's data
	debug_printf("Writing file descriptor for self\n");
	name_to_83(".", fname, fext);
	file_entry = make_file_descriptor(fname, fext, 1, new_cluster_data, 0);
	write_file_entry(file_entry, data_cluster_to_sector(new_cluster_data), 0);
	//sets up parent directory entry and write it to data area of new
	//file's data
	debug_printf("Writing file descriptor for parent\n");
	name_to_83("..", fname, fext);
	file_entry = make_file_descriptor(fname, fext, 1, (uint16_t) parent_cluster, 0);
	write_file_entry(file_entry, data_cluster_to_sector(new_cluster_data), 1);

	return 0;
}

int fat_rmdir(char *path)
{
	(void) path;
	if (path == NULL) {
		debug_printf("Yo dude, why you do dis to me?\n");
		return -1;
	}
	/* check input arguments for errors */
	/* traverse directories, find the file and the directory it's in */
	/* check the directory to be removed isn't the root directory */
	/* check the directory doesn't contain any files */
	/* work along FAT chain, mark each cluster as free */
	/* mark file entry as deleted, write entry to disk */
	char namecopy1[MAX_PATH_LEN + 1];
	strncpy(namecopy1, path, MAX_PATH_LEN);
	namecopy1[MAX_PATH_LEN] = '\0';
	char namecopy2[MAX_PATH_LEN + 1];
	strncpy(namecopy2, path, MAX_PATH_LEN);
	namecopy2[MAX_PATH_LEN] = '\0';
	char* dname = dirname(namecopy1);
	char* bname = basename(namecopy2);

	//get parent and current directories sector number
	int current_directory_sector;
	int parent_directory_sector;
	current_directory_sector = dir_lookup(path);
	parent_directory_sector = dir_lookup(dname);

	//make sure it isn't trying to remove root file
	if (strcmp(bname, ".") == 0) {
		debug_printf("Don't be silly bro.\n");
		return -1;
	}

	//check if the directory exists or not
	int file_entry_number;
	file_entry_number = file_lookup(bname, &parent_directory_sector);
	if (file_entry_number < 0) {
		debug_printf("Cannot find the directory.\nMaybe try again later?");
		return -1;
	}

	//get file entry of the directory to be removed
	fat_file_t directory_entry;
	int read_file_entry_successful;
	read_file_entry_successful = read_file_entry(&directory_entry,
												parent_directory_sector,
												file_entry_number);
	if (read_file_entry_successful != 0) {
		debug_printf("Unable to read file entry.\n");
		return -1;
	}

	//check if current directory is empty
	int current_directory_empty;
	current_directory_empty = is_empty_directory(current_directory_sector);
	if (current_directory_empty != 0) {
		debug_printf("Directory not empty\n");
		return -1;
	}

	//unlink the fat chain for the directory and change name to deleted
	unlink_chain(directory_entry.first_cluster);
	directory_entry.name[0] = deleted_file;
	int write_entry_success;
	write_entry_success(directory_entry, parent_directory_sector, file_entry_number);
	return write_entry_success;
}

int first_empty_location(int *directory_sector) {
	int dlocation = *directory_sector;
	bool root_dir = true;
	if(dlocation >= start_of_data())
	{
		root_dir = false;
	}
	debug_printf("looking for first empty file\n");
	while(true)
	{
		uint8_t dir_sector[bytes_sector()];
		read_block(dlocation, &dir_sector);
		fat_file_t dir_files[dir_entries_sector()];
		memcpy(&dir_files, &dir_sector, (size_t)bytes_sector());
		//search the memory block for first empty block
		for(int i = 0; i < dir_entries_sector(); ++i)
		{
			if(dir_files[i].name[0] == 0x00 || dir_files[i].name[0] == deleted_file)
			{
				//got a directory with the correct name
				*directory_sector = dlocation;
				return i;
			}
		}
		//no empty location found, need to pick the next sector to search
		//if there is actually a next sector
		if(root_dir)
		{
			if(dlocation + 1 < start_of_data())
			{
				//there are more root directory sectors to search
				dlocation++;
			}
			else
			{
				//file doesn't exist
				return -1;
			}
		}
		else
		{
			if((dlocation + 1 - start_of_data()) % sectors_cluster() != 0)
			{
				//more sectors to search in this cluster
				dlocation++;
			}
			else if(next_cluster_for_sector(dlocation) <= max_cluster)
			{
				//continues into another cluster
				uint16_t next_c = next_cluster_for_sector(dlocation);
				dlocation = data_cluster_to_sector(next_c);
			}
			else
			{
				return -1;
			}
		}
	}
	return -1;
}

uint16_t unlink_entry(uint16_t current) {
	int next = next_cluster(current);
	int fat_write_success;
	fat_write_success = write_fat_entry((uint16_t) current, free_cluster);
	assert(fat_write_success == 0);
	return next;
}

int unlink_chain(uint16_t start) {
	uint16_t next = start;
	for (int i = 0; i < chain_length(start); ++i) {
		next = unlink_entry(next);
	}
	return 0;
}

int first_free_fat_entry() {
	int current_fat = start_of_fat(),
		fat_end = current_fat + sectors_fat() - 1, //inclusive
		//number of fat entry per block
		entry_block = bytes_sector() / (int) sizeof(uint16_t);
	uint16_t fat_piece[entry_block];
	while (current_fat <= fat_end) {
		read_block(current_fat, &fat_piece); //read the whole fat block
		for (int i = 0; i < entry_block; ++i) {
			if (fat_piece[i] == free_cluster) {
				//return fat entry location by calculating numbers of blocks
				//traversed plus current offset
				return (current_fat - start_of_fat()) * entry_block + i;
			}
		}
		++current_fat;
	}
	//no free fat entry
	return -1;
}

int empty_cluster(uint16_t cluster_entry) {
	int bytes_cluster = bytes_sector() * sectors_cluster(),
		bytes_written,
		current_sector;
	current_sector = data_cluster_to_sector(cluster_entry);
	char *data = (char *) malloc_wrapper(bytes_cluster);
	memset(data, 0, bytes_cluster);
	for (int i = sectors_cluster(); i > 0; --i) {
		bytes_written = write_block(current_sector, (void *) data);
		if (bytes_written == -1) {
			debug_printf("this should never happen [empty_cluster]\n");
			return -1;
		}
		debug_printf("a hell lot of 0s written. %d\n", bytes_written);
	}
	free_wrapper((void *) data);
	return 0;
}

fat_file_t make_file_descriptor(unsigned char *name,
								unsigned char *ext,
								int directory,
								uint16_t first_cluster,
								uint32_t size) {
	fat_file_t new_file;

	memcpy((void *) new_file.name, (void *) name, FAT_FILE_LEN);
	memcpy((void *) new_file.ext, (void *) ext, FAT_EXT_LEN);

	new_file.attr.read_only = (uint8_t)0;
	new_file.attr.hidden = (uint8_t)0;
	new_file.attr.system = (uint8_t)0;
	new_file.attr.vol = (uint8_t)0;
	new_file.attr.dir = directory;
	new_file.attr.archive = (uint8_t)1;
	new_file.attr.device = (uint8_t)0;
	new_file.attr.unused = (uint8_t)0;

	new_file.nt_flags = (uint8_t)0;
	new_file.create_time_fine = fine_time_now();
	new_file.create_time = time_now();
	new_file.create_date = date_now();
	new_file.reserved = (uint16_t)0;
	new_file.fat32_cluster = (uint16_t)0;
	new_file.lm_time = new_file.create_time;
	new_file.lm_date = new_file.create_date;
	new_file.first_cluster = first_cluster;
	new_file.size = (uint32_t) size;
	return new_file;
}

void to_upper(unsigned char *str, int size) {
	for (int i = 0; i < size; ++i) {
		str[i] = toupper((int) str[i]);
	}
}

int is_empty_directory(int directory_sector) {
	bool root_dir = true;
	if(directory_sector >= start_of_data())
	{
		root_dir = false;
	}
	debug_printf("Checking if current directory is empty\n");
	while(true)
	{
		uint8_t dir_sector[bytes_sector()];
		read_block(directory_sector, &dir_sector);
		fat_file_t dir_files[dir_entries_sector()];
		memcpy(&dir_files, &dir_sector, (size_t)bytes_sector());
		//search the memory block to check if it is empty
		for(int i = 0; i < dir_entries_sector(); ++i)
		{
			//if empty, then it should be either 0, deleted, or current and parent
			//directory "pointers". If its any of these continue, otherwise it
			//is not empty
			if(dir_files[i].name[0] == 0x00 ||
				dir_files[i].name[0] == deleted_file ||
				dir_files[i].name[0] == '.' && dir_files[i].name[1] == '.' ||
				dir_files[i].name[0] == '.' && dir_files[i].name[1] == ' ') {
				continue;
			}
			else
			{
				return -1;
			}

		}
		//should be able to delete this.Never need to beck if root directory
		//is empty or not
		if(root_dir)
		{
			if(directory_sector + 1 < start_of_data())
			{
				//there are more root directory sectors to search
				directory_sector++;
			}
			else
			{
				return 0;
			}
		}
		else
		{
			//empty so far, continue searching the next sector
			//if there is actually a next sector
			if((directory_sector + 1 - start_of_data()) % sectors_cluster() != 0)
			{
				// more sectors to search in this cluster
				directory_sector++;
			}
			else if(next_cluster_for_sector(directory_sector) < max_cluster)
			{
				// continues into another cluster
				uint16_t next_c = next_cluster_for_sector(directory_sector);
				directory_sector = data_cluster_to_sector(next_c);
			}
			else
			{
				return 0;
			}
		}
	}
	return 0;
}
