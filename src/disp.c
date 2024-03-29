#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <elf.h>

#include <elf.h>
#include "elf_common.h"
#include "section.h"
#include "symbol.h"
#include "relocation.h"

#include "type_strings.h"

// HEADER
// static const char *get_type_string(const Types t[], Elf32_Word index)
const char *get_type_string(const Types t[], Elf32_Word index)
{
    int i;
    for(i = 0; (t[i].index != index) && t[i].index != UNKNOWN; i++);
    return t[i].string;
}

char *get_eflags_as_string(Elf32_Half machine, Elf32_Word flags)
{
    static char str[64] = "";

    if(machine == EM_ARM)
    {
        if((EF_ARM_EABIMASK & flags) == EF_ARM_EABI_VER5)
            strcat(str, "Version5 EABI");
        if(flags & EF_ARM_ABI_FLOAT_SOFT)
            strcat(str, ", soft-float ABI");
        else if(flags & EF_ARM_ABI_FLOAT_HARD)
            strcat(str, ", hard-float ABI");
    }

    return str;
}

void dump_header(Elf32_Ehdr *ehdr)
{
    printf("En-tête ELF:\n");
    printf("  %-11s", "Magique:");
    for(int i = 0; i < EI_NIDENT; i++)
        printf("%02x ", ehdr->e_ident[i]);

    printf("\n  %-35s%s\n", "Classe:", get_type_string(elfclass, ehdr->e_ident[EI_CLASS]));
    printf("  %-35s %s\n", "Données:",  get_type_string(elfdata, ehdr->e_ident[EI_DATA]));
    printf("  %-35s%i %s\n", "Version:",  ehdr->e_ident[EI_VERSION], (ehdr->e_ident[EI_VERSION] == EV_CURRENT) ? "(current)" : "");
    printf("  %-35s%s\n", "OS/ABI:",   get_type_string(elfosabi, ehdr->e_ident[EI_OSABI]));
    printf("  %-35s%i\n", "Version ABI:", ehdr->e_ident[EI_ABIVERSION]);
    printf("  %-35s%s\n", "Type:",     get_type_string(et, ehdr->e_type));
    printf("  %-35s%s\n", "Machine:",  get_type_string(em, ehdr->e_machine));
    printf("  %-35s%#x\n", "Version:", ehdr->e_version);
    printf("  %-35s 0x%x\n", "Adresse du point d'entrée:", ehdr->e_entry);
    printf("  %-35s %2i (octets dans le fichier)\n", "Début des en-têtes de programme:", ehdr->e_phoff);
    printf("  %-35s%8i (octets dans le fichier)\n", "Début des en-têtes de section:", ehdr->e_shoff);
    printf("  %-35s%#x, %s\n", "Fanions:", ehdr->e_flags, get_eflags_as_string(ehdr->e_machine, ehdr->e_flags));
    printf("  %-35s %i (octets)\n","Taille de cet en-tête:", ehdr->e_ehsize);
    printf("  %-35s %i (octets)\n","Taille de l'en-tête du programme:", ehdr->e_phentsize);
    printf("  %-35s %i\n","Nombre d'en-tête du programme:", ehdr->e_phnum);
    printf("  %-35s %i (octets)\n","Taille des en-têtes de section:", ehdr->e_shentsize);
    printf("  %-35s %i\n","Nombre d'en-têtes de section:", ehdr->e_shnum);
    printf("  %-35s %i\n","Table d'indexes des chaînes d'en-tête de section:", ehdr->e_shstrndx);
}


// SECTION
void dump_section (int fd, Section_Table *secTab, unsigned index){

    printf("\nAffichage hexadécimal de la section « %s » :\n\n", get_section_name(secTab, index));

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

// static char *flags_to_string(Elf32_Word flags)
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

void dump_section_header(Section_Table *secTab, Elf32_Off offset)
{
    printf("Il y a %i en-têtes de section, débutant à l'adresse de décalage %#x:\n\n", secTab->nb_sections, offset);
    printf("En-têtes de section :\n");
    printf("  [%2s] %-18s %-14s  %-8s %6s %-6s %2s %2s %2s %2s %2s\n",
        "Nr", "Nom", "Type", "Adr", "Décala.", "Taille", "ES", "Fan", "LN", "Inf", "Al");

    for(int i = 0; i < secTab->nb_sections; i++)
        printf("  [%2i] %-18s %-14s  %08x %06x %06x %02x  %2s %2i  %2i %2i\n", i,
            get_section_name(secTab, i),
            get_type_string(sht, secTab->shdr[i]->sh_type),
            secTab->shdr[i]->sh_addr,
            secTab->shdr[i]->sh_offset,
            secTab->shdr[i]->sh_size,
            secTab->shdr[i]->sh_entsize,
            flags_to_string(secTab->shdr[i]->sh_flags),
            secTab->shdr[i]->sh_link,
            secTab->shdr[i]->sh_info,
            secTab->shdr[i]->sh_addralign);
    printf("Liste des fanions :\n");
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


// SYMBOL
void dump_symtab(Symtab_Struct *s) {
    char *STT_VAL[]={"NOTYPE","OBJECT","FUNC","SECTION","FILE","COMMON","TLS"};
    char *STB_VAL[]={"LOCAL","GLOBAL","WEAK"};

    int i = 1;

    printf("\nTable de symboles « %s » contient %i entrées :\n", s->name, s->nbSymbol);
    printf("   Num:    Valeur Tail Type    Lien   Vis      Ndx Nom\n");
    for (i = 0; i < s->nbSymbol; ++i) {
        printf("%6d: ", i);
        printf("%08x ", s->tab[i]->st_value);
        printf("%5d ", s->tab[i]->st_size);
        printf("%-7s ", STT_VAL[ELF32_ST_TYPE(s->tab[i]->st_info)]);
        printf("%-6s ", STB_VAL[ELF32_ST_BIND(s->tab[i]->st_info)]);
        printf("DEFAULT  "); // TODO: Gerer les differentes possibilités (DEFAULT,HIDDEN,PROTECTED)

        switch(s->tab[i]->st_shndx) {
            case SHN_UNDEF:
                printf("UND ");
                break;
            case SHN_ABS:
                printf("ABS ");
                break;

            default:
                printf("%3i ", s->tab[i]->st_shndx);
        }
        printf("%-10s ", get_symbol_name(s->tab,s->symbolNameTable,i));
        printf("\n");
    }
}

void displ_symbolTable(symbolTable *st) {
    if (st->dynsym->nbSymbol > 0) {
        dump_symtab(st->dynsym);
    }
    if (st->symtab->nbSymbol > 0) {
        dump_symtab(st->symtab);
    }
}

// RELOCATION
// static const char *relocation_type_to_string(Elf32_Half machine, Elf32_Word type)
const char *relocation_type_to_string(Elf32_Half machine, Elf32_Word type)
{
    struct { Elf32_Half machine; const Types *type; } m[] =
    {
        { EM_68K,          r_68k        },
        { EM_386,          r_386        },
        { EM_SPARC,        r_sparc      },
        { EM_MIPS,         r_mips       },
        { EM_PARISC,       r_parisc     },
        { EM_ALPHA,        r_alpha      },
        { EM_PPC,          r_ppc        },
        { EM_PPC64,        r_ppc64      },
        { EM_AARCH64,      r_aarch64    },
        { EM_ARM,          r_arm        },
        { EM_IA_64,        r_ia64       },
        { EM_S390,         r_s390       },
        { EM_CRIS,         r_cris       },
        { EM_X86_64,       r_x86_64     },
        { EM_MN10300,      r_mn10300    },
        { EM_M32R,         r_m32r       },
        { EM_MICROBLAZE,   r_microblaze },
        { EM_ALTERA_NIOS2, r_nios2      },
        { EM_TILEPRO,      r_tilepro    },
        { EM_TILEGX,       r_tilegx     },
        { EM_BPF,          r_bpf        },
        { EM_METAG,        r_metag      },
        { EM_NONE,         NULL         }
    };

    int i;
    for(i = 0; (m[i].machine != machine) && (m[i].machine != EM_NONE); i++);
    return (m[i].machine == machine) ? get_type_string(m[i].type, type) : "Inconnu";
}

void dump_relocation_type(Elf32_Ehdr *ehdr, Section_Table *secTab, symbolTable *symTabFull, Data_Rel *drel, int is_rela)
{
    unsigned nb_rel   = (!is_rela) ? drel->nb_rel : drel->nb_rela;
    unsigned *e_rel   = (!is_rela) ? drel->e_rel  : drel->e_rela;
    unsigned *i_rel   = (!is_rela) ? drel->i_rel  : drel->i_rela;
    Elf32_Addr *a_rel = (!is_rela) ? drel->a_rel  : drel->a_rela;
    Elf32_Rela ***rel = (!is_rela) ? (Elf32_Rela ***) drel->rel : drel->rela;

    for(int i = 0; i < nb_rel; i++)
    {
        printf("\nSection de réadressage '%s' à l'adresse de décalage %#x contient %u entrées:\n",
            get_section_name(secTab, i_rel[i]), a_rel[i], e_rel[i]);
        printf(" %-8s   %-8s%-16s%-8s  %s%s\n", "Décalage", "Info", "Type", "Val.-sym", "Noms-symboles", is_rela ? "+ Addenda" : "");
        for(int j = 0; j < e_rel[i]; j++)
        {
            printf("%08x  %08x %-16s  %08x   %s",
                rel[i][j]->r_offset,
                rel[i][j]->r_info,
                relocation_type_to_string(ehdr->e_machine, ELF32_R_TYPE(rel[i][j]->r_info)),
                get_symbol_value_generic(symTabFull, rel[i][j]->r_info),
                get_symbol_or_section_name(secTab, symTabFull, rel[i][j]->r_info));
            if(is_rela)
                printf(" + %i", rel[i][j]->r_addend);
            printf("\n");
        }
    }
}

void dump_relocation(Elf32_Ehdr *ehdr, Section_Table *secTab, symbolTable *symTabFull, Data_Rel *drel)
{
    dump_relocation_type(ehdr, secTab, symTabFull, drel, 0);
    dump_relocation_type(ehdr, secTab, symTabFull, drel, 1);
}
