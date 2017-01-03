#ifndef _READELF_H_
#define _READELF_H_


int readelf(Elf32_Ehdr *hdr);

void dumpelf(Elf32_Ehdr *hdr);

#endif

