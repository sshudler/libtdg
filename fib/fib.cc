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

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <omp.h>


#include "timer.h"

#define ITERS	1000000


static bool g_print_debug = false;


double do_work(int i)
{
	int tid = omp_get_thread_num( );
	double total = 5;
	for( int j = 0; j < ITERS; ++j ) total += total/ITERS;

	std::cout << i << " tid = " << tid << ", res = " << total << std::endl;
	
	return total;
}

unsigned int fib (unsigned int n) {
           
	unsigned int result;
  
	//do_work( n );
	
	if (n < 2) 
	{
		result = n;
	}
	else 
	{
		unsigned int prev_num, prev_prev_num;
		#pragma omp task shared(prev_num)
		{
			prev_num = fib(n - 1);
		}
		#pragma omp task shared(prev_prev_num)
		{
			prev_prev_num = fib(n - 2);
		}
		#pragma omp taskwait

		result = prev_prev_num + prev_num;
	}

	return result;
}

int main (int argc, char** argv) 
{	
	if (argc < 3) 
	{
		std::cerr << "Usage: fib <n> <debug info> [nop loops]" << std::endl;
		exit (1);
	}
	
	unsigned int n = (unsigned int) atoi (argv[1]);
	g_print_debug = (atoi (argv[2]) == 1);

	if (g_print_debug)
		std::cout << "N = " << n << std::endl;
  
	unsigned int result;
	double start_time, total_time;
    
	ftimer_init();

	start_time = ftimer_msec();

	#pragma omp parallel
	{
		#pragma omp single nowait
		#pragma omp task shared(result)
		{
			result = fib(n);
		}
	}
	
	total_time = ftimer_msec() - start_time;
  
	std::cout << "result: " << result << std::endl;
	std::cout << "exec time (ms): " << total_time << std::endl;
    
	return 0;
}
