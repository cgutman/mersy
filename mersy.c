// Standard library
#include <stdio.h>
#include <stdlib.h>

// 3rd party libraries
#include "pthread.h"
#include "gmp.h"

// Implementation-specific header (include last)
#include "mersy.h"

// Definitions related to GMP for convenience
#define GMP_DEF_COMPOSITE (0)
#define GMP_PROB_PRIME (1)
#define GMP_DEF_PRIME (2)

typedef struct _CALC_THREAD_CONTEXT
{
	// Thread temporaries
	mpz_t nextPrime;

	// Thread tracking
	pthread_t id;
	int threadIndex;
} CALC_THREAD_CONTEXT, *PCALC_THREAD_CONTEXT;

// General note regarding the usage of mpz_nextprime():
// This function CAN return numbers that are COMPOSITE in rare
// cases. Our usage does not depend on the fact that numbers returned
// by this function are always prime, but the higher the percentage
// of primes returned, the less time the program will spend examining
// numbers that are definitely not Mersenne primes.

pthread_mutex_t NextPMutex;
mpz_t NextPrimeP;

// This PrimeTest implementation uses the Lucas-Lehmer primality test
static int PrimeTest(unsigned long p, mpz_t *potentialPrime)
{
	mpz_t s;
	unsigned long i;

	// LLT starts at 4
	mpz_init_set_ui(s, 4);

	// LLT loop
	for (i = 0; i < p - 2; i++)
	{
		mpz_mul(s, s, s);
		mpz_sub_ui(s, s, 2);
		mpz_mod(s, s, *potentialPrime);
	}

	return (mpz_cmp_ui(s, 0) == 0) ? GMP_DEF_PRIME : GMP_DEF_COMPOSITE;
}

static unsigned long GetNextPrimeP(void)
{
	unsigned long nextP;

	// Acquire the lock to safely modify the next P
	pthread_mutex_lock(&NextPMutex);

	// Store the existing value to return to the caller
	nextP = mpz_get_ui(NextPrimeP);

	// Compute the new value
	mpz_nextprime(NextPrimeP, NextPrimeP);

	// Release the lock
	pthread_mutex_unlock(&NextPMutex);

	return nextP;
}

// stP is inclusive and prime; endP is exclusive
static void* CalculationThread(void *context)
{
	PCALC_THREAD_CONTEXT tcontext = (PCALC_THREAD_CONTEXT)context;
	unsigned long P, setBits;
	unsigned int primality;
	char *primeStr;

	setBits = 0;
	for (;;)
	{
		// Reserve the next prime P value for us to test
		P = GetNextPrimeP();

		// Print a message to the terminal
		PRINT_MSG(MSG_VERBOSE, "Thread %d: Testing P=%lu", tcontext->threadIndex, P);

		// Only set bits that weren't set before
		while (setBits < P)
		{
			mpz_setbit(tcontext->nextPrime, setBits);
			setBits++;
		}

		// Check the result for primality
		primality = mpz_probab_prime_p(tcontext->nextPrime, DIVISIONS_FOR_PRIMALITY);
		switch (primality)
		{
		case GMP_DEF_COMPOSITE:
			// Composite number is composite
			break;

		case GMP_PROB_PRIME:
			// Probably prime, but we need to check for sure
			if (PrimeTest(P, &tcontext->nextPrime) != GMP_DEF_PRIME)
			{
				// More extensive test showed it was composite
				break;
			}

			// Else fall through to prime case

		case GMP_DEF_PRIME:
			// Definitely prime
                        primeStr = mpz_get_str(NULL, 10, tcontext->nextPrime);
			if (primeStr)
			{
				PRINT_MSG(MSG_INFO, "Thread %d --- Mersenne prime found (P=%lu): %s",
				                          tcontext->threadIndex, P, primeStr);
				free(primeStr);
			}
			else
			{
				PRINT_MSG(MSG_WARNING, "Unable to allocate memory to output prime!");
			}
			break;
		}
	}

	// Unreachable code
	return NULL;
}

void FindPrimes(unsigned int ThreadCount, unsigned int StartingPValue)
{
	PCALC_THREAD_CONTEXT threads;
	unsigned int i;
	int err;

	threads = (PCALC_THREAD_CONTEXT) malloc(sizeof(*threads) * ThreadCount);
	if (threads == NULL)
		return;

	// Initialize the lock to synchronize fetching a new P value
	pthread_mutex_init(&NextPMutex, NULL);

	// Initialize the next prime P
	mpz_init_set_ui(NextPrimeP, StartingPValue);
	mpz_sub_ui(NextPrimeP, NextPrimeP, 1);
	mpz_nextprime(NextPrimeP, NextPrimeP);

	// Setup and start the threads
	for (i = 0; i < ThreadCount; i++)
	{
		// Do one-time setup of thread context index
		threads[i].threadIndex = i;

		// Initialize the temps for this thread
		mpz_init(threads[i].nextPrime);

		// Spawn the thread
		err = pthread_create(&threads[i].id, NULL, CalculationThread, &threads[i]);
		if (err)
		{
			PRINT_MSG(MSG_WARNING, "Unable to create thread %d!", threads[i].threadIndex);

			// Cleanup this thread's state
			mpz_clear(threads[i].nextPrime);

			// Modify the thread count to the actual number of created threads
			ThreadCount = i;

			// Execution can continue fine from here
			break;
		}
	}

	// Join the threads (which hopefully shouldn't terminate anyways)
	for (i = 0; i < ThreadCount; i++)
	{
		pthread_join(threads[i].id, NULL);

		PRINT_MSG(MSG_WARNING, "Unexpected termination of thread %d!", threads[i].threadIndex);

		// Cleanup this thread's working state
		mpz_clear(threads[i].nextPrime);
	}

	// Everybody's dead :(
	mpz_clear(NextPrimeP);
	pthread_mutex_destroy(&NextPMutex);
	free(threads);

	PRINT_MSG(MSG_ERROR, "All threads died!");
}
