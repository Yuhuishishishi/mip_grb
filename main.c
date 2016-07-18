#include "solver.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	const char* filepath;

	if (argc < 2)
	{
		printf("Missing arugments.\n \
				Usage: input file path");
		return 1;
	} else
	{
		filepath = argv[1];
		printf("Filepath: %s\n", filepath);
	}

	solve(filepath);
	


	return 0;
}