/*
ELF Loader - chargeur/implanteur d'ex�cutables au format ELF � but p�dagogique
Copyright (C) 2012 Guillaume Huard
Ce programme est libre, vous pouvez le redistribuer et/ou le modifier selon les
termes de la Licence Publique G�n�rale GNU publi�e par la Free Software
Foundation (version 2 ou bien toute autre version ult�rieure choisie par vous).

Ce programme est distribu� car potentiellement utile, mais SANS AUCUNE
GARANTIE, ni explicite ni implicite, y compris les garanties de
commercialisation ou d'adaptation dans un but sp�cifique. Reportez-vous � la
Licence Publique G�n�rale GNU pour plus de d�tails.

Vous devez avoir re�u une copie de la Licence Publique G�n�rale GNU en m�me
temps que ce programme ; si ce n'est pas le cas, �crivez � la Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
�tats-Unis.

Contact: Guillaume.Huard@imag.fr
         ENSIMAG - Laboratoire LIG
         51 avenue Jean Kuntzmann
         38330 Montbonnot Saint-Martin
*/

#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>


int elf32_is_big = 0;

int is_big_endian() {
    static uint32_t one = 1;
    return ((* (uint8_t *) &one) == 0);
}

ssize_t __wrap_read(int fildes, void *buf, size_t nbyte)
{
        ssize_t r = __real_read(fildes, buf, nbyte);

        if((is_big_endian() && !elf32_is_big) || (!is_big_endian() && elf32_is_big))
        {
                if(nbyte == sizeof(uint32_t))
                {
                        uint32_t tmp1 = *((uint32_t*) buf);
                        uint32_t *tmp2 = (uint32_t*) buf;
                        *tmp2 = reverse_4(tmp1);
                }
                else if(nbyte == sizeof(uint16_t))
                {
                        uint16_t tmp1 = *((uint16_t*) buf);
                        uint16_t *tmp2 = (uint16_t*) buf;
                        *tmp2 = reverse_2(tmp1);
                }
        }

        return r;
}

int print_debug(const char *format, ...)
{
	if(getenv("DEBUG_FUSION") == NULL)
		return 0;

	va_list aptr;
	va_start(aptr, format);
	int ret = vprintf(format, aptr);
	va_end(aptr);
	return ret;
}
