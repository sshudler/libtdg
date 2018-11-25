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
#include <cstdlib>
#include <ompt.h>
#include <timer.h>
#include "callbacks.h"
#include "metrics.h"


#define INIT_CALLBACK(func)																						\
	if ( (*callback_set)( ompt_callback_##func, (ompt_callback_t)libtdg::cb_##func ) !=	ompt_set_always ) {		\
		std::cout << "libtdg: cb_##func set failed" << std::endl;	}											\

#define INIT_CALLBACK_EXT(func)																					\
	if ( (*callback_set)( ext_callback_##func, (ompt_callback_t)libtdg::cb_ext_##func ) != ompt_set_always ) {	\
		std::cout << "libtdg: cb_##func set failed" << std::endl;	}											\

/*========================= Init =========================*/


int init_libtdg( ompt_function_lookup_t lookup, ompt_fns_t* fns )
{
	std::cout << "libtdg (empty): initialize..." << std::endl;
	
	//ftimer::clock_init();
	
	libtdg::g_lookup = lookup;
	
	ompt_set_callback_t callback_set = (ompt_set_callback_t)(*lookup)( "ompt_set_callback" );
	if ( !callback_set )
	{
		std::cerr << "libtdg: callback_set from runtime is wrong" << std::endl;
		exit( -1 );
	}
	
	INIT_CALLBACK(thread_begin);
	INIT_CALLBACK(thread_end);
	INIT_CALLBACK(parallel_begin);
	INIT_CALLBACK(parallel_end);
	INIT_CALLBACK(implicit_task);
	INIT_CALLBACK(work);
	INIT_CALLBACK(task_schedule);
	INIT_CALLBACK(task_create);
	INIT_CALLBACK(task_dependences);
	INIT_CALLBACK(task_dependence);
	INIT_CALLBACK(sync_region);
	
	// Extensions:
	INIT_CALLBACK_EXT(loop);
	INIT_CALLBACK_EXT(chunk);
	
	return 1;
}

void finalize_libtdg( ompt_fns_t* fns )
{
#ifdef LIBTDG_TRACE
	std::cout << "libtdg: finalize..." << std::endl;
#endif
}

extern "C" ompt_fns_t* ompt_start_tool( unsigned int omp_version, 
										const char * runtime_version )
{
	libtdg::g_fns.initialize = init_libtdg;
	libtdg::g_fns.finalize = finalize_libtdg;
	
	return &libtdg::g_fns;
}

