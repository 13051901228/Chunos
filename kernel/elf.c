#include <os/elf.h>
#include <os/slab.h>
#include <os/types.h>
#include <os/mm.h>
#include <os/string.h>
#include <os/errno.h>
#include <os/printk.h>
#include <os/file.h>

struct elf_file *dup_elf_info(struct elf_file *src)
{
	struct elf_file *elf_file;
	int section_size;

	if (!src)
		return NULL;

	elf_file = kmalloc(sizeof(struct elf_file), GFP_KERNEL);
	if (!elf_file)
		return NULL;

	section_size = sizeof(struct elf_section) * src->section_nr;
	elf_file->sections = kmalloc(section_size, GFP_KERNEL);
	if (!elf_file->sections) {
		kfree(elf_file);
		return NULL;
	}

	elf_file->section_nr = src->section_nr;
	elf_file->entry_point_address = src->entry_point_address;
	memcpy(src->sections, elf_file->sections, section_size);

	return elf_file;
}

static struct elf_file *parse_elf_info(elf_section_header *header,
				int section_num, char *str)
{
	elf_section_header *tmp = header;
	int i, nr, max_id = 0;
	char *name;
	struct elf_file *elf_file =NULL;
	struct elf_section *section;
	struct elf_section buf;

	elf_file = (struct elf_file *)
		kzalloc(sizeof(struct elf_file), GFP_KERNEL);
	if (elf_file == NULL)
		return NULL;

	/* calculate how many section need to allocate memory */
	for (i = 0; i < section_num; i++) {
		if (tmp[i].sh_flags & SHF_ALLOC)
			nr++;
	}

	/* alloc memory for the all sections which need memory */
	elf_file->sections =
		kzalloc(sizeof(struct elf_section) * nr, GFP_KERNEL);
	if (elf_file->sections) {
		kfree(elf_file);
		return NULL;
	}

	for (i = 0; i < section_num; i++) {
		/* whether this section needed allocate mem. */
		if (tmp->sh_flags & SHF_ALLOC) {
			name = &str[tmp->sh_name];
			section = &elf_file->sections[elf_file->section_nr];

			section->offset = header->sh_offset;
			section->size = header->sh_size;
			section->load_addr = header->sh_addr;
			strcpy(section->name, name);

			if (section->size > elf_file->sections[max_id].size)
				max_id = elf_file->section_nr;

			elf_file->section_nr++;
		}

		tmp++;
	}

	/* put the max_size section to the last */
	if (max_id != (elf_file->section_nr - 1)) {
		memcpy(&buf, &elf_file->sections[max_id],
				sizeof(struct elf_section));

		memcpy(&elf_file->sections[max_id],
				&elf_file->sections[elf_file->section_nr - 1],
				sizeof(struct elf_section));

		memcpy(&elf_file->sections[elf_file->section_nr - 1], &buf,
				sizeof(struct elf_section));
	}

	for (i = 0; i < elf_file->section_nr; i ++)
		elf_file->elf_size += elf_file->sections[i].size;

	return elf_file;
}

void release_elf_file(struct elf_file *file)
{
	kfree(file->sections);
	kfree(file);
}

size_t elf_memory_size(struct elf_file *efile)
{
	return efile->elf_size;
}

struct elf_file *get_elf_info(struct file *file)
{
	elf_header hdr;
	int ret = -1;
	elf_section_header *header;
	char *str;
	struct elf_file *elf_file;

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
	if (elf_file != NULL) {
		release_elf_file(elf_file);
		elf_file =NULL;
	}

	elf_file->entry_point_address = hdr.e_entry;

go_out:
	kfree(str);

err_str_mem:
	kfree(header);

	return elf_file;
}

