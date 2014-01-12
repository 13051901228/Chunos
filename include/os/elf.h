#ifndef _ELF_H
#define _ELF_H

#include <os/types.h>
#include <os/fs.h>

typedef unsigned char	elf_byte;
typedef unsigned short	elf_half;
typedef unsigned int	elf_word;
typedef signed int	elf_sword;
typedef unsigned int	elf_addr;
typedef unsigned int	elf_off;

#define EI_NIDENT	16

typedef struct _elf_header {
	char		e_ident[EI_NIDENT];
	elf_half	e_type;
	elf_half	e_machine;
	elf_word	e_version;
	elf_addr	e_entry;
	elf_off		e_phoff;				/*offset of the program table header in the file*/
	elf_off		e_shoff;				/*offset of the section header table in the file*/
	elf_word	e_flags;
	elf_half	e_ehsize;				
	elf_half	e_phentsize;				/*size of a program header*/
	elf_half	e_phnum;				/*how many program header in this obj file*/
	elf_half	e_shentsize;				/*size of section header*/
	elf_half	e_shnum;				/*how many section header in this obj file*/
	elf_half	e_shstrndx;				/*index point to the string header in the file*/
} elf_header;

#define PF_R		0X04
#define PF_W		0X02
#define PF_X		0X01

/* sh_type */
#define SHT_NULL	0
#define SHT_PROGBITS	1
#define SHT_SYMTAB	2
#define SHT_STRTAB	3
#define SHT_RELA	4
#define SHT_HASH	5
#define SHT_DYNAMIC	6
#define SHT_NOTE	7
#define SHT_NOBITS	8
#define SHT_REL		9
#define SHT_SHLIB	10
#define SHT_DYNSYM	11
#define SHT_NUM		12
#define SHT_LOPROC	0x70000000
#define SHT_HIPROC	0x7fffffff
#define SHT_LOUSER	0x80000000
#define SHT_HIUSER	0xffffffff

/* sh_flags */
#define SHF_WRITE	0x1
#define SHF_ALLOC	0x2
#define SHF_EXECINSTR	0x4
#define SHF_MASKPROC	0xf0000000

typedef struct _program_header {
	elf_word	p_type;
	elf_off		p_offset;
	elf_addr	p_vaddr;
	elf_addr	p_paddr;
	elf_word	p_filesz;
	elf_word	p_memz;
	elf_word	p_flags;
	elf_word	p_align;
} elf_program_header;

typedef struct _section_header {
	elf_word	sh_name;			/*index of the section which can find the section's name'*/
	elf_word	sh_type;			/*type of the section*/
	elf_word	sh_flags;
	elf_addr	sh_addr;			
	elf_off		sh_offset;			/*offset of the section in the file*/
	elf_word	sh_size;			/*section size */
	elf_word	sh_link;		
	elf_word	sh_info;
	elf_word	sh_addralign;
	elf_word	sh_entsize;
} elf_section_header;

/* special section indexes */
#define SHN_UNDEF	0
#define SHN_LORESERVE	0xff00
#define SHN_LOPROC	0xff00
#define SHN_HIPROC	0xff1f
#define SHN_ABS		0xfff1
#define SHN_COMMON	0xfff2
#define SHN_HIRESERVE	0xffff
#define	EI_MAG0		0		/* e_ident[] indexes */
#define	EI_MAG1		1
#define	EI_MAG2		2
#define	EI_MAG3		3
#define	EI_CLASS	4
#define	EI_DATA		5
#define	EI_VERSION	6
#define	EI_OSABI	7
#define	EI_PAD		8

#define	ELFMAG0		0x7f		/* EI_MAG */
#define	ELFMAG1		'E'
#define	ELFMAG2		'L'
#define	ELFMAG3		'F'
#define	ELFMAG		"\177ELF"
#define	SELFMAG		4

#define	ELFCLASSNONE	0		/* EI_CLASS */
#define	ELFCLASS32	1
#define	ELFCLASS64	2
#define	ELFCLASSNUM	3

#define ELFDATANONE	0		/* e_ident[EI_DATA] */
#define ELFDATA2LSB	1
#define ELFDATA2MSB	2

#define EV_NONE		0		/* e_version, EI_VERSION */
#define EV_CURRENT	1
#define EV_NUM		2

typedef enum _section_id {
	SECTION_TEXT,
	SECTION_DATA,
	SECTION_BSS,
	SECTION_MAX
} section_id;

struct elf_section;
struct elf_section {
	char name[32];		/*section id,text data or bss*/
	u32 offset;		/*section offset in the elf file*/
	u32 size;		/*section size*/
	u32 load_addr;
	struct elf_section *next;
};

struct elf_file {
	u32 alloc_size;
	struct elf_section *head;
};

void release_elf_file(struct elf_file *file);
struct elf_file *get_elf_info(struct file *file);

#endif
