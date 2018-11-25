/******************************************************************************
* FILE: omp_mm.c
* DESCRIPTION:  
*   OpenMp Example - Matrix Multiply - C Version
*   Demonstrates a matrix multiply using OpenMP. Threads share row iterations
*   according to a predefined chunk size.
* AUTHOR: Blaise Barney
* LAST REVISED: 06/28/05
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

#define WARMUP_ITERS		1
#define MATV(m,i,j,col)		(m[i*col + j])

int main( int argc, char *argv[] ) 
{
	int	i, j, k, chunk, l;
	double total = 0.0;
	
	if (argc < 4) 
	{
		printf( "Usage: omp_mm <NRA> <NCA> <NCB>\n" );
		exit( -1 );
	}
	
	int NRA = atoi( argv[1] );
	int NCA = atoi( argv[2] );
	int NCB = atoi( argv[3] );
	
	double* a = malloc( NRA * NCA * sizeof(double) );	/* matrix A to be multiplied */
	double* b = malloc( NCA * NCB * sizeof(double) );	/* matrix B to be multiplied */
	double* c = malloc( NRA * NCB * sizeof(double) );	/* result matrix C */

	chunk = 16;                    /* set loop iteration chunk size */

	/*** Initialize matrices ***/
	//#pragma omp for schedule(dynamic, chunk) 
	for (i=0; i<NRA; i++)
		for (j=0; j<NCA; j++)
			MATV(a,i,j,NCA) = i + j;
	//#pragma omp for schedule(dynamic, chunk)
	for (i=0; i<NCA; i++)
		for (j=0; j<NCB; j++)
			MATV(b,i,j,NCB) = i * j;
	//#pragma omp for schedule(dynamic, chunk)
	for (i=0; i<NRA; i++)
		for (j=0; j<NCB; j++)
			MATV(c,i,j,NCB) = 0.0;
	
	// Warmup the cache
	for( l = 0; l < WARMUP_ITERS; ++l)
	{
		for(i=0; i<NRA; i++)
		{
			for(j=0; j<NCB; j++)
				for(k=0; k<NCA; k++)
					MATV(c,i,j,NCB) += MATV(a,i,k,NCA) * MATV(b,k,j,NCB);
		}
	}
	//----
	for (i=0; i<NRA; i++)
		for (j=0; j<NCB; j++)
			total += MATV(c,i,j,NCB);
	
	ftimer_init();
	
	double start = ftimer_msec();
	
	/*** Spawn a parallel region explicitly scoping all variables ***/
	#pragma omp parallel shared(a,b,c,chunk) private(i,j,k)
	{
		#pragma omp for schedule(dynamic, chunk)
		for (i=0; i<NRA; i++)    
		{
			for(j=0; j<NCB; j++)       
				for (k=0; k<NCA; k++)
					MATV(c,i,j,NCB) += MATV(a,i,k,NCA) * MATV(b,k,j,NCB);
		}
	}   /*** End of parallel region ***/

	double elapsed = ftimer_msec() - start;
	
	printf("exec time (ms): %f\n", elapsed);

	//---
	for (i=0; i<NRA; i++)
		for (j=0; j<NCB; j++)
			total += MATV(c,i,j,NCB);
	
	printf("..: %f\n", total);
	//---
	
	free( a );
	free( b );
	free( c );
	
	return 0;
}
