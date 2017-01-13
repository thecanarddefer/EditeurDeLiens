#!/bin/bash

DIR=$(dirname $0)

$DIR/../fusion $1 $2 $3
readelf -h -S -s -r prog.o > /tmp/fusion_project.txt

arm-none-eabi-ld -r -o $3 $1 $2
readelf -h -S -s -r prog.o > /tmp/fusion_system.txt

vimdiff /tmp/fusion_project.txt /tmp/fusion_system.txt

rm /tmp/fusion_project.txt /tmp/fusion_system.txt
