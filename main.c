#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

//One line macro version from:
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define setBit(arr, int) ( (arr)[(int)/32] |=  1 << ((int) % 32) )
#define testBit(arr, int) ( (arr)[(int)/32] & (1 << ((int) % 32)) )

//total array of all numbers <=n
unsigned int* ints;

//result set
int* primes;

//max index of primes with a current value
//use: primes[pCount]
//Make local to main?
int pCount = 0;


void markMultiples ( int n, int iMax );
void seekPrime( int* pMax, const int* iMax, int* status );
void debug( int  in );

int main( int argc, char* argv[] ){

    //performance testing
    clock_t start = clock();

    int done = 0;
    long param = strtol( argv[1], NULL, 10 );

    if( param > UINT_MAX ){

        printf( "Your chosen range is significantly too large.\nPlease try again" );
        exit( EXIT_FAILURE );
    }

    const unsigned in = (unsigned int)param;

    //properly size the base bitarray for 'in' integers
    int iMax = (in / 32) + 1;
    ints = malloc( iMax * sizeof(unsigned int) );

    //find best initial approximation for needed prime array size
    //since realloc is expensive.  Uses the PNT
    double initialSize = (double) in / log(in);

    //max addressable location of primes, when exceeded, we realloc by a factor of 2
    int pMax = (int)initialSize;
    primes = malloc( pMax * sizeof(int) );

    //manually add 2 to kickstart everything
    primes[0] = 2;


   for( int i = 0; i < iMax; i++ ){

       //this is the equivalent of flagging every representative of an even number in the bitset
       //instead of manually enumerating each bit with
       //markMultiples( 2, in );
        ints[i] = 2863311530;

    }

    //TODO: my runner function for 8 threads?
    while( !done ){

        seekPrime( &pMax, &in, &done);
        markMultiples( primes[pCount], in);
    }

    debug( in );

    free( ints );
    free ( primes );
    clock_t stop = clock();

    double elapsed = (double) (stop - start) / CLOCKS_PER_SEC;
    printf("\nTime elapsed: %.5f\n", elapsed);

}

///
/// \param pMax largest address of global prime array
/// \param iMax largest address of global int arary
/// \param status set to 1 when all integers have been enumerated
void seekPrime( int* pMax, const int* iMax, int* status ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    //e.g. the check status of 5 is located at ints[4]
    int i = primes[pCount] - 1;

    //if testbits(ints,i) flags true, then this number has already been processed
    //and either found prime or marked composite
    while( testBit(ints, i) && i < *iMax ){

        i++;

    }

    if( i >= *iMax ){

        //we have exhausted our integer array and checked for all prime candidates
        *status = 1;
        return;

    }

    //flag this prime then pass it to the primes array.
    setBit(ints, i);
    pCount++;

    if( pCount >= *pMax ){

        *pMax *= 2;
        primes = realloc( primes, *pMax * sizeof(int) );

    }

    primes[pCount] = i + 1;
}

/// \details takes a bit array storing iMax values and sets every multiple of n to 1
/// \param n integer to mark multiples of
/// \param iMax maximum multiple to mark
void markMultiples ( int n, int iMax ){


    //0 indexed so the mark for n is at n-1
    int location = n - 1;

    //we'll assume marking of n will be handled in the calling code as it loops
    while ( location + n < iMax ){

        location += n;
        setBit( ints, location );

    }
}

void debug( int in ){

    for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);

    }

}