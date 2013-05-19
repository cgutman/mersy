// Standard library includes
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Mersy definitions and our platform header
#include "mersy.h"

// Platform-specific headers
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

// Current trace level (prints all messages at this level and above)
int TraceLevel = MSG_ALL;

// Prints the default message header
void PrintMsgHeader(int tag)
{
#define BUFF_LEN 100
	char buff[BUFF_LEN];
	time_t now;

	// Print the time
	now = time(NULL);
	strftime(buff, BUFF_LEN, "%m/%d/%Y %H:%M:%S", localtime(&now));
	printf("%s ", buff);

	// Print the tag header
	switch (tag)
	{
		case MSG_WARNING:
			printf("WARNING: ");
			break;

		case MSG_ERROR:
			printf("ERROR: ");
			break;

		default:
			break;
	}
#undef BUFF_LEN
}
void Usage(void)
{
	printf("mersy [<starting P value>]\n");
}

// Platform-specific function to get the number of threads to make
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
