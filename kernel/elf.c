#include <os/elf.h>
#include <os/slab.h>
#include <os/types.h>
#include <os/fs.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/errno.h>
#include <os/printk.h>

static void elf_file_add_section(struct elf_file *file,
			 elf_section_header *header,
			 struct elf_section *section,
			 char *name)
{
	struct elf_section *tmp = file->head;

	section->offset = header->sh_offset;
	section->size = header->sh_size;
	section->load_addr = header->sh_addr;
	section->next = NULL;
	strcpy(section->name, name);

	file->alloc_size += section->size;
	if (tmp == NULL)
		file->head = section;
	else {
		while ((tmp->next) != NULL) {
			tmp = tmp->next;
		}
		
		tmp->next = section;
	}

	kernel_debug("[%s]: offset:0x%x size:0x%x load_addr:0x%x\n",
		section->name, section->offset,
		section->size, section->load_addr);
}

static struct elf_file *parse_elf_info(elf_section_header *header,
				int section_num, char *str)
{
	elf_section_header *tmp = header;
	int i;
	char *name;
	struct elf_file *elf_file =NULL;
	struct elf_section *section;

	elf_file = (struct elf_file *)kzalloc(sizeof(struct elf_file), GFP_KERNEL);
	if (elf_file == NULL)
		return NULL;

	for (i = 0; i< section_num; i++) {
		/* whether this section needed allocate mem. */
		if (tmp->sh_flags & SHF_ALLOC) {
			section = (struct elf_section *)kzalloc(sizeof(struct elf_section), GFP_KERNEL);
			if (section == NULL) {
				elf_file->alloc_size = 0;
				goto out;
			}

			name = &str[tmp->sh_name];
			elf_file_add_section(elf_file, tmp, section, name);
		}

		tmp++;
	}

out:
	return elf_file;
}

void release_elf_file(struct elf_file *file)
{
	struct elf_section *section;
	struct elf_section *tmp;

	tmp = file->head;
	while (tmp != NULL) {
		section = tmp;
		tmp = tmp->next;
		kfree(section);
	}

	kfree(file);
}

struct elf_file *get_elf_info(struct file *file)
{
	elf_header hdr;
	int ret = -1;
	elf_section_header *header;
	char *str;
	struct elf_file *elf_file;

	ret = fs_read(file, (char *)&hdr, sizeof(elf_header));
	if (ret < 0) {
		kernel_error("read elf file error at 0x%x offset 0",
						sizeof(elf_header));
		return NULL;
	}
	
       /* confirm whether this file is a elf binary */
	if (strncmp(ELFMAG, hdr.e_ident, 4)) {
		kernel_error("file is not a elf file exist\n");
		return NULL;
	}

	kernel_debug("ident:          %s\n", &hdr.e_ident[1]);
	kernel_debug("type:           %d\n", hdr.e_type);
	kernel_debug("machine:        %d\n", hdr.e_machine);
	kernel_debug("version:        %d\n", hdr.e_version);
	kernel_debug("entry:          0x%x\n", hdr.e_entry);
	kernel_debug("phoff:          0x%x\n", hdr.e_phoff);
	kernel_debug("pentsize:       %d\n", hdr.e_phentsize);
	kernel_debug("phnum:          %d\n", hdr.e_phnum);
	kernel_debug("shoff:          0x%x\n", hdr.e_shoff);
	kernel_debug("shnum:          %d\n", hdr.e_shnum);
	kernel_debug("shentsize:      %d\n", hdr.e_shentsize);
	kernel_debug("shstrndx:       %d\n", hdr.e_shstrndx);

	header = (elf_section_header *)kmalloc(hdr.e_shnum * hdr.e_shentsize, GFP_KERNEL);
	if (header == NULL)
		return NULL;

	str = (char *)kmalloc(4096, GFP_KERNEL);
	if (str == NULL)
		goto err_str_mem;

	ret = fs_seek(file, hdr.e_shoff);
	if (ret < 0) {
		kernel_error("seek elf file failed\n");
		return NULL;
	}
	
	ret = fs_read(file, (char *)header, hdr.e_shnum * hdr.e_shentsize);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	ret = fs_seek(file, header[hdr.e_shstrndx].sh_offset);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	ret = fs_read(file, str, 4096);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	elf_file = parse_elf_info(header, hdr.e_shnum, str);
	if ((elf_file != NULL)  &&  (elf_file->alloc_size == 0) ) {
		release_elf_file(elf_file);
		elf_file =NULL;
	}

go_out:
	kfree(str);

err_str_mem:
	kfree(header);

	return elf_file;
}

