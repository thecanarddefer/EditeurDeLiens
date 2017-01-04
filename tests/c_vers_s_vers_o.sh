#!/bin/bash

FILE=$(basename $1 .c)

if [[ $# -lt 1 ]]; then
	echo "$0 : un nom de fichier est requis en argument."
	exit 1
fi

arm-none-eabi-gcc -mno-thumb-interwork -S $1
arm-none-eabi-as -o "$FILE.s" "$FILE.s"

