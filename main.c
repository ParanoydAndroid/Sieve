#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

//One line macro version from:
//http://www.mathcs.emory.edu/~cheung/Courses/255/Syllabus/1-C-intro/bit-array.html
#define setBit(arr, int) ( arr[int/32] |=  1 << (int % 32) )
#define clearBit(arr, int) ( arr[int/32] &= ~(1 << (int % 32)) )
#define testBit(arr, int) ( arr[int/32] & (1 << (k%32)) )

//total array of all numbers <=n
int* ints;

//result set
int* primes;

//max index of primes with a current value
//use: primes[pCount]
//Make local to main?
int pCount = 0;


void markMultiples ( int n, int max );
void seekPrime( int* pMax, const int* iMax, int* status );
void debug( int  in );

int main( int argc, char* argv[] ){

    //testing my branch
    clock_t start = clock();

    int done = 0;
    long param = strtol( argv[1], NULL, 10 );

    if( param > INT_MAX ){

        printf( "Your chosen range is significantly too large.\nPlease try again" );
        exit( EXIT_FAILURE );
    }

    const int in = (int)param;

    if( param < 3 ){

        //I dunno, do something about this

    }

    //create the base integer array
    ints = malloc( (int)param * sizeof(int) );

    //find best initial approximation for needed prime array size
    //since realloc is expensive
    double initialSize = (double) in / log(in);

    //max addressable location of primes, when exceeded, we realloc by a factor of 2
    int pMax = (int)initialSize;
    primes = malloc( pMax * sizeof(int) );

    //manually add 2 to kickstart everything
    //TODO: refactor into something more efficient to mark all evens?
    primes[0] = 2;
    ints[1] = 1;
    markMultiples( 2, in);


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

void seekPrime( int* pMax, const int* iMax, int* status ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    //e.g. the check status of 5 is located at ints[4]
    int i = primes[pCount] - 1;
    while( ints[i] && i < *iMax ){

        i++;

    }

    if( i >= *iMax ){

        //we have exhausted our integer array and checked for all prime candidates
        *status = 1;
        return;

    }

    //process this prime then pass
    ints[i] = 1;
    pCount++;

    if( pCount >= *pMax ){

        *pMax *= 2;
        primes = realloc( primes, *pMax * sizeof(int) );

    }

    primes[pCount] = i + 1;
}

void markMultiples ( int n, int max ){

    //0 indexed so the mark for n is at n-1
    int location = n - 1;

    //we'll assume marking of n will be handled in the calling code as it loops
    while ( location + n < max ){

        location += n;
        *(ints + location) = 1;

    }
}

void debug( int in ){

    for( int i = 0; i < in; i++ ){

        printf( "%d: %d ", i+1, ints[i]);

    }

    puts("");
    for( int i = 0; i < pCount + 1; i++ ){

        printf( "%d, ", primes[i]);

    }


}