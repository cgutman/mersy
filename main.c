#include <stdlib.h>
#include <stdio.h>

#include "mersy.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void Usage(void)
{
	printf("mersy [<starting P value>]\n");
}

int GetOptimalThreadCount(void)
{
#ifdef _WIN32 /* Windows */
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwNumberOfProcessors;
#else /* POSIX */
	return sysconf(_SC_NPROCESSORS_CONF);
#endif
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

	// Call the platform-specific code for CPU count detection
	threadCount = GetOptimalThreadCount();

	// Start the worker threads with this thread being the arbiter
	FindPrimes(threadCount, startP);

	// FindPrimes() only returns in the case of an error
	return -1;
}
