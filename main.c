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

//threads instantiation requires a void func( void* param), so we use a void pointer to a struct
//to pass the parameters

struct threadArgs{

    atomic_int* cycleDone;
    int* done;
    atomic_int* segment;
    int* currentPrime;
    int* start;
};

//input number
int in;

//ints array holds the discrete interval [1, input]; primes holds the found primes
//each array has a max variable denoting its maximum allocated size, and primes
//has a count variable holding the most recently assigned index
unsigned int* ints;
int* primes;

//iMax ~= in / 32 because we use a bitArray
//pMax = PNT(in) * 2 ^ k for some k, initially 0
int iMax, pMax;
int pCount = 0;

int seekPrime( int* status );
void addPrime( int toAdd );
void* sieve_runner( void* args );
void markMultiples ( int n, int segment );
void debug();

int main( int argc, char* argv[] ){

    //performance testing
    clock_t startTime = clock();

    //Base thread config
    pthread_t tids[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    //

    //sentinels to determine when individual runs have completed
    //and when int array has been exhausted and program is finished
    atomic_int cycleDone = MAX_THREADS;
    int done = 0;

    //Threads spin until start[] is set to 1 for each thread, based on the segment that thread is handling
    //then main spins until all threads have finished their cycles and set cycleDone == MAX_THREADS
    atomic_int segment;
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

    //manually add 2 to prime array and mark every even number in the bitarray kickstart everything
    primes[0] = 2;
    int currentPrime = 2;

    for( int i = 0; i < iMax; i++ ){

        //this is the equivalent of manually flagging each bit with markMultiples( 2, in );
        ints[i] = 2863311530;
    }

    //instantiate struct because pthread_create can only accept a single pointer to args for runner function.
    //because the struct only holds pointers, all threads can be passed the same struct
    struct threadArgs args;

        args.cycleDone = &cycleDone;
        args.done = &done;
        args.segment = &segment;
        args.currentPrime = &currentPrime;
        args.start = start;

    for( int i = 0; i < MAX_THREADS; i++ ){

        if( pthread_create( tids + i, &attr, sieve_runner, &args ) ){

            perror( "System Error: cannot create thread" );
            exit(EXIT_FAILURE);
        }
    }

    //main loop.  Runs once then spins until all threads signal done,
    //then threads spin until a prime is found
    //finally causes all threads to run with start[segment] for all segments and loops back.
    while( !done ){

        if( cycleDone >= MAX_THREADS ){

            cycleDone = 0;
            currentPrime = seekPrime( &done );

            if( 0 == currentPrime ){ // ...|| done )

                break;
            }

            //once currentPrime has been set, hand off to threads to mark
            //multiples in ints
            for( int i = 0; i < MAX_THREADS; i++ ){

                start[i] = 1;
            }

            addPrime( currentPrime );
        }
    }

    //once we've marked all composite numbers, we just have to grab
    //the remaining primes > sqrt( in ), remembering that flag i represents number i + 1
    for( int i = (int)sqrt( (double)in ); i < in; i++ ){

        if( !testBit(ints, i) ){

            addPrime( i + 1);
        }
    }

    for( int j = 0; j < MAX_THREADS; j++ ){

        pthread_join( tids[j], NULL );
    }

    debug();
    clock_t stop = clock();

    //Note CLOCKS_PER_SEC is a constant 1,000,000/s and does not reflect system performance, so all times are nominal
    double elapsed = (double) (stop - startTime) / CLOCKS_PER_SEC;
    printf("\n Normalized Time elapsed: %.5f\n", elapsed);

    free( ints );
    free ( primes );
}

/// \brief spins threads until main sets start[], then marksMultiples, increment cycleDone and spin
/// \param args threadArgs struct
void* sieve_runner( void* args ){

    struct threadArgs* localPtrs = (struct threadArgs*)args;
        atomic_int* local_cycleDone = localPtrs -> cycleDone;
        int* local_done = localPtrs -> done;
        int* local_start = localPtrs -> start;
        atomic_int* local_segment = localPtrs -> segment;
        int* local_cPrime = localPtrs -> currentPrime;

    //each thread is guaranteed to get a unique segment number in the discrete interval [0, MAX_THREADS - 1]
    int seg = atomic_fetch_sub( local_segment, 1 );

    //creating threads isn't super expensive, but isn't super cheap either, so we create few threads
    //and loop the existing threads until the job is done, instead of creating many threads which each execute
    //a single pass of the loop.
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wfor-loop-analysis" //loop condition variable is set outside loop.
    while ( !(*local_done) ){

        if( local_start[seg] ){

            local_start[seg] = 0;
            markMultiples( *local_cPrime, seg );
            atomic_fetch_add( local_cycleDone, 1);
        }
    }
    #pragma clang diagnostic pop

    pthread_exit( NULL );
}

/// \brief searches a global arr of consecutive ints until an unflagged entry, and adds this entry to the prime array
/// \param pMax implicit, global: largest address of global prime array
/// \param iMax implicit, global: largest address of global int array
/// \param pCount implicit, global: index of currentPrime
/// \param status set to 1 when all integers have been enumerated
/// \param primes implicit. global array.
int seekPrime( int* status ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    //e.g. the check status of 5 is located at ints[4]
    int i = *(primes + pCount) - 1;

    //search for the first unflagged int in the bitarray
    //if testbits(ints,i) flags true, then i has already been found prime or marked composite
    while( testBit(ints, i) && i < in ){

        i++;
    }

    //if i > sqrt(in), then we've marked all composites < n
    if( i >= sqrt( (double)in) ){

        *status = 1;
        return 0;
    }

    //flag this prime
    setBit(ints, i);

    return i + 1;
}

/// \brief adds toAdd to primes
/// \param primes implicit. global array.
/// \param toAdd integer to add to (global) primes
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
/// \brief takes a bit array storing iMax values and sets every multiple of n to 1
/// \param n integer to mark multiples of
/// \param segment range of the ints array in which to mark primes
/// \param ints implicit, global: bitArray holding flags for each int [1, in]
void markMultiples ( int n, int segment ){

    int rangeLength, start, end, gap;

    //note that "start" refers to the array index, not the number itself
    //so it will always be 1 too low (e.g. start == 250 means the first checked number will be 251)
    //So we get the number value by adding 1, then calculate the first multiple of the given prime
    //in the segment this thread is handling using gap
    rangeLength = in / MAX_THREADS;
    start = segment * rangeLength;
    gap = n - ( ( start + 1 ) % n );

    if( n == gap ){

        gap = 0;
    }

    start += gap;

    // the segments are exactly equal. Since we can't guarantee that in will be divisible by MAX_THREADS, we assign
    //all segments an equal length except the last, which just runs up until the end of ints
    if( segment < MAX_THREADS - 1 ){

        end = start + rangeLength;

    } else {

        end = in;
    }

    for( int i = start; i < end; i += n ){

        setBit( ints, i);
    }
}

void debug(){

    /*for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);
    }*/

    printf( "There are %u primes <= %u\n", pCount + 1, in);
}