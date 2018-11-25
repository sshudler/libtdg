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
#include <string>
#include <algorithm>
#include <cstdlib>
#include <vector>
#include <timer.h>
#include <pthread.h>
#include <ompt.h>
#include "callbacks.h"
#include "metrics.h"

#ifdef HAVE_PAPI
#include <papi.h>
#endif


libtdg::Metric** g_metrics = NULL;


#define INIT_CALLBACK(func)																						\
	if ( (*callback_set)( ompt_callback_##func, (ompt_callback_t)libtdg::cb_##func ) !=	ompt_set_always ) {		\
		std::cout << "libtdg: cb_##func set failed" << std::endl;	}											\

#define INIT_CALLBACK_EXT(func)																					\
	if ( (*callback_set)( ext_callback_##func, (ompt_callback_t)libtdg::cb_ext_##func ) != ompt_set_always ) {	\
		std::cout << "libtdg: cb_##func set failed" << std::endl;	}											\


/*========================= Aux =========================*/


static void parse_tokens( const char* env_var, std::vector<std::string>& tokens_v )
{
	if( env_var )
	{
		unsigned int num_tokens = 0;
		std::string env_str = env_var;
		if( env_str.size() > 0 )
		{
			num_tokens = std::count( env_str.begin(), env_str.end(), ',' ) + 1;
		}

		size_t last_pos = 0;
		for( int i = 0; i < num_tokens; ++i )
		{
			size_t curr_pos = env_str.find( ',', last_pos );
			std::string token = 
				env_str.substr( last_pos, (curr_pos == std::string::npos) ? std::string::npos : (curr_pos - last_pos) );
		
			last_pos = curr_pos + 1;
			
			tokens_v.push_back( token );
		}
	}
}


/*========================= Init =========================*/

void init_papi_events()
{
#ifdef HAVE_PAPI
	if( PAPI_library_init( PAPI_VER_CURRENT ) != PAPI_VER_CURRENT ) 
	{
		std::cerr << "libtdg: PAPI initialization error" << std::endl;
		exit( -1 );
	}
	
	if( PAPI_thread_init( pthread_self ) != PAPI_OK )
	{
		std::cerr << "libtdg: PAPI thread_init error" << std::endl;
		exit(1);
	}
  
	// Check if combination works "papi_event_chooser PRESET event1 event2 ... eventi"
	const char* papi_metrics_env = std::getenv( "TDG_PAPI_METRICS" );
	std::vector<std::string> tokens_v;
	
	parse_tokens( papi_metrics_env, tokens_v );
	libtdg::g_papiNumEvents = tokens_v.size();

#ifdef LIBTDG_TRACE
	std::cout << "libtdg: TDG_PAPI_METRICS = " << (papi_metrics_env ? papi_metrics_env : "") << std::endl;
	std::cout << "libtdg: num papi metrics: " << libtdg::g_papiNumEvents << std::endl;
#endif

	if( libtdg::g_papiNumEvents > 0 )
	{
		libtdg::g_papiEvents = new int[libtdg::g_papiNumEvents];
		for( int i = 0; i < libtdg::g_papiNumEvents; ++i )
		{
			std::string& papi_event = tokens_v[i];
			if( PAPI_event_name_to_code( (char*)papi_event.c_str(), &libtdg::g_papiEvents[i] ) != PAPI_OK )
			{
				std::cerr << "libtdg: PAPI event " << papi_event << " error" << std::endl;
				exit( -1 );
			}
		}
	}
#endif
}

int init_libtdg( ompt_function_lookup_t lookup, ompt_fns_t* fns )
{
	std::cout << "libtdg: initialize..." << std::endl;
	
	ftimer_init();
	
	libtdg::g_tdg = new libtdg::Graph();
	libtdg::g_lookup = lookup;
	
	ompt_set_callback_t callback_set = (ompt_set_callback_t)(*lookup)( "ompt_set_callback" );
	if( !callback_set )
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
	
	init_papi_events();
	
	// Lookup additional functions:
	libtdg::g_get_thread_data_f = (ompt_get_thread_data_t)(*lookup)( "ompt_get_thread_data" );
	if( !libtdg::g_get_thread_data_f )
	{
		std::cerr << "libtdg: ompt_get_thread_data from runtime is wrong" << std::endl;
		exit( -1 );
	}
	
	return 1;
}

void finalize_libtdg( ompt_fns_t* fns )
{
#ifdef LIBTDG_TRACE
	std::cout << "libtdg: finalize..." << std::endl;
#endif
	
	if( libtdg::g_finalNode )
		libtdg::g_finalNode->addTime( ftimer_msec() );
	
	// Possible metrics: tim,cri,dot
	const char* metrics_env = std::getenv( "TDG_TOOL_METRICS" );
	unsigned int num_metrics = 0;
	std::vector<std::string> tokens_v;
	
	parse_tokens( metrics_env, tokens_v );
	num_metrics = tokens_v.size();
	
#ifdef LIBTDG_TRACE
	std::cout << "libtdg: TDG_TOOL_METRICS = " << (metrics_env ? metrics_env : "") << std::endl;
	std::cout << "libtdg: num tool metrics: " << num_metrics << std::endl;
#endif

	if( num_metrics )
	{
		g_metrics = new libtdg::Metric*[num_metrics];
		for( int i = 0; i < num_metrics; ++i )
		{
			std::string& token = tokens_v[i];
			
			if( token == "tim" )
			{
				g_metrics[i] = new libtdg::TotalTimeMetric( );
			}
			if( token == "cri" )
			{
				g_metrics[i] = new libtdg::CriticalPathMetric( );
			}
			if( token == "dot" )
			{
				g_metrics[i] = new libtdg::SimpleDotFileMetric( "tdg.dot" );
			}
			if( token == "log" )
			{
				g_metrics[i] = new libtdg::LogFileMetric( "chunks.log" );
			}
		}
	}
			
	for( unsigned int i = 0; i < num_metrics; ++i )
		g_metrics[i]->init( libtdg::g_tdg );
		
	for( unsigned int i = 0; i < num_metrics; ++i )
		g_metrics[i]->printMetric( std::cout );
		
	for( unsigned int i = 0; i < num_metrics; ++i )
		delete g_metrics[i];

	delete[] libtdg::g_papiEvents;
	delete libtdg::g_tdg;
}

extern "C" ompt_fns_t* ompt_start_tool( unsigned int omp_version, 
										const char * runtime_version )
{
	libtdg::g_fns.initialize = init_libtdg;
	libtdg::g_fns.finalize = finalize_libtdg;
	
	return &libtdg::g_fns;
}

