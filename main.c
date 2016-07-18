#include "data_structure.h"
#include "json_read.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{
	const char* filepath;
	if (argc < 2)
	{
		printf("Missing arugments.\n \
				Usage: input file path");
	} else
	{
		filepath = argv[1];
		printf("Filepath: %s\n", filepath);
	}

	return 0;
}