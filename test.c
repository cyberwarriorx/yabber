#include <conio.h>
#include "aosdk/ao.h"
#include "aosdk/corlett.h"

int main(int argc, char *argv[])
{
	char buf[4];
	if (argc != 2)
		return 1;

	if (!load_ssf(argv[1]))
		return 1;

	while (!kbhit())
	{
		if (ssf_gen((short *)buf, 1) == AO_FAIL)
			break;
	}

	return 0;
}