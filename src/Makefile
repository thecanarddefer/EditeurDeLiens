CC=gcc
CFLAGS=-Wall -g -std=c99
EXE=readelf fusion

all: $(EXE)

readelf: readelf.o elf_common.o symbolTable.o readelf.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -o $@ $^

readelf.o: readelf.c readelf.h elf_common.h
	$(CC) $(CFLAGS) -c $<

elf_common.o: elf_common.c elf_common.h
	$(CC) $(CFLAGS) -c $<

symbolTable.o: symbolTable.c symbolTable.h
	$(CC) $(CFLAGS) -c $<

fusion: fusion.o elf_common.o symbolTable.o fusion.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -o $@ $^

fusion.o: fusion.c fusion.h elf_common.h symbolTable.h
	$(CC) $(CFLAGS) -c $<

clean:
	@rm *.o

mrproper: clean
	@rm $(EXE)

.PHONY: clean mrproper

