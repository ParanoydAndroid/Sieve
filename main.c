#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <time.h>

int* ints;
int* primes;
int pCount = 0;
int done = 0;

void markMultiples ( int n, int max );
void seekPrime( int* pMax, int* iMax );
void debug( int  in );

int main( int argc, char* argv[] ){

    clock_t start = clock();

    long param = strtol( argv[1], NULL, 10 );

    if( param > INT_MAX ){

        printf( "Your chosen range is significantly too large.\nPlease try again" );
        exit( EXIT_FAILURE );
    }

    int in = (int)param;

    if( param < 3 ){

        //I dunno, do something about this

    }

    //create the base integer array
    ints = malloc( (int)param * sizeof(int) );

    //find best initial approximation for needed prime array size
    //since realloc is expensive
    double initialSize = (double) in / log(in);
    int pMax = (int)initialSize;
    primes = malloc( pMax * sizeof(int) );

    //manually add 2 to kickstart everything
    //TODO: refactor into something more efficient to mark all evens?
    primes[0] = 2;
    ints[1] = 1;
    markMultiples( 2, in);


    while( !done ){

        seekPrime( &pMax, &in );
        markMultiples( primes[pCount], in);
    }

    debug( in );
    clock_t stop = clock();
    double elapsed = (double) (stop - start) / CLOCKS_PER_SEC;
    printf("\nTime elapsed: %.5f\n", elapsed);

}

void seekPrime( int* pMax, int* iMax ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    int i = primes[pCount] - 1;
    while( ints[i] && i < *iMax ){

        i++;

    }

    if( i >= *iMax ){

        done = 1;
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