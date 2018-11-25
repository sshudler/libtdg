/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */


#include <unistd.h>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <iostream>

#include "x86_64-gcc-rdtsc.h"



static const char* CPU_FREQ_FILENAME = ".cpu_freq";
extern unsigned long long g_timerfreq;

bool sanity_check( bool print )
{
	HRT_TIMESTAMP_T t1, t2;
	uint64_t s, s2, s3;
	bool sanity = true;

	if(print) 
	{
		std::cout << "# Sanity check of the timer" << std::endl;
	}
	HRT_GET_TIMESTAMP(t1);
	sleep(1);
	HRT_GET_TIMESTAMP(t2);
	HRT_GET_ELAPSED_TICKS(t1, t2, &s)
	if(print) 
	{
		std::cout << "# sleep(1) needed " << s << " ticks" << std::endl;
	}
	HRT_GET_TIMESTAMP(t1);
	sleep(2);
	HRT_GET_TIMESTAMP(t2);
	HRT_GET_ELAPSED_TICKS(t1, t2, &s2)
	if(print) 
	{
		std::cout << "# sleep(2)/2 needed " << s2 / 2 << " ticks" << std::endl;
	}
	if (fabs((double)s - (double)s2/2) > (s*0.05)) 
	{
		sanity = false;
		std::cout << "# The high performance timer gives bogus results on this system!" << std::endl;
	}
	s = s2/2;
	HRT_GET_TIMESTAMP(t1);
	sleep(3);
	HRT_GET_TIMESTAMP(t2);
	HRT_GET_ELAPSED_TICKS(t1, t2, &s3)
	if(print) 
	{
		std::cout << "# sleep(3)/3 needed " << s3 / 3 << " ticks" << std::endl;
	}
	if (fabs((double)s - (double)s3/3) > (s*0.05)) 
	{
		sanity = false;
		std::cout << "# The high performance timer gives bogus results on this system!" << std::endl;
	}
	return sanity;
}


int main(int argc, char **argv) {
	
	bool sane = sanity_check( true );
	if( sane ) 
	{
		HRT_INIT( 1, g_timerfreq );
		std::ofstream freq_file;
		freq_file.open( CPU_FREQ_FILENAME );
		if( freq_file.is_open() ) 
		{
            freq_file << g_timerfreq << std::endl;
            freq_file.close();
        }
        exit (EXIT_SUCCESS);
	}
	exit (EXIT_FAILURE);
}
