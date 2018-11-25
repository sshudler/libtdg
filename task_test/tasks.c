// Copyright (c) 2018 Sergei Shudler
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <math.h>
#include <omp.h>
//#include "omp_testsuite.h"
//#include "omp_my_sleep.h"

//#define NUM_TASKS     10

int main( int argc, char** argv )
{
//  int tids[NUM_TASKS];
//  int i;
  int x, y;

  #pragma omp parallel
  {
    #pragma omp single
    {
        //int myi;
        //myi = i;
        #pragma omp task depend(in:x) depend(out:y) 
        {
	  printf( "Task 2\n" );
          //my_sleep (SLEEPTIME);
          //tids[myi] = omp_get_thread_num();
        } /* end of omp task */

        #pragma omp task depend(out:x)
        {
	  printf( "Task 1\n" );
        }

        #pragma omp task depend(in:y)
        {
	  printf( "Task 3\n" );
        }

    } /* end of single */
  } /*end of parallel */

  return 0;
} /* end of check_parallel_for_private */
