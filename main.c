#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <stdatomic.h>

//TODO: Remove a billion debug statements
//TODO: change iterators to use sqrt(n) instead of n

//One line macro version from:
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define setBit(arr, int) ( (arr)[(int)/32] |=  1 << ((int) % 32) )
#define testBit(arr, int) ( (arr)[(int)/32] & (1 << ((int) % 32)) )


const unsigned int MAX_THREADS = 2;

//threads instantiation requires a void func( void* param), so we use a void pointer to a struct
//to pass the parameters

struct threadArgs{

    atomic_int* cycleDone;
    int* done;
    atomic_int* segment;
    int* currentPrime;
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

    //Threads spin until start[] is set to 1 in main for each thread,
    //then main spins until all threads have finished their cycles and set cycleDone == MAX_THREADS
    atomic_int segment = MAX_THREADS, cycleDone = MAX_THREADS;
    int start[MAX_THREADS];
    for( int i = 0; i < MAX_THREADS; i++ ){

        start[i] = 0;
    }

    atomic_store( &segment, MAX_THREADS - 1 );

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
    pMax = (int)initialSize;

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
        args.done = &done;
        args.segment = &segment;
        args.currentPrime = &currentPrime;
        args.start = start;

    for( int i = 0; i < iMax; i++ ){

       //this is the equivalent of flagging every representative of an even number in the bitset
       //instead of manually enumerating each bit with markMultiples( 2, in );
       ints[i] = 2863311530;
    }

    for( int i = 0; i < MAX_THREADS; i++ ){

        if( pthread_create( tids + i, &attr, sieve_runner, &args ) ){

            perror( "System Error: cannot create thread" );
            exit(EXIT_FAILURE);
        }

        //printf("thread created: %u\n", i);
    }

    while( !done ){

        if( cycleDone >= MAX_THREADS ){

            //printf( "starting main cycle\n");
            cycleDone = 0;
            currentPrime = seekPrime( &done );

            if( 0 == currentPrime ){

                break;
            }

            for( int i = 0; i < MAX_THREADS; i++ ){

                start[i] = 1;
            }

            //printf("Found prime: %u\n", currentPrime);
            addPrime( currentPrime );
        }
    }


    for( int j = 0; j < MAX_THREADS; j++ ){

        pthread_join( tids[j], NULL );
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

    atomic_int* local_cycleDone = localPtrs -> cycleDone;
    int* local_done = localPtrs -> done;
    int* local_start = localPtrs -> start;
    atomic_int* local_segment = localPtrs -> segment;
    int* local_cPrime = localPtrs -> currentPrime;

    //printf( "thread %u attempting to grab segment\n", pthread_self());
    int seg = atomic_fetch_sub( local_segment, 1 );
    //printf("thread %u got segment %u\n", pthread_self(), seg);

    //creating threads isn't super expensive, but isn't super cheap either, so we create few threads
    //and loop the existing threads until the job is done, instead of creating many threads which each execute
    //a single pass of the loop.
    while ( !(*local_done) ){

        if( local_start[seg] ){

            local_start[seg] = 0;
            markMultiples( *local_cPrime, seg );
            //printf("thread %u attempting to finish\n", pthread_self());
            atomic_fetch_add( local_cycleDone, 1);
            //printf("thread %u finished and cycleDone = %u\n", pthread_self(), *local_cycleDone);
        }

    }

    pthread_exit( NULL );
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
    while( testBit(ints, i) && i < in ){

        i++;
    }

    if( i >= in ){

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

    //printf("thread %u has segment %u\n", pthread_self(), segment);
    int rangeLength, start, end, gap;

    if( segment < MAX_THREADS - 1 ){

        rangeLength = in / MAX_THREADS;
        //printf("rangelength = %u\n", rangeLength);

        //note that "start" refers to the array index, not the number itself
        //so it will always be 1 too low (e.g. start == 250 means the first checked number will be 251)
        //We get the number value by adding 1, then calculate the first multiple of the given prime
        //in the segment this thread is handling
        start = segment * rangeLength;
        end = start + rangeLength;
        gap = n - ( ( start + 1 ) % n );
        start += gap;

        //printf("starting at %u, ending at %u\n", start, end);
    } else {

        rangeLength = in / MAX_THREADS;
        start = segment * rangeLength;
        //printf("original start: %u\n", start);
        end = in;
        gap = n - ( ( start + 1 ) % n );

        if( n == gap ){

            gap = 0;
        }
        //printf("gap value = %u\n", gap);
        start += gap;
        //printf("starting at %u, ending at %u, with prime %u\n", start, end, n);
    }

    while ( start < end  ){

        //printf("marking %u, which represents %u\n", start, start + 1);
        setBit( ints, start );
        start += n;
    }

    //printf("ended cycle %u\n", n);
}

void debug(){

    /*for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);
    }*/

    printf( "\n pCount final value: %d", pCount);
}