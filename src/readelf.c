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
#include "elf_common.h"
#include "symbolTable.h"


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
				if(isdigit(optarg[0]))
					args->section_ind = atoi(optarg);
				else
					strcpy(args->section_str, optarg);
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

void free_all(Elf32_Ehdr *ehdr, Section_Table *sectab, symbolTable *symTabFull, Data_Rel *drel)
{
	for(int i = 0; i < ehdr->e_shnum; i++)
		free(sectab->shdr[i]);
	free(ehdr);
	free(sectab->shdr);
	free(sectab->sectionNameTable);
	free(sectab);
	for(int i = 0; i < symTabFull->nbSymbol; i++)
		free(symTabFull->symtab[i]);
	free(symTabFull->symtab);
	for(int i = 0; i < symTabFull->nbDynSymbol; i++)
		free(symTabFull->dynsym[i]);
	free(symTabFull->dynsym);
	free(symTabFull->dynSymbolNameTable);
	free(symTabFull->symbolNameTable);
	free(symTabFull);
	for(int i = 0; i < drel->nb_rel; i++)
	{
		for(int j = 0; j < drel->e_rel[i]; j++)
			free(drel->rel[i][j]);
		free(drel->rel[i]);
	}
	free(drel->rel);
	for(int i = 0; i < drel->nb_rela; i++)
	{
		for(int j = 0; j < drel->e_rela[i]; j++)
			free(drel->rela[i][j]);
		free(drel->rela[i]);
	}
	free(drel->rela);
	free(drel->e_rel);
	free(drel->e_rela);
	free(drel->a_rel);
	free(drel->a_rela);
	free(drel->i_rel);
	free(drel->i_rela);
	free(drel);
}

int main(int argc, char *argv[])
{
	int fd;
	Arguments args = { .display = DSP_NONE, .section_str = "" };
	Elf32_Ehdr *ehdr;
	Section_Table *secTab;
	symbolTable *symTabFull;
	Data_Rel *drel;

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

	/* Lecture en-tête ELF */
	ehdr = read_elf_header(fd);

	/* Lecture table des sections */
	secTab = read_sectionTable(fd, ehdr);

	/* Lecture table des symboles */
	symTabFull = read_symbolTable(fd, ehdr, secTab);

	/* Lecture tables de réimplantation */
	drel = read_relocationTables(fd, ehdr, secTab->shdr);

	switch(args.display)
	{
		case DSP_FILE_HEADER:
			dump_header(ehdr);
			break;
		case DSP_SECTION_HEADERS:
			dump_section_header(ehdr, secTab);
			break;
		case DSP_HEX_DUMP:
			if(is_valid_section(secTab, ehdr->e_shnum, args.section_str, &args.section_ind))
				dump_section(fd, ehdr, secTab, args.section_ind);
			break;
		case DSP_SYMS:
			displ_symbolTable(symTabFull);
			break;
		case DSP_RELOCS:
			dump_relocation(ehdr, secTab, symTabFull, drel);
			break;
		default:
			fprintf(stderr, "Cette option n'est pas encore implémentée.\n");
	}

	close(fd);
	free_all(ehdr, secTab, symTabFull, drel);

	return 0;
}


/**************************** ÉTAPE 1 ****************************
*                     Affichage de l’en-tête
*****************************************************************/

void dump_header(Elf32_Ehdr *ehdr)
{
	printf("En-tête ELF :\n");
	printf("%-11s", "Magique:");
	for(int i = 0; i < EI_NIDENT; i++)
		printf("%02x ", ehdr->e_ident[i]);

	printf("\n %-35s", "Classe:");
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

	printf(" %-35s", "Données:");
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

	printf(" %-35s%i\n", "Version:",ehdr->e_ident[EI_VERSION]);

	printf(" %-35s", "OS/ABI:");
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

	printf(" %-35s", "Type:");
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

	printf(" %-35s", "Machine:");
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

	printf(" %-35s%#x\n", "Version:", ehdr->e_version);
	printf(" %-35s0x%x\n", "Adresse du point d'entrée:", ehdr->e_entry);
	printf(" %-35s %2i (octets dans le fichier)\n", "Début des en-têtes de programme:", ehdr->e_phoff);
	printf(" %-35s %8i (octets dans le fichier)\n", "Début des en-têtes de section:", ehdr->e_shoff);
	printf(" %-35s%#x\n","Fanions:", ehdr->e_flags);
	printf(" %-35s %i (octets)\n","Taille de cet en-tête:", ehdr->e_ehsize);
	printf(" %-35s %i (octets)\n","Taille de l'en-tête du programme:", ehdr->e_phentsize);
	printf(" %-35s %i\n","Nombre d'en-tête du programme:", ehdr->e_phnum);
	printf(" %-35s %i (octets)\n","Taille des en-têtes de section:", ehdr->e_shentsize);
	printf(" %-35s %i\n","Nombre d'en-têtes de section:", ehdr->e_shnum);
	printf(" %-35s %i\n","Table d'indexes des chaînes d'en-tête de section:", ehdr->e_shstrndx);
}


/**************************** ÉTAPE 2 ****************************
*                Affichage de la table des sections
*****************************************************************/

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

void dump_section_header(Elf32_Ehdr *ehdr, Section_Table *secTab)
{
	printf("Il y a %i en-têtes de section, débutant à l'adresse de décalage %#x :\n\n", ehdr->e_shnum, ehdr->e_shoff);
	printf("En-têtes de section :\n");
	printf("[%2s] %-18s %-14s %8s %6s %6s %2s %2s %2s %2s %2s\n",
		"Nr", "Nom", "Type", "Adresse", "Déc.", "Taille", "ES", "Flg", "Lk", "Inf", "Al");

	for(int i = 0; i < ehdr->e_shnum; i++)
		printf("[%2i] %-18s %-14s %08x %06x %06x %02x %2s %2i %2i %2i\n", i,
			get_section_name(secTab->shdr, secTab->sectionNameTable, i),
			section_type_to_string(secTab->shdr[i]->sh_type),
			secTab->shdr[i]->sh_addr,
			secTab->shdr[i]->sh_offset,
			secTab->shdr[i]->sh_size,
			secTab->shdr[i]->sh_entsize,
			flags_to_string(secTab->shdr[i]->sh_flags),
			secTab->shdr[i]->sh_link,
			secTab->shdr[i]->sh_info,
			secTab->shdr[i]->sh_addralign);
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


/**************************** ÉTAPE 3 ****************************
*                Affichage du contenu d’une section
*****************************************************************/

int is_valid_section(Section_Table *secTab, Elf32_Half nb_sections, char *name, unsigned *index)
{
	int i;

	if(*index >= nb_sections)
	{
		fprintf(stderr, "La section %i n'existe pas.\n", *index);
		return 0;
	}

	if(name[0] != '\0')
	{

		for(i = 0; (i < nb_sections) && strcmp(name, get_section_name(secTab->shdr, secTab->sectionNameTable, i)); i++);

		if((i == nb_sections) || strcmp(name, get_section_name(secTab->shdr, secTab->sectionNameTable, i)))
		{
			fprintf(stderr, "La section '%s' n'existe pas.\n",name);
			return 0;
		}
		else
			*index = i;
	}

	return 1;
}

#define BYTES_COUNT     16
#define BLOCKS_COUNT    4
#define BYTES_PER_BLOCK BYTES_COUNT / BLOCKS_COUNT
void dump_section (int fd, Elf32_Ehdr *ehdr, Section_Table *secTab, unsigned index){

	printf("\nAffichage hexadécimal de la section « %s » :\n\n", get_section_name(secTab->shdr, secTab->sectionNameTable, index));

	Elf32_Shdr *shdrToDisplay = secTab->shdr[index];

	unsigned char buffer, line[BYTES_COUNT];

	lseek(fd, shdrToDisplay->sh_offset , SEEK_SET);

	int i;
	int j;
	int k;
	for (i=0; i<shdrToDisplay->sh_size;){
		memset(line, ' ', BYTES_COUNT); // Simplifie l'affichage en ASCII si (i % BYTES_COUNT) != 0
		printf("  0x%08x ", i + shdrToDisplay->sh_addr);

		for (j=0; j<BLOCKS_COUNT; j++){

			for (k=0; k<BYTES_PER_BLOCK; k++){
				if ( i < shdrToDisplay->sh_size ){
					read(fd, &buffer, 1);
					line[i%BYTES_COUNT] = buffer;
					printf("%02x", buffer);
					i++;
				}
				else{
					printf("  ");
				}
			}
			printf(" ");
		}

		for(j=0; j<BYTES_COUNT; j++){
			printf("%c", isprint(line[j]) ? line[j] : '.');
		}
		printf("\n");
	}
}

/**************************** ÉTAPE 5 ****************************
*               Affichage des tables de réimplantation
*****************************************************************/

char *relocation_type_to_string(Elf32_Ehdr *ehdr, Elf32_Word type)
{
	int i;
	struct RelocType { Elf32_Word type_ind; char *type_str; } *machine;

	/* ARM 32 bits */
	struct RelocType arm[] =
	{
		{ R_ARM_NONE,              "R_ARM_NONE"               },
		{ R_ARM_PC24,              "R_ARM_PC24"               },
		{ R_ARM_ABS32,             "R_ARM_ABS32"              },
		{ R_ARM_REL32,             "R_ARM_REL32"              },
		{ R_ARM_PC13,              "R_ARM_PC13"               },
		{ R_ARM_ABS16,             "R_ARM_ABS16"              },
		{ R_ARM_ABS12,             "R_ARM_ABS12"              },
		{ R_ARM_THM_ABS5,          "R_ARM_THM_ABS5"           },
		{ R_ARM_ABS8,              "R_ARM_ABS8"               },
		{ R_ARM_SBREL32,           "R_ARM_SBREL32"            },
		{ R_ARM_THM_PC22,          "R_ARM_THM_PC22"           },
		{ R_ARM_THM_PC8,           "R_ARM_THM_PC8"            },
		{ R_ARM_AMP_VCALL9,        "R_ARM_AMP_VCALL9"         },
		{ R_ARM_TLS_DESC,          "R_ARM_TLS_DESC"           },
		{ R_ARM_THM_SWI8,          "R_ARM_THM_SWI8"           },
		{ R_ARM_XPC25,             "R_ARM_XPC25"              },
		{ R_ARM_THM_XPC22,         "R_ARM_THM_XPC22"          },
		{ R_ARM_TLS_DTPMOD32,      "R_ARM_TLS_DTPMOD32"       },
		{ R_ARM_TLS_DTPOFF32,      "R_ARM_TLS_DTPOFF32"       },
		{ R_ARM_TLS_TPOFF32,       "R_ARM_TLS_TPOFF32"        },
		{ R_ARM_COPY,              "R_ARM_COPY"               },
		{ R_ARM_GLOB_DAT,          "R_ARM_GLOB_DAT"           },
		{ R_ARM_JUMP_SLOT,         "R_ARM_JUMP_SLOT"          },
		{ R_ARM_RELATIVE,          "R_ARM_RELATIVE"           },
		{ R_ARM_GOTOFF,            "R_ARM_GOTOFF"             },
		{ R_ARM_GOTPC,             "R_ARM_GOTPC"              },
		{ R_ARM_GOT32,             "R_ARM_GOT32"              },
		{ R_ARM_PLT32,             "R_ARM_PLT32"              },
		{ R_ARM_CALL,              "R_ARM_CALL"               },
		{ R_ARM_JUMP24,            "R_ARM_JUMP24"             },
		{ R_ARM_THM_JUMP24,        "R_ARM_THM_JUMP24"         },
		{ R_ARM_BASE_ABS,          "R_ARM_BASE_ABS"           },
		{ R_ARM_ALU_PCREL_7_0,     "R_ARM_ALU_PCREL_7_0"      },
		{ R_ARM_ALU_PCREL_15_8,    "R_ARM_ALU_PCREL_15_8"     },
		{ R_ARM_ALU_PCREL_23_15,   "R_ARM_ALU_PCREL_23_15"    },
		{ R_ARM_LDR_SBREL_11_0,    "R_ARM_LDSBREL_11_0"       },
		{ R_ARM_ALU_SBREL_19_12,   "R_ARM_ALU_SBREL_19_12"    },
		{ R_ARM_ALU_SBREL_27_20,   "R_ARM_ALU_SBREL_27_20"    },
		{ R_ARM_TARGET1,           "R_ARM_TARGET1"            },
		{ R_ARM_SBREL31,           "R_ARM_SBREL31"            },
		{ R_ARM_V4BX,              "R_ARM_V4BX"               },
		{ R_ARM_TARGET2,           "R_ARM_TARGET2"            },
		{ R_ARM_PREL31,            "R_ARM_PREL31"             },
		{ R_ARM_MOVW_ABS_NC,       "R_ARM_MOVW_ABS_NC"        },
		{ R_ARM_MOVT_ABS,          "R_ARM_MOVT_ABS"           },
		{ R_ARM_MOVW_PREL_NC,      "R_ARM_MOVW_PREL_NC"       },
		{ R_ARM_MOVT_PREL,         "R_ARM_MOVT_PREL"          },
		{ R_ARM_THM_MOVW_ABS_NC,   "R_ARM_THM_MOVW_ABS_NC"    },
		{ R_ARM_THM_MOVT_ABS,      "R_ARM_THM_MOVT_ABS"       },
		{ R_ARM_THM_MOVW_PREL_NC,  "R_ARM_THM_MOVW_PREL_NC"   },
		{ R_ARM_THM_MOVT_PREL,     "R_ARM_THM_MOVT_PREL"      },
		{ R_ARM_THM_JUMP19,        "R_ARM_THM_JUMP19"         },
		{ R_ARM_THM_JUMP6,         "R_ARM_THM_JUMP6"          },
		{ R_ARM_THM_ALU_PREL_11_0, "R_ARM_THM_ALU_PREL_11_0"  },
		{ R_ARM_THM_PC12,          "R_ARM_THM_PC12"           },
		{ R_ARM_ABS32_NOI,         "R_ARM_ABS32_NOI"          },
		{ R_ARM_REL32_NOI,         "R_ARM_REL32_NOI"          },
		{ R_ARM_ALU_PC_G0_NC,      "R_ARM_ALU_PC_G0_NC"       },
		{ R_ARM_ALU_PC_G0,         "R_ARM_ALU_PC_G0"          },
		{ R_ARM_ALU_PC_G1_NC,      "R_ARM_ALU_PC_G1_NC"       },
		{ R_ARM_ALU_PC_G1,         "R_ARM_ALU_PC_G1"          },
		{ R_ARM_ALU_PC_G2,         "R_ARM_ALU_PC_G2"          },
		{ R_ARM_LDR_PC_G1,         "R_ARM_LDPC_G1"            },
		{ R_ARM_LDR_PC_G2,         "R_ARM_LDPC_G2"            },
		{ R_ARM_LDRS_PC_G0,        "R_ARM_LDRS_PC_G0"         },
		{ R_ARM_LDRS_PC_G1,        "R_ARM_LDRS_PC_G1"         },
		{ R_ARM_LDRS_PC_G2,        "R_ARM_LDRS_PC_G2"         },
		{ R_ARM_LDC_PC_G0,         "R_ARM_LDC_PC_G0"          },
		{ R_ARM_LDC_PC_G1,         "R_ARM_LDC_PC_G1"          },
		{ R_ARM_LDC_PC_G2,         "R_ARM_LDC_PC_G2"          },
		{ R_ARM_ALU_SB_G0_NC,      "R_ARM_ALU_SB_G0_NC"       },
		{ R_ARM_ALU_SB_G0,         "R_ARM_ALU_SB_G0"          },
		{ R_ARM_ALU_SB_G1_NC,      "R_ARM_ALU_SB_G1_NC"       },
		{ R_ARM_ALU_SB_G1,         "R_ARM_ALU_SB_G1"          },
		{ R_ARM_ALU_SB_G2,         "R_ARM_ALU_SB_G2"          },
		{ R_ARM_LDR_SB_G0,         "R_ARM_LDSB_G0"            },
		{ R_ARM_LDR_SB_G1,         "R_ARM_LDSB_G1"            },
		{ R_ARM_LDR_SB_G2,         "R_ARM_LDSB_G2"            },
		{ R_ARM_LDRS_SB_G0,        "R_ARM_LDRS_SB_G0"         },
		{ R_ARM_LDRS_SB_G1,        "R_ARM_LDRS_SB_G1"         },
		{ R_ARM_LDRS_SB_G2,        "R_ARM_LDRS_SB_G2"         },
		{ R_ARM_LDC_SB_G0,         "R_ARM_LDC_SB_G0"          },
		{ R_ARM_LDC_SB_G1,         "R_ARM_LDC_SB_G1"          },
		{ R_ARM_LDC_SB_G2,         "R_ARM_LDC_SB_G2"          },
		{ R_ARM_MOVW_BREL_NC,      "R_ARM_MOVW_BREL_NC"       },
		{ R_ARM_MOVT_BREL,         "R_ARM_MOVT_BREL"          },
		{ R_ARM_MOVW_BREL,         "R_ARM_MOVW_BREL"          },
		{ R_ARM_THM_MOVW_BREL_NC,  "R_ARM_THM_MOVW_BREL_NC"   },
		{ R_ARM_THM_MOVT_BREL,     "R_ARM_THM_MOVT_BREL"      },
		{ R_ARM_THM_MOVW_BREL,     "R_ARM_THM_MOVW_BREL"      },
		{ R_ARM_TLS_GOTDESC,       "R_ARM_TLS_GOTDESC"        },
		{ R_ARM_TLS_CALL,          "R_ARM_TLS_CALL"           },
		{ R_ARM_TLS_DESCSEQ,       "R_ARM_TLS_DESCSEQ"        },
		{ R_ARM_THM_TLS_CALL,      "R_ARM_THM_TLS_CALL"       },
		{ R_ARM_PLT32_ABS,         "R_ARM_PLT32_ABS"          },
		{ R_ARM_GOT_ABS,           "R_ARM_GOT_ABS"            },
		{ R_ARM_GOT_PREL,          "R_ARM_GOT_PREL"           },
		{ R_ARM_GOT_BREL12,        "R_ARM_GOT_BREL12"         },
		{ R_ARM_GOTOFF12,          "R_ARM_GOTOFF12"           },
		{ R_ARM_GOTRELAX,          "R_ARM_GOTRELAX"           },
		{ R_ARM_GNU_VTENTRY,       "R_ARM_GNU_VTENTRY"        },
		{ R_ARM_GNU_VTINHERIT,     "R_ARM_GNU_VTINHERIT"      },
		{ R_ARM_THM_PC11,          "R_ARM_THM_PC11"           },
		{ R_ARM_THM_PC9,           "R_ARM_THM_PC9"            },
		{ R_ARM_TLS_GD32,          "R_ARM_TLS_GD32"           },
		{ R_ARM_TLS_LDM32,         "R_ARM_TLS_LDM32"          },
		{ R_ARM_TLS_LDO32,         "R_ARM_TLS_LDO32"          },
		{ R_ARM_TLS_IE32,          "R_ARM_TLS_IE32"           },
		{ R_ARM_TLS_LE32,          "R_ARM_TLS_LE32"           },
		{ R_ARM_TLS_LDO12,         "R_ARM_TLS_LDO12"          },
		{ R_ARM_TLS_LE12,          "R_ARM_TLS_LE12"           },
		{ R_ARM_TLS_IE12GP,        "R_ARM_TLS_IE12GP"         },
		{ R_ARM_ME_TOO,            "R_ARM_ME_TOO"             },
		{ R_ARM_THM_TLS_DESCSEQ,   "R_ARM_THM_TLS_DESCSEQ"    },
		{ R_ARM_THM_TLS_DESCSEQ16, "R_ARM_THM_TLS_DESCSEQ16"  },
		{ R_ARM_THM_TLS_DESCSEQ32, "R_ARM_THM_TLS_DESCSEQ32"  },
		{ R_ARM_THM_GOT_BREL12,    "R_ARM_THM_GOT_BREL12"     },
		{ R_ARM_IRELATIVE,         "R_ARM_IRELATIVE"          },
		{ R_ARM_RXPC25,            "R_ARM_RXPC25"             },
		{ R_ARM_RSBREL32,          "R_ARM_RSBREL32"           },
		{ R_ARM_THM_RPC22,         "R_ARM_THM_RPC22"          },
		{ R_ARM_RREL32,            "R_ARM_RREL32"             },
		{ R_ARM_RABS22,            "R_ARM_RABS22"             },
		{ R_ARM_RPC24,             "R_ARM_RPC24"              },
		{ R_ARM_RBASE,             "R_ARM_RBASE"              },
		{ -1,                      ""                         }
	};

	/* x86 32 bits */
	struct RelocType x86[] =
	{
		{ R_386_NONE,              "R_386_NONE"               },
		{ R_386_32,                "R_386_32"                 },
		{ R_386_PC32,              "R_386_PC32"               },
		{ R_386_GOT32,             "R_386_GOT32"              },
		{ R_386_PLT32,             "R_386_PLT32"              },
		{ R_386_COPY,              "R_386_COPY"               },
		{ R_386_GLOB_DAT,          "R_386_GLOB_DAT"           },
		{ R_386_JMP_SLOT,          "R_386_JMP_SLOT"           },
		{ R_386_RELATIVE,          "R_386_RELATIVE"           },
		{ R_386_GOTOFF,            "R_386_GOTOFF"             },
		{ R_386_GOTPC,             "R_386_GOTPC"              },
		{ R_386_32PLT,             "R_386_32PLT"              },
		{ R_386_TLS_TPOFF,         "R_386_TLS_TPOFF"          },
		{ R_386_TLS_IE,            "R_386_TLS_IE"             },
		{ R_386_TLS_GOTIE,         "R_386_TLS_GOTIE"          },
		{ R_386_TLS_LE,            "R_386_TLS_LE"             },
		{ R_386_TLS_GD,            "R_386_TLS_GD"             },
		{ R_386_TLS_LDM,           "R_386_TLS_LDM"            },
		{ R_386_16,                "R_386_16"                 },
		{ R_386_PC16,              "R_386_PC16"               },
		{ R_386_8,                 "R_386_8"                  },
		{ R_386_PC8,               "R_386_PC8"                },
		{ R_386_TLS_GD_32,         "R_386_TLS_GD_32"          },
		{ R_386_TLS_GD_PUSH,       "R_386_TLS_GD_PUSH"        },
		{ R_386_TLS_GD_CALL,       "R_386_TLS_GD_CALL"        },
		{ R_386_TLS_GD_POP,        "R_386_TLS_GD_POP"         },
		{ R_386_TLS_LDM_32,        "R_386_TLS_LDM_32"         },
		{ R_386_TLS_LDM_PUSH,      "R_386_TLS_LDM_PUSH"       },
		{ R_386_TLS_LDM_CALL,      "R_386_TLS_LDM_CALL"       },
		{ R_386_TLS_LDM_POP,       "R_386_TLS_LDM_POP"        },
		{ R_386_TLS_LDO_32,        "R_386_TLS_LDO_32"         },
		{ R_386_TLS_IE_32,         "R_386_TLS_IE_32"          },
		{ R_386_TLS_LE_32,         "R_386_TLS_LE_32"          },
		{ R_386_TLS_DTPMOD32,      "R_386_TLS_DTPMOD32"       },
		{ R_386_TLS_DTPOFF32,      "R_386_TLS_DTPOFF32"       },
		{ R_386_TLS_TPOFF32,       "R_386_TLS_TPOFF32"        },
		{ R_386_SIZE32,            "R_386_SIZE32"             },
		{ R_386_TLS_GOTDESC,       "R_386_TLS_GOTDESC"        },
		{ R_386_TLS_DESC_CALL,     "R_386_TLS_DESC_CALL"      },
		{ R_386_TLS_DESC,          "R_386_TLS_DESC"           },
		{ R_386_IRELATIVE,         "R_386_IRELATIVE"          },
		{ -1,                      ""                         }
	};

	switch(ehdr->e_machine)
	{
		case EM_ARM:
			machine = arm;
			break;
		case EM_386:
			machine = x86;
			break;
		default:
			return "";
	}

	for(i = 0; (type != machine[i].type_ind && (machine[i].type_ind != -1)); i++);
	return machine[i].type_str;
}

int isDynamicRel(int rel_type) {
	return ((rel_type == R_ARM_TLS_DESC)
	|| (rel_type == R_ARM_TLS_DTPMOD32)
	|| (rel_type == R_ARM_TLS_DTPOFF32)
	|| (rel_type == R_ARM_TLS_TPOFF32)
	|| (rel_type == R_ARM_COPY)
	|| (rel_type == R_ARM_GLOB_DAT)
	|| (rel_type == R_ARM_JUMP_SLOT)
	|| (rel_type == R_ARM_RELATIVE));
}

static inline Elf32_Addr get_symbol_value_generic(symbolTable *symTabFull, Elf32_Word info)
{
	return isDynamicRel(ELF32_R_TYPE(info)) ?
		symTabFull->dynsym[ELF32_R_SYM(info)]->st_value :
		symTabFull->symtab[ELF32_R_SYM(info)]->st_value;
}

static inline char *get_symbol_name_generic(symbolTable *symTabFull, Elf32_Word info)
{
	return isDynamicRel(ELF32_R_TYPE(info)) ?
		get_symbol_name(symTabFull->dynsym, symTabFull->dynSymbolNameTable, ELF32_R_SYM(info)) :
		get_symbol_name(symTabFull->symtab, symTabFull->symbolNameTable,    ELF32_R_SYM(info));
}

void dump_relocation(Elf32_Ehdr *ehdr, Section_Table *secTab, symbolTable *symTabFull, Data_Rel *drel)
{
	/* Table de réimplantation sans addenda */
	for(int i = 0; i < drel->nb_rel; i++)
	{
		printf("\nSection de relocalisation '%s' à l'adresse de décalage %#x contient %u entrées :\n",
			get_section_name(secTab->shdr, secTab->sectionNameTable, drel->i_rel[i]),
			drel->a_rel[i],
			drel->e_rel[i]);
		printf("%-8s  %-8s  %-23s  %-8s  %s\n", "Décalage", "Info", "Type", "Val.-sym", "Noms-symboles");
		for(int j = 0; j < drel->e_rel[i]; j++)
		{
			printf("%08x  %08x  %-23s  %08x  %s\n",
				drel->rel[i][j]->r_offset,
				drel->rel[i][j]->r_info,
				relocation_type_to_string(ehdr, ELF32_R_TYPE(drel->rel[i][j]->r_info)),
				get_symbol_value_generic(symTabFull, drel->rel[i][j]->r_info),
				get_symbol_name_generic(symTabFull,  drel->rel[i][j]->r_info));
		}
	}

	/* Table de réimplantation avec addenda */
	for(int i = 0; i < drel->nb_rela; i++)
	{
		printf("\nSection de relocalisation '%s' à l'adresse de décalage %#x contient %u entrées :\n",
			get_section_name(secTab->shdr, secTab->sectionNameTable, drel->i_rela[i]),
			drel->a_rela[i],
			drel->e_rela[i]);
		printf("%-8s  %-8s  %-23s  %-8s  %s\n", "Décalage", "Info", "Type", "Val.-sym", "Noms-symboles+ Addenda");
		for(int j = 0; j < drel->e_rela[i]; j++)
		{
			printf("%08x  %08x  %-23s  %08x  %s + %i\n",
				drel->rela[i][j]->r_offset,
				drel->rela[i][j]->r_info,
				relocation_type_to_string(ehdr, ELF32_R_TYPE(drel->rela[i][j]->r_info)),
				get_symbol_value_generic(symTabFull, drel->rela[i][j]->r_info),
				get_symbol_name_generic(symTabFull,  drel->rela[i][j]->r_info),
				drel->rela[i][j]->r_addend);
		}
	}
}
