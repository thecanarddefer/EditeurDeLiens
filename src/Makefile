CC=gcc
CFLAGS=-Wall -g -std=c99

readelf: readelf.o elf_common.o readelf.h
	$(CC) $(CFLAGS) -o $@ $^

readelf.o: readelf.c readelf.h
	$(CC) $(CFLAGS) -c $<

elf_common.o: elf_common.c readelf.h
	$(CC) $(CFLAGS) -c $<

