#ifndef _READELF_H_
#define _READELF_H_


#ifdef __LP64__
int readelf(Elf64_Ehdr *hdr, char *filename);
#else
int readelf(Elf32_Ehdr *hdr, char *filename);
#endif

#ifdef __LP64__
void dumpelf(Elf64_Ehdr *hdr);
#else
void dumpelf(Elf32_Ehdr *hdr);
#endif

#endif

