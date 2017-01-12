#ifndef _FUSION_H_
#define _FUSION_H_

#include <elf.h>
#include "section.h"
#include "symbol.h"
#include "relocation.h"

typedef struct
{
	char section[32];
	Elf32_Shdr *ptr_shdr1;
	Elf32_Shdr *ptr_shdr2;
	Elf32_Word size;
	Elf32_Off offset;
} Fusion;

typedef struct
{
	unsigned nb_sections;
	Elf32_Off offset;
	unsigned nb_written;
	off_t file_offset;
	Elf32_Word symbolNameTable_size;
	Elf32_Section *newsec1, *newsec2;
	Fusion **f;
} Data_fusion;

typedef enum { ONLY1, MERGE, MERGE_NOT_IN } Gather_Mode;

/**
 * Ouvre les deux premiers fichiers passés en argument en lecture
 * et le troisième en écriture
 *
 * @param argv:   la ligne des arguments
 * @param fd_in1: premier fichier en entrée
 * @param fd_in2: second fichier en entrée
 * @param fd_out: fichier de sortie
 * @retourne 0 en cas de succès
 **/
static int open_files(char *argv[], int *fd_in1, int *fd_in2, int *fd_out);

/**
 * Rassemble les sections des types passés en paramètre
 *
 * @param df:       une structure de type Data_fusion
 * @param secTab1:  une structure de type Section_Table initialisée concernant le premier fichier
 * @param secTab2:  une structure de type Section_Table initialisée concernant le second fichier
 * @param nb_types: le nombre de types à passer en argument
 * @param mode:     le mode, de type Gather_Mode
 * @param ...:      les types de section à placer dans df
 **/
static void gather_sections(Data_fusion *df, Section_Table *secTab1, Section_Table *secTab2, Gather_Mode mode, int nb_types, ...);

/**
 * Donne la correspondance des numéros de sections d'un fichier d'entrée avec
 * les numéros de section du fichier de sortie
 *
 * @param df:     une structure de type Data_fusion
 * @param secTab: une structure de type Section_Table initialisée
 * @retourne un tableau de type Elf32_Section contenant les nouveaux indices de sections
 **/
static Elf32_Section *find_new_section_index(Data_fusion *df, Section_Table *secTab);

/**
 * Met à jour l'index de section d'un symbole
 *
 * @param symbol:      un symbole de type Elf32_Sym à corriger
 * @param newsec:      un tableau de type Elf32_Section contenant les nouveaux index de sections
 * @param nb_sections: le nombre de sections dans le fichier de sortie
 **/
static void update_section_index_in_symbol(Elf32_Sym *symbol, Elf32_Section *newsec, unsigned nb_sections);

/**
 * Calcule l'indice d'une sous-chaîne dans une chaîne
 *
 * @param haystack: la chaîne dans laquelle rechercher la sous-chaîne
 * @param needle:   la sous-chaîne
 * @param size:     la taille de haystack
 * @retourne l'indice une valeur >= 0 en cas de succès, -1 sinon
 **/
static int find_index_in_name_table(char *haystack, char *needle, Elf32_Word size);

/**
 * Ajoute un symbole à la table des symboles
 *
 * @param st:     une structure de type symbolTable
 * @param symtab: un symbole de type Elf32_Sym initialisé
 * @retourne l'indice où le nouveau symbole a été ajouté
 **/
static int add_symbol_in_table(Symtab_Struct *st, Elf32_Sym *symtab);

/**
 * Fusionne deux tables des symboles tout en les corrigeant
 *
 * @param df:      une structure de type Data_fusion initialisée
 * @param secTab1: une structure de type Section_Table initialisée correspondant au premier fichier
 * @param secTab2: une structure de type Section_Table initialisée correspondant au second fichier
 * @param st1:     une structure de type symbolTable initialisée correspondant au premier fichier
 * @param st2:     une structure de type symbolTable initialisée correspondant au second fichier
 * @param st_out:  une structure de type symbolTable initialisée correspondant au fichier à créer
 * @retourne 0 en cas de succès
 **/
static int merge_and_fix_symbols(Data_fusion *df, Section_Table *secTab1, Section_Table *secTab2, symbolTable *st1, symbolTable *st2, Symtab_Struct *st_out);

/**
 * Met à jour le champ r_info des tables de réimplantations
 *
 * @param drel:   une structure de type Data_Rel initialisée
 * @param newsec: un tableau de type Elf32_Section contenant les nouveaux index de sections
 **/
static void update_relocations_info(Data_Rel *drel, Elf32_Section *newsec);

/**
 * Fusionne deux tables de réimplantations tout en corrigeant les symboles
 *
 * @param df:      une structure de type Data_fusion initialisée
 * @param fd2:     le descripteur de fichier du second fichier
 * @param secTab1: une structure de type Section_Table initialisée correspondant au premier fichier
 * @param secTab2: une structure de type Section_Table initialisée correspondant au second fichier
 * @param drel1:   une structure de type Data_Rel initialisée  correspondant au premier fichier
 * @param drel1:   une structure de type Data_Rel initialisée  correspondant au second fichier
 **/
static void merge_and_fix_relocations(Data_fusion *df, int fd2, Section_Table *secTab1, Section_Table *secTab2, Data_Rel *drel1, Data_Rel *drel2);

/**
 * Trie une table des symboles
 *
 * @param st: une structure de type symbolTable
 **/
static void sort_new_symbol_table(Symtab_Struct *st);

/**
 * Écrit le nouvel en-tête ELF dans le fichier de sortie
 *
 * @param fd_out: fichier de sortie
 * @param ehdr:   une structure de type Elf32_Ehdr initialisée
 * @param df:     une structure de type Data_fusion initialisée
 **/
static void write_elf_header_in_file(int fd_out, Elf32_Ehdr *ehdr, Data_fusion *df);

/**
 * Écrit des sections du premier fichier dans le fichier de sortie
 *
 * @param df:     une structure de type Data_fusion initialisée
 * @param fd_in1: premier fichier en entrée
 * @param fd_out: fichier de sortie
 **/
static void write_only_sections_in_file(Data_fusion *df, int fd1, int fd_out);

/**
 * Concatène des sections dans le fichier de sortie
 *
 * @param df:     une structure de type Data_fusion initialisée
 * @param fd_in1: premier fichier en entrée
 * @param fd_in2: second fichier en entrée
 * @param fd_out: fichier de sortie
 **/
static void merge_and_write_sections_in_file(Data_fusion *df, int fd1, int fd2, int fd_out);

/**
 * Recopie une section depuis un fichier vers un autre fichier
 *
 * PRÉ-CONDITION: le curseur de fd_out est placé au bon endroit
 * @param fd_in:  un descrpteur de fichier vers le fichier d'entrée
 * @param fd_out: un descrpteur de fichier vers le fichier de sortie
 * @param shdr:   une structure de type Elf32_Shdr initialisée
 * @retourne le nombre d'octets écrits dans le fichier
 **/
static ssize_t write_section_in_file(int fd_in, int fd_out, Elf32_Shdr *shdr);

/**
 * Écrit la nouvelle table des noms de section dans le fichier de sortie
 *
 * @param fd_out: fichier de sortie
 * @param ehdr:   une structure de type Elf32_Ehdr initialisée
 * @param df:     une structure de type Data_fusion initialisée
 **/
static void write_new_section_name_table_in_file(int fd_out, Elf32_Ehdr *ehdr, Data_fusion *df);

/**
 * Écrit la table des symboles dans le fichier de sortie
 *
 * @param fd_out: fichier de sortie
 * @param df:     une structure de type Data_fusion initialisée
 * @param st_out: une structure de type Symtab_Struct initialisée
 **/
static void write_new_symbol_table_in_file(int fd_out, Data_fusion *df, Symtab_Struct *st_out);

/**
 * Libère la mémoire allouée pour une structure de type Data_fusion
 *
 * @param df: une structure de type Data_fusion initialisée
 **/
static void destroy_data_fusion(Data_fusion *df);


#endif
