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


#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#include "x86_64-gcc-rdtsc.h"



static const char* CPU_FREQ_FILENAME = ".cpu_freq";

static void init_clock_time() 
{ 
        //sanity_check (true);
        char buff[128];
        FILE* cpufreq_file = fopen( CPU_FREQ_FILENAME, "r" );
        if (cpufreq_file) 
        {
                if( fgets( buff, 128, cpufreq_file ) ) 
                {
                        g_timerfreq = (uint64_t)strtod( buff, NULL );
                }
                else 
                {
                        exit (-1);
                }
                fclose( cpufreq_file );
        }
        else 
        {
                HRT_INIT( 1, g_timerfreq );
        }
        printf( "Timer: cycles per sec = %llu\n", g_timerfreq );
}
  
static double get_clock_time() { 
        HRT_TIMESTAMP_T t;
        uint64_t ticks;
        HRT_GET_TIMESTAMP( t );
        HRT_GET_TIME( t, ticks );
        return HRT_GET_MSEC( ticks );
}

void 	ftimer_init() { init_clock_time(); }
double 	ftimer_msec() { return get_clock_time(); }

