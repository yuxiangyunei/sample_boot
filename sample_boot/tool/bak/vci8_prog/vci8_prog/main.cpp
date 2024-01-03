#include <stdio.h>
#include "vci_prog.h"

int main(int argc, char *argv[])
{
	int ret;

	if (argc != 3)
	{
		printf("USAGE: %s ip_address hex_file_name\n", argv[0]);
		ret = -1;
	}
	else
	{
		ret = vci_prog(argv[1], argv[2], NULL);
	}
	return ret;
}

