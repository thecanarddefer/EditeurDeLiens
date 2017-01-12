#ifndef _READELF_H_
#define _READELF_H_

#include <elf.h>
#include "elf_common.h"

#define DSP_FILE_HEADER     (1 << 0)
#define DSP_SECTION_HEADERS (1 << 1)
#define DSP_HEX_DUMP        (1 << 2)
#define DSP_SYMS            (1 << 3)
#define DSP_RELOCS          (1 << 4)


typedef struct
{
	unsigned display;
	unsigned nb_hexdumps;
	unsigned section_ind[32];
	char     section_str[32][32];
} Arguments;

#endif
