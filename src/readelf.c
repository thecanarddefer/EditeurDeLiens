#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <linux/elf.h>
#include "readelf.h"


static enum DSP display = DSP_NONE;
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

static void parse_options(int argc, char *argv[])
{
	int c = 0;
	char shortopts[64] = "", *arg;
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
				display = DSP_FILE_HEADER;
				break;
			case 'S':
				display = DSP_SECTION_HEADERS;
				break;
			case 'x':
				display = DSP_HEX_DUMP;
				arg = optarg;
				//TODO: Interpréter l'argument
				break;
			case 's':
				display = DSP_SYMS;
				break;
			case 'r':
				display = DSP_RELOCS;
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
	int fd;
	char *table = NULL;
	Elf32_Ehdr *ehdr;
	Elf32_Shdr **shdr;

	if(argc <= 2)
	{
		print_help(argv[0]);
		return 1;
	}

	parse_options(argc, argv);
	fd = open(argv[argc - 1], O_RDONLY); // Le dernier argument est le nom du fichier
	if(fd < 0)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier %s.\n", argv[2]);
		return 2;
	}

	ehdr = malloc(sizeof(Elf32_Ehdr));
	read_header(fd, ehdr);

	shdr = malloc(sizeof(Elf32_Shdr*) * ehdr->e_shnum);
	for(int i = 0; i < ehdr->e_shnum; i++)
		shdr[i] = malloc(sizeof(Elf32_Shdr));
	read_section_header(fd, ehdr, shdr);

	switch(display)
	{
		case DSP_FILE_HEADER:
			dump_header(ehdr);
			break;
		case DSP_SECTION_HEADERS:
			table = get_section_name_table(fd, ehdr, shdr);
			dump_section_header(ehdr, shdr, table);
			free(table);
			break;
		default:
			fprintf(stderr, "Cette option n'est pas encore implémentée.\n");
	}

	close(fd);
	for(int i = 0; i < ehdr->e_shnum; i++)
		free(shdr[i]);
	free(ehdr);
	free(shdr);

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

#define SHF_MERGE            0x10
#define SHF_STRING           0x20
#define SHF_INFO_LINK        0x40
#define SHF_LINK_ORDER       0x80
#define SHF_GROUP            0x200
#define SHF_TLS              0x400
char *flags_to_string(Elf32_Word flags)
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
	if(flags & SHF_STRING)
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
	const char *sht[] = { "NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA, HASH",
	                     "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM", "NUM" };

	printf("Il y a %i en-têtes de section, débutant à l'adresse de décalage %#x :\n\n", ehdr->e_shnum, ehdr->e_shoff);
	printf("En-têtes de section :\n");
	printf("[%2s] %-18s %-8s %8s %6s %6s %2s %2s %2s %2s %2s\n",
		"Nr", "Nom", "Type", "Adresse", "Déc.", "Taille", "ES", "Flg", "Lk", "Inf", "Al");

	for(int i = 0; i < ehdr->e_shnum; i++)
		printf("[%2i] %-18s %-8s %08x %06x %06x %02x %2s %2i %2i %2i\n", i, get_section_name(shdr, table, i),
			shdr[i]->sh_type > SHT_NUM ? "" : sht[shdr[i]->sh_type], shdr[i]->sh_addr,
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

int get_symtab_index(int nbSections, Elf32_Shdr **shdr) {
	int i;
	while (i < nbSections && ((shdr[i]->sh_type != SHT_SYMTAB) || (shdr[i]->sh_type != SHT_DYNSYM)) ) {
		i++;
	}
	if ((shdr[i]->sh_type != SHT_SYMTAB) && (shdr[i]->sh_type != SHT_DYNSYM)) {
		i = -1;
	}
	return i;
}

void read_symbol(int fd, Elf32_Sym *symtab) {
	read(fd, &symtab->st_name, sizeof(symtab->st_name));
	read(fd, &symtab->st_value, sizeof(symtab->st_value));
	read(fd, &symtab->st_size, sizeof(symtab->st_size));
	read(fd, &symtab->st_info, sizeof(symtab->st_info));
	read(fd, &symtab->st_other, sizeof(symtab->st_other));
	read(fd, &symtab->st_shndx, sizeof(symtab->st_shndx));
}

int read_symtab(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, Elf32_Sym **symtab) {
	int idx = 0,
		nbElt;

	idx = get_symtab_index(ehdr->e_shnum,shdr);

	if(idx == -1) {
		return 1;
	}
	else {
		nbElt = shdr[idx]->sh_size / shdr[idx]->sh_entsize; // Nombre de symboles dans la table.

		symtab = malloc(sizeof(Elf32_Sym*) * nbElt);

		for (int i = 0; i < nbElt; ++i)
		{
			lseek(fd, shdr[idx]->sh_addr + i * shdr[idx]->sh_entsize, SEEK_SET);
			read_symbol(fd, symtab[i]);
		}
	}
	return 0;
}
