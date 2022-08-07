#include <unistd.h>
#include <elf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int required_space(FILE *obj, Elf64_Ehdr header, Elf64_Phdr progheader)
{
	fseek(obj, header.e_phoff, SEEK_SET);
	int size = 0;
	for (int i = 0; i < header.e_phnum; i++)
	{
		fread(&progheader, 1, sizeof(Elf64_Phdr), obj);
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

	Elf64_Ehdr header;
	Elf64_Phdr progheader;
	FILE *obj = fopen(av[1], "rb");
	if (!obj)
	{
		printf("Can't open a file\n");
		return 1;
	}

	fread(&header, 1, sizeof(header), obj);

	printf("Magic: %x %x %x %x\n", header.e_ident[0], header.e_ident[1],header.e_ident[2], header.e_ident[3]);
	printf("OS/ABI: %s\n", os_abi(header.e_ident[7]));
	printf("Entry: %p\n", header.e_entry);
	printf("Type: %s\n", file_type(header.e_type));
	printf("Required space to load: %d bytes\n", required_space(obj, header, progheader));
	
	fclose(obj);
}
