CC=gcc
CFLAGS=-Wall -g -std=c99
EXE=readelf ld

all: $(EXE)

readelf: readelf.o elf_common.o symbolTable.o readelf.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -o $@ $^

readelf.o: readelf.c readelf.h elf_common.h
	$(CC) $(CFLAGS) -c $<

elf_common.o: elf_common.c elf_common.h
	$(CC) $(CFLAGS) -c $<

symbolTable.o: symbolTable.c symbolTable.h
	$(CC) $(CFLAGS) -c $<

ld: ld.o elf_common.o symbolTable.o ld.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -o $@ $^

ld.o: ld.c ld.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -c $<

clean:
	@rm *.o

mrproper: clean
	@rm $(EXE)

.PHONY: clean mrproper

