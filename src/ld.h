#ifndef _LD_H_
#define _LD_H_

#include <elf.h>


typedef struct
{
	Elf32_Shdr *ptr_shdr1;
	Elf32_Shdr *ptr_shdr2;
	Elf32_Word size;
	Elf32_Off offset;
} Fusion;

/**
 * Recopie une section PROGBITS dans un autre fichier
 *
 * PRÉ-CONDITION: le curseur de fd_out est placé au bon endroit
 * @param fd_in:  un descrpteur de fichier vers le fichier d'entrée
 * @param fd_out: un descrpteur de fichier vers le fichier de sortie
 * @param: size:  le nombre d'octets à recopier
 * @param offset: l'adresse à partir de laquelle lire dans le fichier d'entrée
 * @retourne 0 si la copie s'est bien déroulée
 **/
int write_progbits_in_file(int fd_in, int fd_out, Elf32_Word size, Elf32_Off offset);


#endif
