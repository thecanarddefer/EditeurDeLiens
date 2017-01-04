#ifndef _READELF_H_
#define _READELF_H_


enum DSP { DSP_NONE, DSP_FILE_HEADER, DSP_SECTION_HEADERS, DSP_HEX_DUMP, DSP_SYMS, DSP_RELOCS };

/**
 * Lis l'en-tête d'un fichier ELF 32 bits et stocke les informations dans une structure
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr
 * @retourne 0 en cas de succès
 **/
int read_header(int fd, Elf32_Ehdr *ehdr);

/**
 * Lis les sections d'un fichiers ELF 32 bits et stocke les informations dans un tableau de structures
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr
 * @retourne 0 en cas de succès
 **/
int read_section_header(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);

/**
 * Lis la table des noms de section d'un fichier ELF 32 bits et retourne la table
 *
 * @param fd:   un descripteur de fichier (ELF32)
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 * @param shdr: un tableau de structures de type Elf32_Shdr initialisé
 * @retourne la table des noms de section
 **/
char *get_section_name_table(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);

/**
 * Retourne le nom d'une section donnée
 *
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param table: une chaîne de caractères initialisée contenant la table des noms de section
 * @param index: le numéro d'une section
 * @retourne une chaîne de caractères correspondant au nom de la section
 **/
char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index);

/**
 * Affiche les informations sur l'en-tête lu
 *
 * @param ehdr: une structure de type Elf32_Ehdr initialisée
 **/
void dump_header(Elf32_Ehdr *ehdr);

/**
 * Affiche les informations sur l'en-tête de section lu
 *
 * @param ehdr:  une structure de type Elf32_Ehdr initialisée
 * @param shdr:  un tableau de structures de type Elf32_Shdr initialisé
 * @param table: la table des noms de section
 **/
void dump_section_header(Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, char *table);

Elf32_Sym **read_symtab(int fd, Elf32_Ehdr *ehdr, Elf32_Shdr **shdr, int *nbElt, int *idxStrTab);
char *get_symbol_name_table(int fd, int idxStrTab, Elf32_Shdr **shdr);
void dump_symtab(int nbElt, Elf32_Sym **symtab, char *table);

#endif
