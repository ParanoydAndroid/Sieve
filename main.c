#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

//TODO: add malloc and realloc error handling

//One line macro version from:
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define setBit(arr, int) ( (arr)[(int)/32] |=  1 << ((int) % 32) )
#define testBit(arr, int) ( (arr)[(int)/32] & (1 << ((int) % 32)) )

//threads instantiation requires a void func( void* param), so we use a void pointer to a struct
//to pass the parameters
struct threadArgs{

    int* pMax;
    const int* in;
    int* done;
};

const unsigned int MAX_THREADS = 4;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//total array of all numbers <=n
unsigned int* ints;

//result set
int* primes;

//max index of primes with a current value
int pCount = 0;

void markMultiples ( int n, int iMax );
void seekPrime( int* pMax, const int* iMax, int* status );
void debug();
void* sieve_runner( void* args );

int main( int argc, char* argv[] ){

    //performance testing
    clock_t start = clock();

    //Base thread config
    pthread_t tids[MAX_THREADS];
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    //

    //sentinel to determine when int array has been exhausted
    int done = 0;

    long param = strtol( argv[1], NULL, 10 );

    if( param > UINT_MAX ){

        printf( "Your chosen range is significantly too large.\nPlease try again" );
        exit( EXIT_FAILURE );
    }

    const int in = (int)param;

    //properly size the base bitarray for 'in' integers, where we store 32 flags per int array bucket
    int iMax = (in / 32) + 1;
    ints = malloc( iMax * sizeof(unsigned int) );

    //find best initial approximation for needed prime array size
    //since realloc is expensive.  Uses the PNT
    double initialSize = (double) in / log(in);
    int pMax = (int)initialSize;

    primes = malloc( pMax * sizeof(int) );

    if( primes == NULL){

        perror( "Could not initially allocate sufficient space in memory" );
        exit(EXIT_FAILURE);
    }

    //instantiate struct because pthread_create can only accept a single pointer to args for runner function.
    //because the struct only holds pointers, all threads can be passed the same struct
    struct threadArgs args;
    args.done = &done;
    args.in = &in;
    args.pMax = &pMax;

    //manually add 2 to prime array kickstart everything
    primes[0] = 2;

    for( int i = 0; i < iMax; i++ ){

       //this is the equivalent of flagging every representative of an even number in the bitset
       //instead of manually enumerating each bit with markMultiples( 2, in );
       ints[i] = 2863311530;
    }

    for( int i = 0; i < MAX_THREADS; i++ ){

        if( pthread_create( tids + i, &attr, sieve_runner, &args ) ){

            perror( "Error creating thread.  Attempting to continue" );
        }
    }

    for( int j = 0; j < MAX_THREADS; j++ ){

        pthread_join( tids[j], NULL );
    }

    debug();

    pthread_mutex_destroy( &mutex );
    clock_t stop = clock();

    double elapsed = (double) (stop - start) / CLOCKS_PER_SEC;
    printf("\nTime elapsed: %.5f\n", elapsed);

    free( ints );
    free ( primes );
}

void* sieve_runner( void* args ){

    struct threadArgs* localPtrs = (struct threadArgs*)args;

    int* local_pMax = localPtrs -> pMax;
    const int* local_in = localPtrs -> in;
    int* local_done = localPtrs -> done;

    //creating threads isn't super expensive, but isn't super cheap either, so we create few threads
    //and loop the existing threads until the job is done, instead of creating many threads which each execute
    //a single pass of the loop.
    while ( !(*local_done) ){

        seekPrime( local_pMax, local_in, local_done );
        markMultiples( primes[pCount], *local_in );
    }

    //pthread_exit( NULL );
}

/// \details searches a global arr of consecutive ints until an unflagged entry, and adds this entry to the prime array
/// \param pMax largest address of global prime array
/// \param iMax largest address of global int array
/// \param status set to 1 when all integers have been enumerated
void seekPrime( int* pMax, const int* iMax, int* status ){

    pthread_mutex_lock( &mutex );

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    //e.g. the check status of 5 is located at ints[4]
    int i = *(primes + pCount) - 1;

    //if testbits(ints,i) flags true, then this number has already been processed
    //and either found prime or marked composite
    while( testBit(ints, i) && i < *iMax ){

        i++;
    }

    if( i >= *iMax ){

        //we have exhausted our integer array and checked for all prime candidates
        *status = 1;
        pthread_mutex_unlock( &mutex );
        return;
    }

    //flag this prime then pass it to the primes array.
    setBit(ints, i);
    pCount++;

    if( pCount >= *pMax ){

        *pMax *= 2;
        primes = realloc( primes, *pMax * sizeof(int) );

        if( primes == NULL ){

            perror("could not allocate sufficient space.  Try again with a smaller parameter");
            exit(EXIT_FAILURE);
        }
    }

    primes[pCount] = i + 1;
    pthread_mutex_unlock( &mutex );
}

/// \details takes a bit array storing iMax values and sets every multiple of n to 1
/// \param n integer to mark multiples of
/// \param iMax maximum multiple to mark
void markMultiples ( int n, const int iMax ){

    //0 indexed so the mark for n is at n-1
    int location = n - 1;

    //Since even multiples of odd numbers are even, we only need to check the odd multiples
    //we do that by jumping 2n bits instead of n bits when checking multiples
    n *= 2;

    while ( location + n < iMax ){

        location += n;
        setBit( ints, location );
    }
}

void debug(){

    for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);
    }

    printf( "\n pCount final value: %d", pCount);
}