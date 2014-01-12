#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include "ramdisk.h"
#include <string.h>
#include <errno.h>

#define RAMDISK_MAX_SIZE (1024 * 1024)

int __gen_ramdisk(int fd, char *buf, char *path)
{
	struct ramdisk_header *ramdisk_header;
	struct file_header *file_header;
	struct dirent *pdir;
	char temp[123] = {0};
	int count = -2;
	char *file_header_base = sizeof(struct ramdisk_header) + buf;
	char *file_data_base;
	int file;
	int read_size;
	int error = 0;
	DIR *dir;

	dir = opendir(path);
	if (dir == NULL) {
		printf("Can not open dir %s\n", path);
		return -EACCES;
	}

	while (pdir = readdir(dir)) {
		count ++;
	}
	printf("Find %d files in directory %s\n", count, path);

	file_data_base = file_header_base + count * sizeof(struct file_header);
	file_header = (struct file_header *)file_header_base;

	seekdir(dir, 0);
	printf("%-32s    %-32s    %-32s\n", "File", "Base Address", "Size");
	while (pdir = readdir(dir)) {
		/*
		 *pass . and .. directory
		 */
		if (!strcmp(pdir->d_name, "."))
			continue;
		if (!strcmp(pdir->d_name, ".."))
			continue;

		/*
		 * open file
		 */
		strcpy(temp, path);
		strcat(temp, "/");
		strcat(temp, pdir->d_name);
		file = open(temp, O_RDONLY);
		if (file < 0) {
			printf("Can not open file %s in directory %s\n",temp, path);
			continue;
		}
		
		/*
		 * copy file to ramdisk
		 */
		strcpy(file_header->name, pdir->d_name);
		file_header->base = file_data_base - buf;
		file_header->size = 0;
		do {
			read_size = read(file, file_data_base, 512);
			file_data_base += read_size;
			if ((file_data_base - buf) > RAMDISK_MAX_SIZE) {
				printf("Can Not Generate Ramdisk: Ramdisk Too Large\n");
				error = -ENOMEM;
				close(file);
				goto out;
			}
			file_header->size += read_size;
		} while(read_size == 512);

		printf("%-32s    %-32d    %-32d\n", file_header->name,
					file_header->base,
					file_header->size);
		/*
		 * 4 byte aligin
		 */
		if ((file_header->size % 4) != 0) {
			file_data_base += 4 - (file_header->size % 4);
		}

		close(file);
		file_header ++;
	}

	ramdisk_header = (struct ramdisk_header *)buf;
	strcpy(ramdisk_header->name, "ramdisk");
	ramdisk_header->file_count = count;
	ramdisk_header->total_size = file_data_base - buf;

	write(fd, buf, file_data_base - buf);
out:
	closedir(dir);
	return error;
}

int gen_ramdisk(char *dir, char *target)
{
	DIR *folder = NULL;
	char *buf = NULL;
	int ramdisk = 0;
	int error = 0;

	buf = malloc(RAMDISK_MAX_SIZE);
	if (buf == NULL) {
		printf("Can not allocate temp memory for ramdisk\n");
		return 1;
	}

	ramdisk = open(target, O_RDWR | O_CREAT, S_IRWXU);
	if (ramdisk < 0) {
		printf("Can not create ramdisk: %s\n", target);
		goto release_mem; 
	}

	error = __gen_ramdisk(ramdisk, buf, dir);

close_ramdisk:
	close(ramdisk);

release_mem:
	free(buf);

	return error;
}

int main(int argc, char **argv)
{
	if (!argv[1] || !argv[2]) {
		printf("Please select a folder to generate ramdisk\n");
		return 1;
	}
	
	gen_ramdisk(argv[1], argv[2]);

	return 0;
}
