#include <unistd.h>
#include <elf.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

#if defined(__LP64__)
# define ElfW(type) Elf64_ ## type
#else
# define ElfW(type) Elf32_ ## type
#endif

void sym_table(FILE *obj, ElfW(Ehdr) header, void *bytes)
{
	ElfW(Shdr) shdr;
	char *cbytes = (char *)bytes;
	size_t dynstr_off = 0;
	size_t dynsym_off = 0;
	size_t dynsym_sz = 0;
	
	fseek(obj, header.e_shoff, SEEK_SET);
	for (int i = 0; i < header.e_shnum; i++)
	{
		fread(&shdr, 1, sizeof(shdr), obj);
		switch(shdr.sh_type)
		{
			case SHT_SYMTAB:
			case SHT_STRTAB:
				if (!dynstr_off)
				{
					printf("String table at %zd\n", shdr.sh_offset);
					dynstr_off = shdr.sh_offset;
				}
				break;
			case SHT_DYNSYM:
				dynsym_off = shdr.sh_offset;
				dynsym_sz = shdr.sh_size;
				printf("Dynsym table at %zd, size %zd bytes\n", dynsym_off, dynsym_sz);
				break;
			defdault: break;
		}
	}
	fseek(obj, 0, SEEK_SET);

	for (size_t i = 0; i * sizeof(ElfW(Sym)) < dynsym_sz; i++)
	{
		ElfW(Sym) sym;
		size_t absoff = dynsym_off + i * sizeof(sym);

		//memmove(&sym, cbytes + absoff, sizeof(sym));
		fseek(obj, absoff, SEEK_SET);
		fread(&sym, 1, sizeof(sym), obj);
		printf("SYMBOL TABLE ENTRY %zd\n", i);
		printf("st_name = %d", sym.st_name);
		if(sym.st_name != 0)
			printf(" - (%s)", cbytes + dynstr_off + sym.st_name);
		printf("\nst_info = %d\n", sym.st_info);
		printf("st_other = %d\n", sym.st_other);
		printf("st_shndx = %d\n", sym.st_shndx);
		printf("st_value = %p\n", sym.st_value);
		printf("st_size = %zd\n", sym.st_size);
	}
}

int required_space(FILE *obj, ElfW(Ehdr) header, ElfW(Phdr) progheader)
{
	fseek(obj, header.e_phoff, SEEK_SET);
	int size = 0;
	for (int i = 0; i < header.e_phnum; i++)
	{
		fread(&progheader, 1, sizeof(ElfW(Phdr)), obj);
		if (progheader.p_type == PT_LOAD)
			size += progheader.p_memsz;
	}
	fseek(obj, 0, SEEK_SET);
	
	return size;
}

char *os_abi(unsigned char c)
{
	switch(c)
	{
		case ELFOSABI_SYSV: return "UNIX System V ABI"; break;
		case ELFOSABI_HPUX: return "HP-UX ABI"; break;
		case ELFOSABI_NETBSD: return "NetBSD ABI"; break;
		case ELFOSABI_LINUX: return "Linux ABI"; break;
		case ELFOSABI_SOLARIS: return "Solaris ABI"; break;
		case ELFOSABI_IRIX: return "IRIX ABI"; break;
		case ELFOSABI_FREEBSD: return "FreeBSD ABI"; break;
		default: return "unknown";
	}
}

char *file_type(unsigned char c)
{
	switch(c)
	{
		case ET_REL: return "Relocatable file"; break;
		case ET_EXEC: return "Executable file"; break;
		case ET_DYN: return "Position-Independent Executable file"; break;
		case ET_CORE: return "Core file"; break;
		default: return "unknown";
	}
}

int main(int ac, char **av)
{
	if (ac != 2)
	{
		printf("Usage: %s <file>\n", av[0]);
		return 1;
	}

	ElfW(Ehdr) header;
	ElfW(Phdr) progheader;
	FILE *obj = fopen(av[1], "rb");
	if (!obj)
	{
		printf("Can't open a file\n");
		return 1;
	}
	
	fseek(obj, 0, SEEK_END);
	long obj_size = ftell(obj);
	void *bytes = mmap(NULL, (size_t) obj_size, PROT_READ, MAP_PRIVATE, fileno(obj), 0);
	printf("File size: %d\n", obj_size);
	fseek(obj, 0, SEEK_SET);
	fread(&header, 1, sizeof(header), obj);

	printf("Magic: %x %x %x %x\n", header.e_ident[0], header.e_ident[1],header.e_ident[2], header.e_ident[3]);
	printf("OS/ABI: %s\n", os_abi(header.e_ident[7]));
	printf("Entry: %p\n", header.e_entry);
	printf("Type: %s\n", file_type(header.e_type));
	sym_table(obj, header, bytes);
	printf("Required space to load: %d bytes\n", required_space(obj, header, progheader));
	
	fclose(obj);
}
