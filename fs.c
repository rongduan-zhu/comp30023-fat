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
	// fat_rmdir("SUBSUB.DIR/..");
	// fat_unlink("kilobyte.bin");

	/* test case 2 */
	fat_mkdir("TESTDIR");
	fat_mkdir("TESTDIR/SUB1");
	fat_mkdir("TESTDIR/SUB1/BIGLONGNAME"); // test what happens when you give long names
	fat_mkdir("TESTDIR/SUB1/BIG.LONGEXTENSION"); // test what happens when you give long extensions
	fat_mkdir("TESTDIR/SUB1/SUB2");

	// test rmdir
	fat_rmdir("DIRTORM"); // should print out an error
	fat_rmdir("DIRTORM/RMBLOCK");
	fat_rmdir("DIRTORM"); // should remove fine now
	fat_rmdir(""); // shouldn't be able to remove root directory
	fat_rmdir(".");

	// make some files
	int file;
	unsigned char text[] = "Some short text to write\n";
	file = fat_open("TESTDIR/TESTDIR.TXT", 'r'); // should be error
	file = fat_open("TESTDIR/TESTDIR.TXT", 'a'); // should make a file
	fat_write(file, &text, sizeof(text));
	fat_write(file, &text, sizeof(text)); // should write it twice, appended
	fat_close(file);

	// test appending after a single character
	file = fat_open("1CHARAP.TXT", 'w');
	unsigned char text2[] = "a";
	fat_write(file, &text2, sizeof(text2));
	fat_close(file);
	file = fat_open("1CHARAP.TXT", 'a');
	unsigned char text3[] = "bcdefghijklmnopqrstuvwxyz";
	fat_write(file, text3, sizeof(text3));
	fat_close(file);

	// test fat_unlink
	file = fat_open("UNLINK.TXT", 'w');
	unsigned char text4[] = "Text in the unlink.txt file\n";
	fat_write(file, &text4, sizeof(text4));
	fat_close(file);
	fat_unlink("UNLINK.TXT");

	// test unlink when file is open
	file = fat_open("ULOPEN.TXT", 'w');
	unsigned char text5[] = "Text in the ulopen.txt file\n";
	fat_write(file, &text5, sizeof(text5));
	fat_unlink("ULOPEN.TXT");
	fat_close(file);

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
