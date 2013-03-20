#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include <gmp.h>

// Configurable settings
#define DIVISIONS_FOR_PRIMALITY (3)
#define P_RANGE_PER_THREAD (100)

// Definitions related to GMP for convenience
#define GMP_DEF_COMPOSITE (0)
#define GMP_PROB_PRIME (1)
#define GMP_DEF_PRIME (2)

typedef struct _CALC_THREAD_CONTEXT
{
	mpz_t stP, endP;
	pthread_t id;
	int threadIndex;
} CALC_THREAD_CONTEXT, *PCALC_THREAD_CONTEXT;

// This variable has bits set that represent the terminated status of calc threads
unsigned long long TerminationBits;
pthread_mutex_t TerminationMutex;
pthread_cond_t TerminationVariable;

// General note regarding the usage of mpz_nextprime():
// This function CAN return numbers that are COMPOSITE in rare
// cases. Our usage does not depend on the fact that numbers returned
// by this function are always prime, but the higher the percentage
// of primes returned, the less time the program will spend examining
// numbers that are definitely not Mersenne primes.


// This PrimeTest implementation uses the Lucas-Lehmer primality test
int PrimeTest(unsigned long p, mpz_t *potentialPrime)
{
	mpz_t s;
	unsigned long i;
	int cmp;

	// LLT starts at 4
	mpz_init_set_ui(s, 4);

	printf("LLT(P=%lu) START\n", p);

	// LLT loop
	for (i = 0; i < p - 2; i++)
	{
		mpz_mul(s, s, s);
		mpz_sub_ui(s, s, 2);
		mpz_mod(s, s, *potentialPrime);
	}

	printf("LLT(P=%lu) END\n", p);

	cmp = mpz_cmp_ui(s, 0);
	if (cmp == 0)
	{
		return GMP_DEF_PRIME;
	}
	else
	{
		printf("LLT(P=%lu) LIAR (Residue: %lu)\n", p, mpz_get_ui(s));
		return GMP_DEF_COMPOSITE;
	}
}

// stP is inclusive; endP is exclusive
void* CalculationThread(void *context)
{
	PCALC_THREAD_CONTEXT tcontext = (PCALC_THREAD_CONTEXT)context;
	mpz_t nextPrime, nextP;
	unsigned long p, i, endP;
	int primality;

	// Fetch the end P value
	endP = mpz_get_ui(tcontext->endP);

	// Initialize the GMP variables
	mpz_init_set(nextP, tcontext->stP);
	mpz_init(nextPrime);

	// Decrement nextP and jump to the next prime
	mpz_sub_ui(nextP, nextP, 1);
	mpz_nextprime(nextP, nextP);

	// We don't set every bit each time, because we're guaranteed
	// that p > oldP. We just set the bits that weren't set before.
	i = 0;

	// Loop until P is >= endP
	while ((p = mpz_get_ui(nextP)) < endP)
	{
		// Only set bits that weren't set before
		while (i < p)
		{
			mpz_setbit(nextPrime, i);
			i++;
		}

		// Check the result for primality
		primality = mpz_probab_prime_p(nextPrime, DIVISIONS_FOR_PRIMALITY);
		switch (primality)
		{
		case GMP_DEF_COMPOSITE:
			// Composite number is composite
			break;

		case GMP_PROB_PRIME:
			// Probably prime, but we need to check for sure
			if (PrimeTest(p, &nextPrime) != GMP_DEF_PRIME)
			{
				// More extensive test showed it was composite
				break;
			}

			// Else fall through to prime case

		case GMP_DEF_PRIME:
			// Definitely prime
			printf("Thread %d --- Mersenne prime found (P=%lu): ", tcontext->threadIndex, p);
			mpz_out_str(stdout, 10, nextPrime);
			printf("\n");
			fflush(stdout);
			break;
		}

		// Skip to the next P
		mpz_nextprime(nextP, nextP);
	}

	// Free the GMP variables
	mpz_clear(nextP);
	mpz_clear(nextPrime);

	// Set the termination bit to notify the arbiter that this thread needs to be respawned
	// with more work.
	pthread_mutex_lock(&TerminationMutex);
	TerminationBits |= (1 << tcontext->threadIndex);
	pthread_cond_signal(&TerminationVariable);
	pthread_mutex_unlock(&TerminationMutex);

	// Return the thread context when we terminate
	pthread_exit(context);
}

void usage(void)
{
	printf("mersy [<starting P value>]\n");
}

int main(int argc, char *argv[])
{
	PCALC_THREAD_CONTEXT threads;
	int threadCount, i, err;
	unsigned long long st;

	if (argc > 2)
	{
		usage();
		return -1;
	}

	if (argc >= 2)
	{
		// Read the starting value from the parameter list
		st = atoi(argv[1]);
	}
	else
	{
		// Default starts at 2
		st = 2;
	}

	threadCount = sysconf(_SC_NPROCESSORS_ONLN);

	threads = (PCALC_THREAD_CONTEXT) malloc(sizeof(*threads) * threadCount);
	if (threads == NULL)
		return -1;

	pthread_mutex_init(&TerminationMutex, NULL);
	pthread_cond_init(&TerminationVariable, NULL);

	// Setup some initial state of the thread contexts
	for (i = 0; i < threadCount; i++)
	{
		// Do one-time setup of thread context index
		threads[i].threadIndex = i;

		// Initialize the start and end GMP variables
		mpz_init(threads[i].stP);
		mpz_init(threads[i].endP);

		// Set termination bit in order for the arbiter to respawn the thread
		TerminationBits |= (1 << i);
	}

	// This is the arbitration loop that handles work assignments as threads complete
	// their assigned work blocks.
	pthread_mutex_lock(&TerminationMutex);
	for (;;)
	{
		for (i = 0; i < threadCount; i++)
		{
			// If the thread has indicated that it's terminated, respawn it.
			if ((1 << i) & TerminationBits)
			{
				// Setup the context again
				mpz_set_ui(threads[i].stP, st);
				st += P_RANGE_PER_THREAD;
				mpz_set_ui(threads[i].endP, st);

				// Clear the termination bit
				TerminationBits &= ~(1 << i);

				printf("Thread %d: Starting P=%lu to P=%lu\n", i,
				        mpz_get_ui(threads[i].stP), mpz_get_ui(threads[i].endP));

				// Spawn another thread
				err = pthread_create(&threads[i].id, NULL, CalculationThread, &threads[i]);
				if (err != 0)
					return -1;
			}
		}

		// Flush output stream
		fflush(stdout);

		// Wait on the termination bits to change. This releases the
		// mutex while the wait is in progress to avoid a deadlock.
		// The function returns with the mutex locked. These mutex operations
		// are atomic. This wait is done after the loop in order to handle the
		// initial setup of the threads.
		pthread_cond_wait(&TerminationVariable, &TerminationMutex);
	}
	pthread_mutex_unlock(&TerminationMutex);
}
