#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <elf.h>
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

static void parse_options(int argc, char *argv[], Arguments *args)
{
	int c = 0;
	char shortopts[64] = "";
	struct option longopts[sizeof(opts)/sizeof(opts[0]) - 1];

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
				args->display = DSP_FILE_HEADER;
				break;
			case 'S':
				args->display = DSP_SECTION_HEADERS;
				break;
			case 'x':
				args->display = DSP_HEX_DUMP;
				if((optarg[0] == '.') && (isalpha(optarg[1])))
					strcpy(args->section_str, optarg);
				else
					args->section_ind = atoi(optarg);
				break;
			case 's':
				args->display = DSP_SYMS;
				break;
			case 'r':
				args->display = DSP_RELOCS;
				break;
			case 'H':
				print_help(argv[0]);
				exit(0);
			default:
				print_help(argv[0]);
				exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	int i, fd;
	char *table = NULL;
	Arguments args = { .display = DSP_NONE, .section_str = "" };
	Elf32_Ehdr *ehdr;
	Elf32_Shdr **shdr;

	if(argc <= 2)
	{
		print_help(argv[0]);
		return 1;
	}

	parse_options(argc, argv, &args);
	fd = open(argv[argc - 1], O_RDONLY); // Le dernier argument est le nom du fichier
	if(fd < 0)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier %s.\n", argv[2]);
		return 2;
	}

	/* Lecture en-tête */
	ehdr = malloc(sizeof(Elf32_Ehdr));
	read_header(fd, ehdr);

	/* Lecture sections */
	shdr = malloc(sizeof(Elf32_Shdr*) * ehdr->e_shnum);
	for(i = 0; i < ehdr->e_shnum; i++)
		shdr[i] = malloc(sizeof(Elf32_Shdr));
	read_section_header(fd, ehdr, shdr);

	/* Récupération table des noms */
	table = get_section_name_table(fd, ehdr, shdr);
	if(args.section_str[0] != '\0')
	{
		for(i = 0; strcmp(args.section_str, get_section_name(shdr, table, i)) && i < ehdr->e_shnum - 1; i++);
		if((strcmp(args.section_str, get_section_name(shdr, table, i))) && (i == ehdr->e_shnum - 1))
		{
			fprintf(stderr, "La section '%s' n'existe pas.\n", args.section_str);
			return 3;
		}
		args.section_ind = i;
	}
	else if(args.section_ind >= ehdr->e_shnum)
	{
		fprintf(stderr, "La section %i n'existe pas.\n", args.section_ind);
		return 3;
	}

	switch(args.display)
	{
		case DSP_FILE_HEADER:
			dump_header(ehdr);
			break;
		case DSP_SECTION_HEADERS:
			dump_section_header(ehdr, shdr, table);
			break;
		case DSP_HEX_DUMP:
			dump_section(fd, ehdr, shdr, args.section_ind);
			break;
		default:
			fprintf(stderr, "Cette option n'est pas encore implémentée.\n");
	}

	close(fd);
	for(int i = 0; i < ehdr->e_shnum; i++)
		free(shdr[i]);
	free(ehdr);
	free(shdr);
	free(table);

	return 0;
}

int read_header(int fd, Elf32_Ehdr *ehdr)
{
	read(fd, ehdr->e_ident, EI_NIDENT);
	if(ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3 || ehdr->e_ident[EI_CLASS] != ELFCLASS32)
	{
		fprintf(stderr, "Le fichier n'est pas de type ELF32.\n");
		exit(3);
	}

	read(fd, &ehdr->e_type,      sizeof(ehdr->e_type));
	read(fd, &ehdr->e_machine,   sizeof(ehdr->e_machine));
	read(fd, &ehdr->e_version,   sizeof(ehdr->e_version));
	read(fd, &ehdr->e_entry,     sizeof(ehdr->e_entry));
	read(fd, &ehdr->e_phoff,     sizeof(ehdr->e_phoff));
	read(fd, &ehdr->e_shoff,     sizeof(ehdr->e_shoff));
	read(fd, &ehdr->e_flags,     sizeof(ehdr->e_flags));
	read(fd, &ehdr->e_ehsize,    sizeof(ehdr->e_ehsize));
	read(fd, &ehdr->e_phentsize, sizeof(ehdr->e_phentsize));
	read(fd, &ehdr->e_phnum,     sizeof(ehdr->e_phnum));
	read(fd, &ehdr->e_shentsize, sizeof(ehdr->e_shentsize));
	read(fd, &ehdr->e_shnum,     sizeof(ehdr->e_shnum));
	read(fd, &ehdr->e_shstrndx,  sizeof(ehdr->e_shstrndx));

	return 0;
}

int read_section_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr)
{
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		lseek(fd, ehdr->e_shoff + i * ehdr->e_shentsize, SEEK_SET);
		read(fd, &shdr[i]->sh_name,      sizeof(shdr[i]->sh_name));
		read(fd, &shdr[i]->sh_type,      sizeof(shdr[i]->sh_type));
		read(fd, &shdr[i]->sh_flags,     sizeof(shdr[i]->sh_flags));
		read(fd, &shdr[i]->sh_addr,      sizeof(shdr[i]->sh_addr));
		read(fd, &shdr[i]->sh_offset,    sizeof(shdr[i]->sh_offset));
		read(fd, &shdr[i]->sh_size,      sizeof(shdr[i]->sh_size));
		read(fd, &shdr[i]->sh_link,      sizeof(shdr[i]->sh_link));
		read(fd, &shdr[i]->sh_info,      sizeof(shdr[i]->sh_info));
		read(fd, &shdr[i]->sh_addralign, sizeof(shdr[i]->sh_addralign));
		read(fd, &shdr[i]->sh_entsize,   sizeof(shdr[i]->sh_entsize));
	}

	return 0;
}

char *get_section_name_table(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr)
{
	int pos;
	const unsigned offset = shdr[ehdr->e_shstrndx]->sh_offset;
	char *table = (char *) malloc(sizeof(char) * offset);

	pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, offset, SEEK_SET);
	read(fd, table, offset);

	lseek(fd, pos, SEEK_SET);
	return table;
}

char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index)
{
	return &(table[shdr[index]->sh_name]);
}

int read_relocation_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr)
{
	for(int i = 0; i < ehdr->e_shnum; i++)
	{
		//printf("%i : %i (%i)\n", i, shdr[i]->sh_type, SHT_REL);
		if(shdr[i]->sh_type == SHT_REL)
			printf("%i\n", i);
	}

	return 0;
}

void dump_header(Elf32_Ehdr *ehdr)
{
	printf("En-tête ELF :\n");
	printf("Magique : ");
	for(int i = 0; i < EI_NIDENT; i++)
		printf("%02x ", ehdr->e_ident[i]);

	printf("\nClasse : ");
	switch(ehdr->e_ident[EI_CLASS])
	{
		case ELFCLASS32:
			printf("ELF32\n");
			break;
		case ELFCLASS64:
			printf("ELF64\n");
			break;
		default:
			printf("Inconnue\n");
	}

	printf("Données : ");
	switch(ehdr->e_ident[EI_DATA])
	{
		case ELFDATA2LSB:
			printf("Little endian\n");
			break;
		case ELFDATA2MSB:
			printf("Big endian\n");
			break;
		default:
			printf("Inconnue\n");
	}

	printf("Version : %i\n", ehdr->e_ident[EI_VERSION]);

	printf("OS/ABI : ");
	switch(ehdr->e_ident[EI_OSABI])
	{
		case ELFOSABI_NONE:
			printf("UNIX - System V\n");
			break;
		case ELFOSABI_LINUX:
			printf("GNU/Linux\n");
			break;
		default:
			printf("Inconnu\n");
	}

	printf("Type : ");
	switch(ehdr->e_type)
	{
		case ET_REL:
			printf("Repositionable\n");
			break;
		case ET_EXEC:
			printf("Executable\n");
			break;
		case ET_DYN:
			printf("Objet partagé\n");
			break;
		case ET_CORE:
			printf("Fichier Core\n");
			break;
		default:
			printf("Inconnu\n");
	}

	printf("Machine : ");
	switch(ehdr->e_machine)
	{
		case EM_ARM:
			printf("ARM (32 bits)\n");
			break;
		case EM_AARCH64:
			printf("ARM  (64 bits)\n");
			break;
		case EM_386:
			printf("x86 (32 bits)\n");
			break;
		case EM_X86_64:
			printf("x86_64 (64 bits)\n");
			break;
		default:
			printf("Inconnue\n");
	}

	printf("Version : %#x\n", ehdr->e_version);
	printf("Adresse du point d'entrée : %#x\n", ehdr->e_entry);
	printf("Début des en-têtes de programme : %i (octets dans le fichier)\n", ehdr->e_phoff);
	printf("Début des en-têtes de section : %i (octets dans le fichier)\n", ehdr->e_shoff);
	printf("Fanions : %#x\n", ehdr->e_flags);
	printf("Taille de cet en-tête : %i (octets)\n", ehdr->e_ehsize);
	printf("Taille de l'en-tête du programme : %i (octets)\n", ehdr->e_phentsize);
	printf("Nombre d'en-tête du programme : %i\n", ehdr->e_phnum);
	printf("Taille des en-têtes de section : %i (octets)\n", ehdr->e_shentsize);
	printf("Nombre d'en-têtes de section : %i\n", ehdr->e_shnum);
	printf("Table d'indexes des chaînes d'en-tête de section : %i\n", ehdr->e_shstrndx);
}

void dump_section (int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, unsigned index){

	char *table = get_section_name_table(fd, ehdr, shdr);
	char *name = get_section_name(shdr, table, index);

	printf("\nAffichage hexadécimal de la section « %s »:\n\n", name);

	Elf32_Shdr *shdrToDisplay = shdr[index];

	unsigned char buffer[16];

	lseek(fd, ehdr->e_entry + shdrToDisplay->sh_offset, SEEK_SET);

	int i;
	int j;
	int k;
	int nbByte = 0;
	for (i=0; i<shdrToDisplay->sh_size; i+=16){
		printf("  0x%08x ", i);

		for (j=0; j<4; j++){

			for (k=0; k<4; k++){
				if ( nbByte < shdrToDisplay->sh_size ){
					read(fd, &buffer, 1);
					printf("%02x", *buffer);
					nbByte++;
				}
			}
			printf(" ");
		}
		printf("\n");
	}
}

static char *section_type_to_string(Elf32_Word type)
{
	int i;
	const struct { Elf32_Word type_ind; char *type_str; } types[] =
	{
		{ SHT_NULL,           "NULL"           },
		{ SHT_PROGBITS,       "PROGBITS"       },
		{ SHT_SYMTAB,         "SYMTAB"         },
		{ SHT_STRTAB,         "STRTAB"         },
		{ SHT_RELA,           "RELA"           },
		{ SHT_HASH,           "HASH"           },
		{ SHT_DYNAMIC,        "DYNAMIC"        },
		{ SHT_NOTE,           "NOTE"           },
		{ SHT_NOBITS,         "NOBITS"         },
		{ SHT_REL,            "REL"            },
		{ SHT_SHLIB,          "SHLIB"          },
		{ SHT_DYNSYM,         "DYNSYM"         },
		{ SHT_INIT_ARRAY,     "INIT_ARRAY"     },
		{ SHT_FINI_ARRAY,     "FINI_ARRAY"     },
		{ SHT_PREINIT_ARRAY,  "PREINIT_ARRAY"  },
		{ SHT_GROUP,          "GROUP"          },
		{ SHT_SYMTAB_SHNDX,   "SYMTAB_SHNDX"   },
		{ SHT_GNU_ATTRIBUTES, "GNU_ATTRIBUTES" },
		{ SHT_GNU_HASH,       "GNU_HASH"       },
		{ SHT_GNU_LIBLIST,    "GNU_LIBLIST"    },
		{ SHT_GNU_verdef,     "VERDEF"         },
		{ SHT_GNU_verneed,    "VERNEED"        },
		{ SHT_GNU_versym,     "VERSYM"         },
		{ SHT_HIUSER,         "UNKNOWN"        }
	};

	for(i = 0; (type != types[i].type_ind) && (types[i].type_ind != SHT_HIUSER); i++);
	return types[i].type_str;
}

static char *flags_to_string(Elf32_Word flags)
{
	static char buff[3];
	memset(buff, 0, 3);

	if(flags & SHF_WRITE)
		strcat(buff, "W");
	if(flags & SHF_ALLOC)
		strcat(buff, "A");
	if(flags & SHF_EXECINSTR)
		strcat(buff, "X");
	if(flags & SHF_MERGE)
		strcat(buff, "M");
	if(flags & SHF_STRINGS)
		strcat(buff, "S");
	if(flags & SHF_INFO_LINK)
		strcat(buff, "I");
	if(flags & SHF_LINK_ORDER)
		strcat(buff, "L");
	if(flags & SHF_GROUP)
		strcat(buff, "G");
	if(flags & SHF_TLS)
		strcat(buff, "T");

	return buff;
}

void dump_section_header(Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, char *table)
{
	printf("Il y a %i en-têtes de section, débutant à l'adresse de décalage %#x :\n\n", ehdr->e_shnum, ehdr->e_shoff);
	printf("En-têtes de section :\n");
	printf("[%2s] %-18s %-14s %8s %6s %6s %2s %2s %2s %2s %2s\n",
		"Nr", "Nom", "Type", "Adresse", "Déc.", "Taille", "ES", "Flg", "Lk", "Inf", "Al");

	for(int i = 0; i < ehdr->e_shnum; i++)
		printf("[%2i] %-18s %-14s %08x %06x %06x %02x %2s %2i %2i %2i\n", i, get_section_name(shdr, table, i),
			section_type_to_string(shdr[i]->sh_type), shdr[i]->sh_addr,
			shdr[i]->sh_offset, shdr[i]->sh_size, shdr[i]->sh_entsize, flags_to_string(shdr[i]->sh_flags),
			shdr[i]->sh_link, shdr[i]->sh_info, shdr[i]->sh_addralign);
	printf("\nListe des fanions :\n");
	printf("  W : écriture\n");
	printf("  A : allocation\n");
	printf("  X : exécution\n");
	printf("  M : fusion\n");
	printf("  S : chaînes\n");
	printf("  I : info\n");
	printf("  L : ordre des liens\n");
	printf("  G : groupes\n");
	printf("  T : TLS\n");
}
