#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/elf.h>
#include "readelf.h"


int main(int argc, char *argv[])
{
	int fd;
#ifdef __LP64__
	Elf64_Ehdr *hdr = malloc(sizeof(Elf64_Ehdr));
	Elf64_Shdr **shdr;
#else
	Elf32_Ehdr *hdr = malloc(sizeof(Elf32_Ehdr));
	Elf32_Shdr **shdr;
#endif

	if(argc <= 1)
	{
		fprintf(stderr, "%s : nom de fichier requis.\n", argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "Impossible d'ouvrir le fichier %s.\n", argv[1]);
		return 2;
	}

	read_header(fd, hdr);
	shdr = malloc(sizeof(Elf32_Shdr*) * hdr->e_shnum);
	for(int i = 0; i < hdr->e_shnum; i++)
#ifdef __LP64__
		shdr[i] = malloc(sizeof(Elf64_Shdr));
#else
		shdr[i] = malloc(sizeof(Elf32_Shdr));
#endif
	read_section_header(fd, hdr, shdr);
	dumpelf(hdr);

	close(fd);
	free(hdr);
	return 0;
}

#ifdef __LP64__
int read_header(int fd, Elf64_Ehdr *hdr)
#else
int read_header(int fd, Elf32_Ehdr *hdr)
#endif
{
	read(fd, hdr->e_ident, EI_NIDENT);
	if(hdr->e_ident[0] != ELFMAG0 || hdr->e_ident[1] != ELFMAG1 || hdr->e_ident[2] != ELFMAG2 || hdr->e_ident[3] != ELFMAG3)
	{
		fprintf(stderr, "Le fichier n'est pas de type ELF.\n");
		exit(2);
	}

	read(fd, &hdr->e_type, sizeof(hdr->e_type));
	read(fd, &hdr->e_machine, sizeof(hdr->e_machine));
	read(fd, &hdr->e_version, sizeof(hdr->e_version));
	read(fd, &hdr->e_entry, sizeof(hdr->e_entry));
	read(fd, &hdr->e_phoff, sizeof(hdr->e_phoff));
	read(fd, &hdr->e_shoff, sizeof(hdr->e_shoff));
	read(fd, &hdr->e_flags, sizeof(hdr->e_flags));
	read(fd, &hdr->e_ehsize, sizeof(hdr->e_ehsize));
	read(fd, &hdr->e_phentsize, sizeof(hdr->e_phentsize));
	read(fd, &hdr->e_phnum, sizeof(hdr->e_phnum));
	read(fd, &hdr->e_shentsize, sizeof(hdr->e_shentsize));
	read(fd, &hdr->e_shnum, sizeof(hdr->e_shnum));
	read(fd, &hdr->e_shstrndx, sizeof(hdr->e_shstrndx));

	return 0;
}

#ifdef __LP64__
int read_section_header(int fd, Elf64_Ehdr *hdr, Elf64_Shdr **shdr)
#else
int read_section_header(int fd, Elf32_Ehdr *hdr, Elf32_Shdr **shdr)
#endif
{
	for(int i = 0; i < hdr->e_shnum; i++)
	{
		lseek(fd, hdr->e_shoff + i*hdr->e_shentsize, SEEK_SET);
		read(fd, &shdr[i]->sh_name, sizeof(shdr[i]->sh_name));
		read(fd, &shdr[i]->sh_type, sizeof(shdr[i]->sh_type));
		read(fd, &shdr[i]->sh_flags, sizeof(shdr[i]->sh_flags));
		read(fd, &shdr[i]->sh_addr, sizeof(shdr[i]->sh_addr));
		read(fd, &shdr[i]->sh_offset, sizeof(shdr[i]->sh_offset));
		read(fd, &shdr[i]->sh_size, sizeof(shdr[i]->sh_size));
		read(fd, &shdr[i]->sh_link, sizeof(shdr[i]->sh_link));
		read(fd, &shdr[i]->sh_info, sizeof(shdr[i]->sh_info));
		read(fd, &shdr[i]->sh_addralign, sizeof(shdr[i]->sh_addralign));
		read(fd, &shdr[i]->sh_entsize, sizeof(shdr[i]->sh_entsize));
	}

	return 0;
}

#ifdef __LP64__
char *get_section_name_table(int fd, Elf64_Ehdr *hdr, Elf64_Shdr **shdr)
#else
char *get_section_name_table(int fd, Elf32_Ehdr *hdr, Elf32_Shdr **shdr)
#endif
{
	int pos;
	const unsigned offset = shdr[hdr->e_shstrndx]->sh_offset;
	char *table = (char *) malloc(sizeof(char) * offset);

	pos = lseek(fd, 0, SEEK_CUR);
	lseek(fd, offset, SEEK_SET);
	read(fd, table, offset);

	lseek(fd, pos, SEEK_SET);
	return table;
}

#ifdef __LP64__
char *get_section_name(Elf64_Shdr **shdr, char *table, unsigned index)
#else
char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index)
#endif
{
	return &(table[shdr[index]->sh_name]);
}

#ifdef __LP64__
void dumpelf(Elf64_Ehdr *hdr)
#else
void dumpelf(Elf32_Ehdr *hdr)
#endif
{
	printf("Classe : ");
	switch(hdr->e_ident[EI_CLASS])
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
	switch(hdr->e_ident[EI_DATA])
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

	printf("Version : %i\n", hdr->e_ident[EI_VERSION]);

	printf("OS/ABI : ");
	switch(hdr->e_ident[EI_OSABI])
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
	switch(hdr->e_type)
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
	switch(hdr->e_machine)
	{
		case EM_ARM:
			printf("ARM 32 bits\n");
			break;
		case EM_AARCH64:
			printf("ARM 64 bits\n");
			break;
		case EM_386:
			printf("Intel 32 bits\n");
			break;
		case EM_X86_64:
			printf("AMD 64 bits\n");
			break;
		default:
			printf("Inconnue\n");
	}

	printf("Version : 0x%x\n", hdr->e_version);
	printf("Adresse du point d'entrée : 0x%x\n", hdr->e_entry);
	printf("Début des en-têtes de programme : %i\n", hdr->e_phoff);
	printf("Début des en-têtes de section : %i\n", hdr->e_shoff);
	printf("Fanions : 0x%x\n", hdr->e_flags);
	printf("Taille de cet en-tête : %i\n", hdr->e_ehsize);
	printf("Taille de l'en-tête du programme : %i\n", hdr->e_phentsize);
	printf("Nombre d'en-tête du programme : %i\n", hdr->e_phnum);
	printf("Taille des en-têtes de section : %i\n", hdr->e_shentsize);
	printf("Nombre d'en-têtes de section : %i\n", hdr->e_shnum);
	printf("Table d'indexes des chaînes d'en-tête de section : %i\n", hdr->e_shstrndx);


}

