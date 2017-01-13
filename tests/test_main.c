#include <stdio.h>

int add(int a, int b);
int loop(int a);

int main(int argc, char *argv[])
{
	int var = 42;
	printf("C'est la fusion !\n");
	printf("%i\n", add(5, 5));
	printf("%i\n", loop(var));
	return 0;
}

