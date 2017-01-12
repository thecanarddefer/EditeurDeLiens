#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>

#include "elf.h"
#include "elf_common.h"
#include "section.h"
#include "symbol.h"
#include "relocation.h"
#include "disp.h"
#include "readelf.h"


static const struct
{
	const char short_opt;
	const char *long_opt;
	const int  need_arg;
	char       *description;
} opts[] =
{
	{ 'h',  "file-header",     no_argument,       "Affiche l'en-tête du fichier ELF"                       },
	{ 'S',  "section-headers", no_argument,       "Affiche les sections de l'en-tête"                      },
	{ 'x',  "hex-dump",        required_argument, "Affiche le contenu en hexadécimal d'une section donnée" },
	{ 's',  "syms",            no_argument,       "Affiche la table des symboles"                          },
	{ 'r',  "relocs",          no_argument,       "Affiche les réalocations (si présentes)"                },
	{ 'A',  "all",             no_argument,       "Similaire à -h -S -s -r"                                },
	{ 'H',  "help",            no_argument,       "Affiche cette aide et quitte"                           },
	{ '\0', NULL,              0,               NULL                                                       }
};

static void print_help(char *prgname)
{
	printf("Usage: %s <option(s)> fichier(s)-elf\n", prgname);
	printf("Afficher les informations à propos du contenu du format des fichiers ELF\n");
	printf("Les options sont :\n");
	for(int i = 0; opts[i].long_opt != NULL; i++)
		printf("  -%c, --%-20s %s\n", opts[i].short_opt, opts[i].long_opt, opts[i].description);
}

static int parse_options(int argc, char *argv[], Arguments *args)
{
	int c = 0, first_file = 1;
	char shortopts[64] = "";
	struct option longopts[sizeof(opts)/sizeof(opts[0]) - 1];
	args->display     = 0;
	args->nb_hexdumps = 0;

	for(int i = 0; opts[i].long_opt != NULL; i++)
	{
		longopts[i].name    = opts[i].long_opt;
		longopts[i].has_arg = opts[i].need_arg;
		longopts[i].flag    = 0;
		longopts[i].val     = opts[i].short_opt;
		shortopts[c++]      = opts[i].short_opt;
		if(opts[i].need_arg)
			shortopts[c++] = ':';
	}

	while((c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
	{
		switch(c)
		{
			case 'h':
				args->display |= DSP_FILE_HEADER;
				break;
			case 'S':
				args->display |= DSP_SECTION_HEADERS;
				break;
			case 'x':
				args->display |= DSP_HEX_DUMP;
				if(isdigit(optarg[0]))
				{
					args->section_ind[args->nb_hexdumps]    = atoi(optarg);
					args->section_str[args->nb_hexdumps][0] = '\0';
				}
				else
				{
					args->section_ind[args->nb_hexdumps] = 0;
					strcpy(args->section_str[args->nb_hexdumps], optarg);
				}
				args->nb_hexdumps++;
				if(argv[first_file][2] == '\0')
					first_file++;
				break;
			case 's':
				args->display |= DSP_SYMS;
				break;
			case 'r':
				args->display |= DSP_RELOCS;
				break;
			case 'A':
				args->display |= DSP_FILE_HEADER | DSP_SECTION_HEADERS | DSP_SYMS | DSP_RELOCS;
				break;
			case 'H':
				print_help(argv[0]);
				exit(0);
			default:
				print_help(argv[0]);
				exit(1);
		}
		first_file++;
	}

	return first_file;
}

static int parse_file(const char *filename, Arguments *args)
{
	int fd;
	Elf32_Ehdr *ehdr;
	Section_Table *secTab;
	symbolTable *symTabFull;
	Data_Rel *drel;

	fd = open(filename, O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier %s.\n", filename);
		return 1;
	}

	ehdr       = read_elf_header(fd);
	secTab     = read_sectionTable(fd, ehdr);
	symTabFull = read_symbolTable(fd, secTab);
	drel       = read_relocationTables(fd, secTab);

	if(args->display & DSP_FILE_HEADER)
		dump_header(ehdr);
	if(args->display & DSP_SECTION_HEADERS)
		dump_section_header(secTab, ehdr->e_shoff);
	if(args->display & DSP_HEX_DUMP)
		for(int h = 0; h < args->nb_hexdumps; h++)
			if(is_valid_section(secTab, args->section_str[h], &args->section_ind[h]))
				dump_section(fd, secTab, args->section_ind[h]);
	if(args->display & DSP_SYMS)
		displ_symbolTable(symTabFull);
	if(args->display & DSP_RELOCS)
		dump_relocation(ehdr, secTab, symTabFull, drel);

	close(fd);
	destroy_elf_header(ehdr);
	destroy_sectionTable(secTab);
	destroy_symbolTable(symTabFull);
	destroy_relocationTables(drel);

	return 0;
}

int main(int argc, char *argv[])
{
	int first_filename, ret = 0;
	Arguments args;

	if(argc <= 2)
	{
		print_help(argv[0]);
		return 1;
	}

	first_filename = parse_options(argc, argv, &args);
	for(int i = first_filename; i < argc; i++)
	{
		if(first_filename < argc - 1)
			printf("Fichier \x1b[1m%s\x1b[0m :\n\n", argv[i]);
		ret += parse_file(argv[i], &args);
		printf("\n\n");
	}

	return ret;
}
