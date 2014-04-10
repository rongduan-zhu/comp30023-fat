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
	fat_mkdir("TESTDIR");
	int file2 = fat_open("TESTDIR/TEST.TXT", 'w');
	unsigned char text[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam malesuada varius arcu vel elementum. Vivamus sed commodo enim. Sed ultrices elementum eleifend. Quisque tempor lorem sed nisl pellentesque, et ultricies justo malesuada. Aliquam erat volutpat. Nullam ac tortor in justo rutrum scelerisque sit amet vel massa. Suspendisse potenti. Vestibulum nec pretium mi. Nam suscipit semper dolor non imperdiet. Nulla et orci nulla. Sed ac nibh sed lacus semper condimentum. Ut dignissim erat turpis, ut elementum lacus pulvinar ut. Donec porta eleifend luctus. Sed luctus ipsum eget enim congue, vel sollicitudin dui adipiscing. Phasellus sagittis elit eget magna venenatis porta. Cras pharetra eros ligula, at aliquam dui laoreet in. Vestibulum eu feugiat magna, et mollis tellus. ENDOFTEXT";
	fat_write(file2, &text, sizeof(text));
	fat_close(file2);
	file2 = fat_open("TESTDIR/TEST.TXT", 'a');
	unsigned char text2[] = "This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. This is some additional text that is to be appended. ENDOFTEXT";
	fat_write(file2, &text2, sizeof(text2));
	fat_close(file2);
	// fat_umount();
	// return EXIT_SUCCESS;
}
