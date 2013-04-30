#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "mersy.h"

void Usage(void)
{
	printf("mersy [<starting P value>]\n");
}

int main(int argc, char *argv[])
{
	int threadCount, startP;

	if (argc > 2)
	{
		Usage();
		return -1;
	}

	if (argc >= 2)
	{
		// Read the starting value from the parameter list
		startP = atoi(argv[1]);
	}
	else
	{
		// Default starts at 2
		startP = 2;
	}

	// We want as many threads as processors
	threadCount = sysconf(_SC_NPROCESSORS_ONLN);

	// Start the worker threads with this thread being the arbiter
	FindPrimes(threadCount, startP);

	// FindPrimes() only returns in the case of an error
	return -1;
}
