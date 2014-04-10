#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/param.h>
#include "fat.h"
#include "utils.h"

int main(int argc, char **argv)
{
	(void)argv;
	if(argc > 1)
	{
		debug_printing = true;
	}
	else
	{
		debug_printing = false;
	}
	fat_mount("new.img");
	// for (int i = 0; i < 100; ++i) {
	// 	char name[50];
	// 	sprintf(name, "new%d", i);
	// 	fat_mkdir(name);
	// 	sprintf(name, "new%d/small.txt", i);
	// 	int file = fat_open(name, 'w');
	// 	fat_close(file);
	// }
	fat_mkdir("1");
	fat_mkdir("1/2");
 	fat_mkdir("1/2/3");
 	fat_mkdir("1/2/3/4");
 	fat_mkdir("1/2/3/4/5");
 	fat_mkdir("1/2/3/4/5/6");
 	int file = fat_open("1/TEST.TXT", 'w');
 	fat_close(file);
 	file = fat_open("1/2/TEST.TXT", 'w');
 	fat_close(file);
 	file = fat_open("1/2/3/TEST.TXT", 'w');
 	fat_close(file);
 	file = fat_open("1/2/3/4/TEST.TXT", 'w');
 	fat_close(file);
 	file = fat_open("1/2/3/4/5/TEST.TXT", 'w');
 	fat_close(file);
 	file = fat_open("1/2/3/4/5/6/TEST.TXT", 'w');
 	fat_close(file);
	// unsigned char buf[512];
	// int file = fat_open("SUBDIR/SMALL.TXT", 'r');
	// int rval = fat_read(file, &buf, 500);
	// printf("read %d bytes\n", rval);
	// printf("%s", buf);
	// fat_close(file);
	fat_umount();
	return EXIT_SUCCESS;
}
