#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <stdatomic.h>


//One line macro version from:
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define setBit(arr, int) ( (arr)[(int)/32] |=  1 << ((int) % 32) )
#define testBit(arr, int) ( (arr)[(int)/32] & (1 << ((int) % 32)) )


const unsigned int MAX_THREADS = 2;
int threadCount;

//threads instantiation requires a void func( void* param), so we use a void pointer to a struct
//to pass the parameters

struct threadArgs{

    atomic_int* cycleDone;
    atomic_int* segment;
    int currentPrime;
    int* start;
};


//total array of all numbers <=n
unsigned int* ints;

//input number
int in;

//max addressable index.  ~= in / 32
int iMax;

//result set
int* primes;

//max index of primes with a current value
int pCount = 0;

//max allocated index for primes
int pMax;

void markMultiples ( int n, int segment );
int seekPrime( int* status );
void addPrime( int toAdd );
void* sieve_runner( void* args );
void debug();

int main( int argc, char* argv[] ){

    //performance testing
    clock_t startTime = clock();

    //Base thread config
    pthread_t tids[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    //

    //sentinel to determine when int array has been exhausted and program is finished
    int done = 0;

    //Threads spin until start is set to 1 in main,
    //then main spins until all threads have finished their cycles and set cycleDone == threadCount
    atomic_int segment, cycleDone = 0;
    int start = 0;

    //grab the input, and if there's no narrowing cast, convert it to an int.
    long param = strtol( argv[1], NULL, 10 );
    if( param > UINT_MAX ){

        printf( "Your chosen range is significantly too large.\nPlease try again" );
        exit( EXIT_FAILURE );
    }
    in = (int)param;

    //properly size the base bitarray for 'in' integers, where we store 32 flags per int array bucket
    iMax = (in / 32) + 1;
    ints = malloc( iMax * sizeof(unsigned int) );

    //find best initial approximation for needed prime array size
    //since realloc is expensive.  Uses the PNT
    double initialSize = (double) in / log(in);
    int pMax = (int)initialSize;

    primes = malloc( pMax * sizeof(int) );

    if( primes == NULL ){

        perror( "Could not initially allocate sufficient space in memory" );
        exit(EXIT_FAILURE);
    }

    //manually add 2 to prime array kickstart everything
    primes[0] = 2;
    int currentPrime = 2;

    //instantiate struct because pthread_create can only accept a single pointer to args for runner function.
    //because the struct only holds pointers, all threads can be passed the same struct
    struct threadArgs args;

        args.cycleDone = &cycleDone;
        args.start = &start;
        args.segment = &segment;
        args.currentPrime = currentPrime;

    for( int i = 0; i < iMax; i++ ){

       //this is the equivalent of flagging every representative of an even number in the bitset
       //instead of manually enumerating each bit with markMultiples( 2, in );
       ints[i] = 2863311530;
    }

    for( int i = 0; i < MAX_THREADS; i++ ){

        if( pthread_create( tids + i, &attr, sieve_runner, &args ) ){

            threadCount--;
            perror( "Error creating thread.  Attempting to continue" );
        }

        threadCount++;
        printf("thread created: %u\n", threadCount);
    }

    //need to release the first semaphore so main will run for the first time
    cycleDone = threadCount;

    while( !done ){

        if( cycleDone >= threadCount ){

            cycleDone = 0;
            currentPrime = seekPrime( &done );

            if( 0 == currentPrime ){

                break;
            }
            atomic_store( &segment, threadCount - 1 );
            start = 1;
            addPrime( currentPrime );
        }
    }


    for( int j = 0; j < MAX_THREADS; j++ ){

        pthread_join( tids[j], NULL );
        threadCount--;
    }

    debug();
    clock_t stop = clock();

    double elapsed = (double) (stop - startTime) / CLOCKS_PER_SEC;
    printf("\nTime elapsed: %.5f\n", elapsed);

    free( ints );
    free ( primes );
}

void* sieve_runner( void* args ){

    struct threadArgs* localPtrs = (struct threadArgs*)args;

    atomic_int* local_done = localPtrs -> cycleDone;
    int* local_start = localPtrs -> start;
    atomic_int* local_segment = localPtrs -> segment;
    int local_cPrime = localPtrs -> currentPrime;

    int seg;
    //creating threads isn't super expensive, but isn't super cheap either, so we create few threads
    //and loop the existing threads until the job is done, instead of creating many threads which each execute
    //a single pass of the loop.
    while ( !(*local_done) ){

        if( *local_start ){

            *local_start = 0;
            seg = atomic_fetch_sub( local_segment, 1);
            markMultiples( local_cPrime, seg );
            atomic_fetch_add( local_done, 1);
        }

    }

    //pthread_exit( NULL );
}

/// \details searches a global arr of consecutive ints until an unflagged entry, and adds this entry to the prime array
/// \param pMax largest address of global prime array
/// \param iMax largest address of global int array
/// \param status set to 1 when all integers have been enumerated
int seekPrime( int* status ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    //e.g. the check status of 5 is located at ints[4]
    int i = *(primes + pCount) - 1;

    //search for the first unflagged int in the bitarray
    //if testbits(ints,i) flags true, then i has already been found prime or marked composite
    while( testBit(ints, i) && i < iMax ){

        i++;
    }

    if( i >= iMax ){

        //we have exhausted our integer array and checked for all prime candidates
        *status = 1;
        return 0;
    }

    //flag this prime then pass it to the primes array.
    setBit(ints, i);

    return i + 1;
}

void addPrime( int toAdd ){

    pCount++;
    if( pCount >= pMax ){

        pMax *= 2;
        primes = realloc( primes, pMax * sizeof(int) );

        if( primes == NULL ){

            perror("could not allocate sufficient space.  Try again with a smaller parameter");
            exit(EXIT_FAILURE);
        }
    }

    primes[pCount] = toAdd;

}
/// \details takes a bit array storing iMax values and sets every multiple of n to 1
/// \param n integer to mark multiples of
/// \param iMax maximum multiple to mark
void markMultiples ( int n, int segment ){

    int rangeLength, start, end, gap;

    if( segment < threadCount - 1 ){

        rangeLength = in/threadCount;

        //note that "start" refers to the array index, not the number itself
        //so it will always be 1 too low (e.g. start == 250 means the first checked number will be 251)
        //We get the number value by adding 1, then calculate the first multiple of the given prime
        //in the segment this thread is handling
        start = segment * rangeLength;
        end = start + rangeLength;
        gap = n - ( ( start + 1 ) % n );
        start += gap;
    } else {

        rangeLength = in/threadCount;
        start = segment * rangeLength;
        end = in;
        gap = n - ( ( start + 1 ) % n );
        start += gap;
    }

    //Since even multiples of odd numbers are even, we only need to check the odd multiples
    //we do that by jumping 2n bits instead of n bits when checking multiples
    n *= 2;

    while ( start + n < end  ){

        start += n;
        setBit( ints, start );
    }
}

void debug(){

    for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);
    }

    printf( "\n pCount final value: %d", pCount);
}