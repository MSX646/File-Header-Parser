#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach-o/loader.h>
#include <mach-o/swap.h>
#include <mach-o/fat.h>

/* CPU TYPE */
struct _cpu_type_names
{
	cpu_type_t cputype;
	const char *cpu_name;
};

static struct _cpu_type_names cpu_type_names[] = {
	{CPU_TYPE_I386, "i386"}, {CPU_TYPE_X86_64, "x86_64"}, {CPU_TYPE_ARM, "ARM"}, {CPU_TYPE_ARM64, "ARM64"}
};

static const char *cpu_type(cpu_type_t cpu_type)
{
	static int cpu_type_size = sizeof(cpu_type_names) / sizeof(struct _cpu_type_names);

	for (int i = 0; i < cpu_type_size; i++)
	{
		if (cpu_type == cpu_type_names[i].cputype)
			return cpu_type_names[i].cpu_name;
	}

	return "unknown";
}


/* READING HEADER */
uint32_t read_magic(FILE *obj, int offset)
{
	uint32_t magic;
	
	fseek(obj, offset, SEEK_SET); // SPECIFY OFFSET EXPLICITLY TO READ WHAT WE WANT
	fread(&magic, sizeof(uint32_t), 1, obj);
	
	return magic;
}

bool is_magic64(uint32_t magic)
{
	return magic == MH_MAGIC_64 || magic == MH_CIGAM_64;
}

bool byte_order(uint32_t magic)
{
	return magic == MH_CIGAM || magic == MH_CIGAM_64;
}

bool is_fat(uint32_t magic)
{
  return magic == FAT_MAGIC || magic == FAT_CIGAM;
}

void *load_bytes(FILE *obj, int offset, int size)
{
	void *buf = calloc(1, size);
	fseek(obj, offset, SEEK_SET);
	fread(buf, size, 1, obj);

	return buf;
}

void dump_seg_cmds(FILE *obj, int offset, bool is_swap, uint32_t ncmds)
{
	int actual = offset;
	for (int i = 0; i < ncmds; i++)
	{
		struct load_command *cmd = load_bytes(obj, actual, sizeof(struct load_command));
		if (is_swap)
			swap_load_command(cmd, 0);

		if (cmd->cmd == LC_SEGMENT_64)
		{
			struct segment_command_64 *segment = load_bytes(obj, actual, sizeof(struct segment_command_64));
			if (is_swap)
				swap_segment_command_64(segment, 0);

			printf("segname: %s\n", segment->segname);
			free(segment);
		}
		else if (cmd->cmd == LC_SEGMENT)
		{
			struct segment_command *segment = load_bytes(obj, actual, sizeof(struct segment_command));
			if (is_swap)
				swap_segment_command(segment, 0);

			printf("segname: %s\n", segment->segname);
			free(segment);
		}
		
		actual += cmd->cmdsize;
		free(cmd);
	}
}

void dump_header(FILE *obj, int offset, bool is_64, bool is_swap)
{
	uint32_t ncmds;
	int cmd_offset = offset;

	if (is_64)
	{
		int header_size = sizeof(struct mach_header_64);
		struct mach_header_64 *header = load_bytes(obj, offset, header_size);
		if (is_swap)
			swap_mach_header_64(header, 0);
		
		ncmds = header->ncmds;
		cmd_offset += header_size;

		printf("Arch: %s\n", cpu_type(header->cputype));

		free(header);
	}
	else
	{
		int header_size = sizeof(struct mach_header);
		struct mach_header *header = load_bytes(obj, offset, header_size);
		if (is_swap)
			swap_mach_header(header, 0);
		
		ncmds = header->ncmds;
		cmd_offset += header_size;
	
		printf("Arch: %s\n", cpu_type(header->cputype));

		free(header);
	}
	dump_seg_cmds(obj, cmd_offset, is_swap, ncmds);
}

void dump_fat_header(FILE *obj, bool is_swap)
{
	int header_size = sizeof(struct fat_header);
	int arch_size = sizeof(struct fat_arch);

	struct fat_header *header = load_bytes(obj, 0, header_size);
	if (is_swap)
		swap_fat_header(header, 0);
	
	int arch_offset = header_size;
	for (uint32_t i = 0U; i < header->nfat_arch; i++)
	{
		struct fat_arch *arch = load_bytes(obj, arch_offset, arch_size);

		if (is_swap)
			swap_fat_arch(arch, 1, 0);

		int mach_offset = arch->offset;
		free(arch);
		arch_offset += arch_size;

		uint32_t magic = read_magic(obj, mach_offset);
		bool is_64 = is_magic64(magic);
		bool is_swap_mach = byte_order(magic);
		dump_header(obj, mach_offset, is_64, is_swap_mach);
	}
	free(header);
}

void dump_segs(FILE *obj)
{
	uint32_t magic = read_magic(obj, 0);
	bool is_64 = is_magic64(magic);
	bool is_swap = byte_order(magic);
	bool fat = is_fat(magic);
	printf("Magic: %" PRIu32 "\n", magic);
	if (fat)
		dump_fat_header(obj, is_swap);
	else
		dump_header(obj, 0, is_64, is_swap);
}

int main(int ac, char **av)
{
	if (ac != 2 || !strcmp(av[1], "-h") || !strcmp(av[1], "--help"))
	{
		printf("Usage: ./dumper <mach-o file>\n");
		return 1;
	}
	const char *file = av[1];
	FILE *obj = fopen(file, "rb"); // BINARY READ

	dump_segs(obj);
	fclose(obj);

	return 0;
}
