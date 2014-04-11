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
	/* mkdir and open w test */
	// fat_mkdir("1");
	// fat_mkdir("1/2");
 // 	fat_mkdir("1/2/3");
 // 	fat_mkdir("1/2/3/4");
 // 	fat_mkdir("1/2/3/4/5");
 // 	fat_mkdir("1/2/3/4/5/6");
 // 	int file = fat_open("1/TEST.TXT", 'w');
 // 	fat_close(file);
 // 	file = fat_open("1/2/TEST.TXT", 'w');
 // 	fat_close(file);
 // 	file = fat_open("1/2/3/TEST.TXT", 'w');
 // 	fat_close(file);
 // 	file = fat_open("1/2/3/4/TEST.TXT", 'w');
 // 	fat_close(file);
 // 	file = fat_open("1/2/3/4/5/TEST.TXT", 'w');
 // 	fat_close(file);
 // 	file = fat_open("1/2/3/4/5/6/TEST.TXT", 'w');
 // 	fat_close(file);

	/* fat_write test */
	// fat_mkdir("TESTDIR");
	// int file2 = fat_open("TESTDIR/TEST.TXT", 'w');
	// unsigned char text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam malesuada varius arcu vel elementum. Vivamus sed commodo enim. Sed ultrices elementum eleifend. Quisque tempor lorem sed nisl pellentesque, et ultricies justo malesuada. Aliquam erat volutpat. Nullam ac tortor in justo rutrum scelerisque sit amet vel massa. Suspendisse potenti. Vestibulum nec pretium mi. Nam suscipit semper dolor non imperdiet. Nulla et orci nulla. Sed ac nibh sed lacus semper condimentum. Ut dignissim erat turpis, ut elementum lacus pulvinar ut. Donec porta eleifend luctus. Sed luctus ipsum eget enim congue, vel sollicitudin dui adipiscing. Phasellus sagittis elit eget magna venenatis porta. Cras pharetra eros ligula, at aliquam dui laoreet in. Vestibulum eu feugiat magna, et mollis tellus. ENDOFTEXT";
	// fat_write(file2, &text, sizeof(text));
	// fat_close(file2);
	// file2 = fat_open("TESTDIR/TEST.TXT", 'a');
	// unsigned char text2[] = "This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. ENDOFTEXT";
	// fat_write(file2, &text2, sizeof(text2));
	// fat_close(file2);

	/* fat_remove */
	// fat_rmdir("TESTDIR");
	// fat_unlink("TESTDIR/TEST.TXT");
	// fat_rmdir("TESTDIR");

	/* fat_write test */
	// fat_mkdir("TESTDIR");
	// unsigned char buf[1024];
	// int file2 = fat_open("TESTDIR/TEST.TXT", 'w');
	// unsigned char text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam malesuada varius arcu vel elementum. Vivamus sed commodo enim. Sed ultrices elementum eleifend. Quisque tempor lorem sed nisl pellentesque, et ultricies justo malesuada. Aliquam erat volutpat. Nullam ac tortor in justo rutrum scelerisque sit amet vel massa. Suspendisse potenti. Vestibulum nec pretium mi. Nam suscipit semper dolor non imperdiet. Nulla et orci nulla. Sed ac nibh sed lacus semper condimentum. Ut dignissim erat turpis, ut elementum lacus pulvinar ut. Donec porta eleifend luctus. Sed luctus ipsum eget enim congue, vel sollicitudin dui adipiscing. Phasellus sagittis elit eget magna venenatis porta. Cras pharetra eros ligula, at aliquam dui laoreet in. Vestibulum eu feugiat magna, et mollis tellus. ENDOFTEXT";
	// fat_write(file2, &text, sizeof(text));
	// fat_close(file2);
	// file2 = fat_open("TESTDIR/TEST.TXT", 'r');
	// int rval = fat_read(file2, &buf, 1024);
	// printf("read %d bytes\n", rval);
	// printf("%s", buf);
	// fat_close(file2);
	// // fat_unlink("TESTDIR/TEST.TXT");

	// file2 = fat_open("TESTDIR/TEST.TXT", 'w');
	// unsigned char text2[] = "This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. ENDOFTEXT";
	// fat_write(file2, &text2, sizeof(text2));
	// fat_close(file2);
	// unsigned char buf2[1024];
	// file2 = fat_open("TESTDIR/TEST.TXT", 'r');
	// rval = fat_read(file2, &buf2, 1024);
	// printf("read %d bytes\n", rval);
	// printf("%s", buf2);
	// fat_close(file2);

	/* rm root directory */
	fat_rmdir(".");
	// fat_rmdir("");

	/* make own image */
	// fat_mkfs("big.img", 1 << 25); // 4 sectors per cluster
	// fat_mount("big.img");
	// fat_mkdir("SUBDIR");
	// fat_mkdir("SUBSUB.DIR");

	// unsigned char kb[1000];
	// memset(&kb, 0x01, 1000);
 //    int file = fat_open("KILOBYTE.BIN", 'w');
 //    fat_write(file, &kb, 1000);
 //    fat_close(file);

 //    unsigned char text[] = "short text string \n\0";
 //    int file2 = fat_open("SUBDIR/SMALL.TXT", 'w');
 //    fat_write(file2, &text, sizeof(text));
 //    fat_close(file2);

 //    unsigned char buf[512];
 //    int file3 = fat_open("SUBDIR/SMALL.TXT", 'r');
 //    int rval = fat_read(file3, &buf, 500);
 //    printf("read %d bytes\n", rval);
 //    printf("%s\n", buf);
 //    fat_close(file3);

	fat_umount();
	// return EXIT_SUCCESS;
}
