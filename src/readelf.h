#ifndef _READELF_H_
#define _READELF_H_


#ifdef __LP64__
int read_header(int fd, Elf64_Ehdr *hdr);
int read_section_header(int fd, Elf64_Ehdr *hdr, Elf64_Shdr **shdr);
void dumpelf(Elf64_Ehdr *hdr);
char *get_section_name_table(int fd, Elf64_Ehdr *hdr, Elf64_Shdr **shdr);
char *get_section_name(Elf64_Shdr **shdr, char *table, unsigned index);
#else
int read_header(int fd, Elf32_Ehdr *hdr);
int read_section_header(int fd, Elf32_Ehdr *hdr, Elf32_Shdr **shdr);
void dumpelf(Elf32_Ehdr *hdr);
char *get_section_name_table(int fd, Elf32_Ehdr *hdr, Elf32_Shdr **shdr);
char *get_section_name(Elf32_Shdr **shdr, char *table, unsigned index);
#endif

#endif

