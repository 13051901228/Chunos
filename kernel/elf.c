#include <os/elf.h>
#include <os/slab.h>
#include <os/types.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/file.h>

int elf_load_elf_image(struct elf_file *efile)
{
	int i, ret, re_seek = 0;
	struct elf_section *section;
	struct elf_section *section_pre = NULL;

	for (i = 0; i < efile->section_nr; i++) {
		section = &efile->sections[i];

		if (section->type & SHT_NOBITS) {
			/* this is a BSS section */
			memset(section->load_addr, 0, section->size);
			re_seek = 0;
			section_pre = NULL;
			continue;
		}

		if (section_pre == NULL) {
			re_seek = 1;
		} else {
			if ((section_pre->load_addr + section_pre->size)
					!= section->load_addr)
				re_seek = 1;
		}

		if (re_seek) {
			ret = kernel_seek(efile->file, section->offset, SEEK_SET);
			if (ret < 0)
				return -EIO;
		}

		ret = kernel_read(efile->file,
			(char *)section->load_addr, section->size);
		if (ret != section->size)
			return -EIO;

		re_seek = 0;
		section_pre = section;
	}

	return 0;
}

static void __parse_elf_info(elf_section_header *header,
		struct elf_file *efile, int section_num, char *str)
{
	int i;
	elf_section_header *tmp = header;
	struct elf_section *section = efile->sections;

	for (i = 0; i < section_num; i++) {
		if (tmp->sh_flags & SHF_ALLOC) {
			if (!efile->elf_base) {
				efile->elf_base = min_align(tmp->sh_addr, PAGE_SIZE);
				efile->elf_size += tmp->sh_addr - efile->elf_base;
			}

			section->offset = tmp->sh_offset;
			section->load_addr = tmp->sh_addr;
			section->size = tmp->sh_size;
			section->type = tmp->sh_type;
			section->flag = tmp->sh_flags;
			efile->elf_size += section->size;
			section++;
		}

		tmp++;
	}
}

static struct elf_file *parse_elf_info(elf_section_header *header,
				int section_num, char *str)
{
	int i;
	elf_section_header *tmp = header;
	struct elf_file *elf_file =NULL;

	elf_file = (struct elf_file *) kzalloc(sizeof(struct elf_file), GFP_KERNEL);
	if (elf_file == NULL)
		return NULL;

	for (i = 0; i < section_num; i ++) {
		if (tmp->sh_flags & SHF_ALLOC)
			elf_file->section_nr++;
		tmp++;
	}

	if (!elf_file->section_nr)
		goto err_out;

	elf_file->sections = kzalloc(sizeof(struct elf_section) *
			elf_file->section_nr, GFP_KERNEL);
	if (!elf_file->sections)
		goto err_out;

	__parse_elf_info(header, elf_file, section_num, str);

	return elf_file;

err_out:
	kfree(elf_file);
	return NULL;
}

void inline release_elf_file(struct elf_file *efile)
{
	if (efile) {
		kernel_close(efile->file);
		kfree(efile->sections);
		kfree(efile);
	}
}

size_t inline elf_memory_size(struct elf_file *efile)
{
	return efile->elf_size;
}

unsigned long inline elf_get_elf_base(struct elf_file *efile)
{
	return efile->elf_base;
}

struct elf_file *get_elf_info(char *name)
{
	elf_header hdr;
	int ret = -1;
	elf_section_header *header;
	char *str;
	struct file *file;
	struct elf_file *elf_file;

	file = kernel_open(name, O_RDONLY);
	if (!file)
		return NULL;

	ret = kernel_read(file, (char *)&hdr, sizeof(elf_header));
	if (ret < 0) {
		kernel_error("read elf file error offset\n");
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


	header = (elf_section_header *)
		kzalloc(hdr.e_shnum * hdr.e_shentsize, GFP_KERNEL);
	if (header == NULL)
		return NULL;

	str = (char *)kmalloc(4096, GFP_KERNEL);
	if (str == NULL)
		goto err_str_mem;

	ret = kernel_seek(file, hdr.e_shoff, SEEK_SET);
	if (ret < 0) {
		kernel_error("seek elf file failed\n");
		return NULL;
	}
	
	ret = kernel_read(file, (char *)header, hdr.e_shnum * hdr.e_shentsize);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	ret = kernel_seek(file, header[hdr.e_shstrndx].sh_offset, SEEK_SET);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	ret = kernel_read(file, str, 4096);
	if (ret < 0) {
		elf_file = NULL;
		goto go_out;
	}

	elf_file = parse_elf_info(header, hdr.e_shnum, str);
	if (elf_file == NULL)
		goto go_out;

	elf_file->entry_point_address = hdr.e_entry;
	elf_file->file = file;

go_out:
	kfree(str);

err_str_mem:
	kfree(header);

	return elf_file;
}

