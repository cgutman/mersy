#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <gmp.h>

// Configurable settings
#define DIVISIONS_FOR_PRIMALITY (1)
#define P_RANGE_PER_THREAD (100)

// Definitions related to GMP for convenience
#define GMP_DEF_COMPOSITE (0)
#define GMP_PROB_PRIME (1)
#define GMP_DEF_PRIME (2)

typedef struct _CALC_THREAD_CONTEXT
{
	// Thread parameters
	mpz_t stP, endP;

	// Thread temporaries
	mpz_t nextPrime;
	int setBits;

	// Thread tracking
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

// stP is inclusive; endP is exclusive
static void* CalculationThread(void *context)
{
	PCALC_THREAD_CONTEXT tcontext = (PCALC_THREAD_CONTEXT)context;
	unsigned long stP, p, i, endP;
	unsigned int primality;

	// Fetch the start and end P value
	stP = mpz_get_ui(tcontext->stP);
	endP = mpz_get_ui(tcontext->endP);

	// Realloc storage for the prime. We actually know the number
	// of bits that the prime will contain at maximum because we know
	// what the end P value will be. This is convenient because GMP can
	// preallocate this memory for us and save us time during calculations.
	mpz_realloc2(tcontext->nextPrime, endP);

	// Decrement stP and jump to the next prime
	mpz_sub_ui(tcontext->stP, tcontext->stP, 1);
	mpz_nextprime(tcontext->stP, tcontext->stP);

	// We don't set every bit each time, because we're guaranteed
	// that p > oldP. We just set the bits that weren't set before.
	i = tcontext->setBits;

	// Loop until P is >= endP
	while ((p = mpz_get_ui(tcontext->stP)) < endP)
	{
		// Only set bits that weren't set before
		while (i < p)
		{
			mpz_setbit(tcontext->nextPrime, i);
			i++;
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
			if (PrimeTest(p, &tcontext->nextPrime) != GMP_DEF_PRIME)
			{
				// More extensive test showed it was composite
				break;
			}

			// Else fall through to prime case

		case GMP_DEF_PRIME:
			// Definitely prime
			printf("Thread %d --- Mersenne prime found (P=%lu): ", tcontext->threadIndex, p);
			mpz_out_str(stdout, 10, tcontext->nextPrime);
			printf("\n");
			fflush(stdout);
			break;
		}

		// Skip to the next P
		mpz_nextprime(tcontext->stP, tcontext->stP);
	}

	// Write the i value back
	tcontext->setBits = i;

	// Print the thread finished message
	printf("Thread %d: Finished P=%lu to P=%lu\n", tcontext->threadIndex, stP, endP);
	fflush(stdout);

	// Set the termination bit to notify the arbiter that this thread needs to be respawned
	// with more work.
	pthread_mutex_lock(&TerminationMutex);
	TerminationBits |= (1ULL << tcontext->threadIndex);
	pthread_cond_signal(&TerminationVariable);
	pthread_mutex_unlock(&TerminationMutex);

	// Return the thread context when we terminate
	pthread_exit(context);

	// pthread_exit() doesn't return
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

	pthread_mutex_init(&TerminationMutex, NULL);
	pthread_cond_init(&TerminationVariable, NULL);

	// Setup some initial state of the thread contexts
	for (i = 0; i < ThreadCount; i++)
	{
		// Do one-time setup of thread context index
		threads[i].threadIndex = i;

		// Initialize the start and end GMP variables
		mpz_init(threads[i].stP);
		mpz_init(threads[i].endP);

		// Initialize the temps for this thread
		mpz_init(threads[i].nextPrime);
		threads[i].setBits = 0;

		// Set termination bit in order for the arbiter to respawn the thread
		TerminationBits |= (1ULL << i);
	}

	// This is the arbitration loop that handles work assignments as threads complete
	// their assigned work blocks.
	pthread_mutex_lock(&TerminationMutex);
	for (;;)
	{
		for (i = 0; i < ThreadCount; i++)
		{
			// If the thread has indicated that it's terminated, respawn it.
			if ((1ULL << i) & TerminationBits)
			{
				// Setup the context again
				mpz_set_ui(threads[i].stP, StartingPValue);
				StartingPValue += P_RANGE_PER_THREAD;
				mpz_set_ui(threads[i].endP, StartingPValue);

				// Clear the termination bit
				TerminationBits &= ~(1ULL << i);

				printf("Thread %d: Starting P=%lu to P=%lu\n", i,
				        mpz_get_ui(threads[i].stP), mpz_get_ui(threads[i].endP));
				fflush(stdout);

				// Spawn another thread
				err = pthread_create(&threads[i].id, NULL, CalculationThread, &threads[i]);
				if (err != 0)
					return;
			}
		}

		// Wait on the termination bits to change. This releases the
		// mutex while the wait is in progress to avoid a deadlock.
		// The function returns with the mutex locked. These mutex operations
		// are atomic. This wait is done after the loop in order to handle the
		// initial setup of the threads.
		pthread_cond_wait(&TerminationVariable, &TerminationMutex);
	}
	pthread_mutex_unlock(&TerminationMutex);
}
