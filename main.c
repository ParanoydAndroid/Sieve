#include <stdio.h>
#include <Pthread.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

int* ints;
int* primes;
int pCount = 0;

void markMultiples ( int n, int max );
void seekPrime( int* max );
void debug();

int main( int argc, char* argv[] ){
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

    primes[0] = 2;
    seekPrime( &pMax );

    markMultiples( primes[pCount], in);

    debug( in );

}

void seekPrime( int* max ){

    //since ints is 0 indexed, the matching bit in ints == numValue - 1
    int i = primes[pCount] - 1;
    while( !ints[i] ){

        i++;

    }

    //process this prime then pass
    ints[i] = 1;
    pCount++;

    if( pCount >= *max ){

        *max *= 2;
        primes = realloc( primes, *max * sizeof(int) );

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

        printf( "&d: %d", i, ints[i])

    }


}