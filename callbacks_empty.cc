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
#include <mutex>
#include <timer.h>
#include "callbacks.h"
#include "graph.h"


#define TRACE_CALLBACK(cb)											\
	std::cout << "libtdg: " << cb << std::endl;						\

#define TRACE_CALLBACK1(cb,argna,arga)								\
	std::cout << "libtdg: " << cb << ", "							\
			  << argna << " " << arga << " "						\
			  << std::endl;											\

#define TRACE_CALLBACK2(cb,argna,arga,argnb,argb)					\
	std::cout << "libtdg: " << cb << ", "							\
			  << argna << " " << arga << " "						\
			  << argnb << " " << argb << " "						\
			  << std::endl;											\
			  
#define TRACE_CALLBACK3(cb,argna,arga,argnb,argb,argnc,argc)		\
	std::cout << "libtdg: " << cb << ", "							\
			  << argna << " " << arga << " "						\
			  << argnb << " " << argb << " "						\
			  << argnc << " " << argc << " "						\
			  << std::endl;											\
			  
#define TRACE_CALLBACK4(cb,argna,arga,argnb,argb,argnc,argc,argnd,argd)		\
	std::cout << "libtdg: " << cb << ", "									\
			  << argna << " " << arga << " "								\
			  << argnb << " " << argb << " "								\
			  << argnc << " " << argc << " "								\
			  << argnd << " " << argd << " "								\
			  << std::endl;													\
			  

namespace libtdg
{
	/*======================== Globals ========================*/
	
	ompt_fns_t 				g_fns;
	ompt_function_lookup_t	g_lookup;
	

	/*======================== Callback funcs ========================*/

	void cb_thread_begin (
		ompt_thread_type_t thread_type,   /* type of thread               */
		ompt_data_t *thread_data          /* data of thread               */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK1("thread begin","type",thread_type);
#endif
	}

	void cb_thread_end (
		ompt_data_t *thread_data          /* data of thread               */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK("thread end");
#endif
	}
	
	void cb_implicit_task (
		ompt_scope_endpoint_t endpoint,
		ompt_data_t *parallel_data,
		ompt_data_t *task_data,
		unsigned int team_size,
		unsigned int thread_num
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK3("implicit task","endpoint",endpoint,"team size",team_size,"thread", thread_num);
#endif
	}
		
	void cb_parallel_begin (
		ompt_data_t *parent_task_data,    /* data of parent task         */
		const ompt_frame_t *parent_frame, /* frame data of parent task    */
		ompt_data_t *parallel_data,       /* data of parallel region   */
		unsigned int requested_team_size, /* requested number of threads in team    */
		ompt_invoker_t invoker,           /* who invokes master task?     */
		const void *codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK2("parallel begin","parent",parent_task_data->value,"team size",requested_team_size);
#endif
	}

	void cb_parallel_end (
		ompt_data_t *parallel_data,        /* data of parallel region    */
		ompt_task_data_t *task_data,       /* data of task                 */
		ompt_invoker_t invoker,            /* who invokes master task?     */ 
		const void *codeptr_ra             
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK1("parallel end","parallel data",parallel_data->value);
#endif
	}
	
	void cb_task_create (
		ompt_data_t *parent_task_data,    /* data of parent task         */
		const ompt_frame_t *parent_frame,       /* frame data for parent task   */
		ompt_data_t *new_task_data,       /* data of created task         */
		ompt_task_type_t type,
		int has_dependences,
		const void *codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("task create","type",type,"parent_task_data",(parent_task_data ? parent_task_data->ptr : 0),"new_task_data",new_task_data,"codeptr_ra",codeptr_ra);
#endif
	}

	void cb_task_schedule (
		ompt_task_data_t *first_task_data,
		ompt_task_status_t prior_task_status,
		ompt_task_data_t *second_task_data
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK3("task schedule","first_task_data",first_task_data,"prior_task_status",prior_task_status,"second_task_data",second_task_data);
#endif
	}
	
	void cb_task_dependences (
		ompt_data_t *task_data,
		const ompt_task_dependence_t *deps,
		int ndeps
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK1("task dependences","task_data",task_data);
#endif

		std::cout << "Ndeps = " << ndeps << std::endl;
		for( int i = 0; i < ndeps; ++i ) {
			std::cout << "Addr = " << (uint64_t)deps[i].variable_addr << ", flags = " << deps[i].dependence_flags << std::endl;
		}
	}

	void cb_task_dependence (
		ompt_data_t *src_task_data,
		ompt_data_t *sink_task_data
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK2("task dependence","src_task_data",src_task_data,"sink_task_data",sink_task_data);
#endif
	}
	
	void cb_work (
		ompt_work_type_t wstype,
		ompt_scope_endpoint_t endpoint,
		ompt_data_t *parallel_data,
		ompt_data_t *task_data,
		uint64_t count,
		const void *codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK3("worksharing","type",wstype,"endpoint",endpoint,"par data",parallel_data->value);
#endif
	}

	void cb_sync_region (
		ompt_sync_region_kind_t kind,
		ompt_scope_endpoint_t endpoint,
		ompt_data_t *parallel_data,
		ompt_data_t *task_data,
		const void *codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK3("sync region","kind",kind,"endpoint",endpoint,"task data",task_data->value);
#endif
	}
	
	void cb_ext_loop (
		ext_loop_sched_t loop_sched,
		ompt_scope_endpoint_t endpoint,
		ompt_data_t * parallel_data,	/* The parallel region */
		ompt_data_t * task_data,		/* The implicit task of the worker */
		int64_t lower,					/* Lower iteration bound */
		int64_t upper,					/* Upper iteration bound */
		int64_t step,					/* Increment */
		uint64_t chunk_size,			/* Chunk size */
		int64_t thread_lower,			/* Lower iter. bound for thread */
		const void * codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("loop","lower",lower,"upper",upper,"chunk_size",chunk_size,"endpoint",endpoint);
#endif
	}
	
	void cb_ext_chunk (
		ompt_data_t *task_data,
		int64_t lower,
		int64_t upper,
		int last_chunk					/* Is scheduled chunk last for thread? */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("chunk","lower",lower,"upper",upper,"task data",task_data->ptr,"last",last_chunk);
#endif
	}
}
