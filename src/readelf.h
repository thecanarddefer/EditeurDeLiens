#ifndef _READELF_H_
#define _READELF_H_

#define DSP_FILE_HEADER     (1 << 0)
#define DSP_SECTION_HEADERS (1 << 1)


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


#endif
