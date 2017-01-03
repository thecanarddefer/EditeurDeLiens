#include <stdio.h>
#include <stdlib.h>
#include <linux/elf.h>
#include "readelf.h"


int main(int argc, char *argv[])
{
	Elf32_Ehdr *hdr = malloc(sizeof(Elf32_Ehdr));

	readelf(hdr);
	dumpelf(hdr);

	free(hdr);
	return 0;
}

int readelf(Elf32_Ehdr *hdr)
{

	return 0;
}

void dumpelf(Elf32_Ehdr *hdr)
{
}

