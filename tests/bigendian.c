#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


static uint32_t move_bit(uint32_t val, uint8_t pos)
{
	return (val << pos);
}

int main(int argc, char *argv[])
{
	int i;

	for(i = 0; i < 32; i++)
		printf("Décalage de %i : %u\n", i, move_bit(42, i));

	return 0;
}
