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
//#include "barrier.h"

#ifdef HAVE_PAPI
#include <papi.h>
#endif


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
	
	ompt_get_thread_data_t	g_get_thread_data_f;
	
	Graph*					g_tdg = NULL;
	Node*					g_finalNode = NULL;
	
	int						g_thread_cnt = 0;
	
#ifdef HAVE_PAPI
	unsigned int			g_papiNumEvents;
	int*					g_papiEvents = NULL;
#endif

	/*======================== Definitions ========================*/
	
	struct WorksharingData
	{
		WorksharingData() : _start_node( NULL ), _sink_node( NULL ), _last_chunk_node( NULL ) {}
		
		Node* _start_node;
		Node* _sink_node;
		Node* _last_chunk_node;
	};
	
	struct TaskData
	{
		TaskData() : _curr_task_node( NULL ), _curr_ws_data( NULL ), _curr_barrier_node( NULL ),
					 _sink_node( NULL ), _threadNum( 0 ) {}
		
		Node* 				_curr_task_node;
		WorksharingData*	_curr_ws_data;
		// The following two members are initialized from ParallelRegionData
		// to be able to access them even when the ParallelRegionData instance
		// is destroyed
		Node*				_curr_barrier_node;
		Node* 				_sink_node;
		
		int					_threadNum;
	};
	
	struct ParallelRegionData
	{
		ParallelRegionData() 
			: _parent_task_data( NULL ), _sink_node( NULL ), _team_size( 0 ), 
			  _barrier_cnt( 0 ), _curr_barrier_node( NULL ) {}
		
		TaskData* 			_parent_task_data;
		Node* 				_sink_node;
		unsigned int		_team_size;
		unsigned int		_barrier_cnt;
		Node*				_curr_barrier_node;
		std::mutex 			_barrier_mutex;
	};
	
	struct ThreadData
	{
		uint64_t			_loopCounter;
#ifdef HAVE_PAPI
		int					_papiEventset;
#endif
	};
	
	
	/*======================== Helper funcs ========================*/
	
	Node* create_clean_node( Node::NodeType type, bool gen_id )
	{
		int64_t nid = gen_id ? Node::nextId() : 0;
		Node* node = new Node( nid, type, 0 );
		g_tdg->addNode( nid, node );
		
		return node;
	}
	
	Node* create_new_node( Node::NodeType type, Node* parent_node, bool set_time = true )
	{
		Node* node = create_clean_node( type, parent_node != NULL );
		if( parent_node )
			Graph::connectNodes( parent_node, node );
		if( set_time )
			node->setLastTime( ftimer_msec() );
		return node;
	}
	
	
	/*======================== Callback funcs ========================*/

	void cb_thread_begin (
		ompt_thread_type_t thread_type,   /* type of thread               */
		ompt_data_t *thread_data          /* data of thread               */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK1("thread begin","type",thread_type);
#endif
	
		ThreadData* th_data = new ThreadData;
	
		th_data->_loopCounter = 0;
	
#ifdef HAVE_PAPI
		th_data->_papiEventset = PAPI_NULL;
	
		if( PAPI_create_eventset( &th_data->_papiEventset ) != PAPI_OK ) 
		{
			std::cerr << "libtdg: PAPI eventset creation error" << std::endl;
			exit( -1 );
		}
		for( int i = 0; i < libtdg::g_papiNumEvents; ++i )
		{
			if( PAPI_add_event( th_data->_papiEventset, libtdg::g_papiEvents[i] ) != PAPI_OK ) {
				char papi_event[1024];
				PAPI_event_code_to_name( libtdg::g_papiEvents[i], papi_event );
				std::cerr << "libtdg: adding PAPI counter " << papi_event << " failed" << std::endl;
				exit( -1 );
			}
		}
#endif
		
		thread_data->ptr = th_data;

	}

	void cb_thread_end (
		ompt_data_t *thread_data          /* data of thread               */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK("thread end");
#endif

		ThreadData* th_data = (ThreadData*)thread_data->ptr;

#ifdef HAVE_PAPI
		if( PAPI_cleanup_eventset( th_data->_papiEventset ) != PAPI_OK )
		{
			std::cerr << "libtdg: PAPI eventset cleanup error" << std::endl;
			exit( -1 );
		}
		if( PAPI_destroy_eventset( &th_data->_papiEventset ) != PAPI_OK )
		{
			std::cerr << "libtdg: PAPI eventset destruction error" << std::endl;
			exit( -1 );
		}
#endif

		delete th_data;
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
		TRACE_CALLBACK4("implicit task","endpoint",endpoint,"team size",team_size,"thread",thread_num,"task_data",task_data);
#endif
		
		if( endpoint == ompt_scope_begin )
		{
			ParallelRegionData* par_info = (ParallelRegionData*)parallel_data->ptr;
			TaskData* new_task_data = new TaskData;
			new_task_data->_curr_task_node = 
				create_new_node( Node::IMP_TASK, par_info->_parent_task_data->_curr_task_node );
			new_task_data->_sink_node = par_info->_sink_node;
			new_task_data->_threadNum = thread_num;
			task_data->ptr = new_task_data;
		}
		
		if( endpoint == ompt_scope_end )
		{
			TaskData* curr_task_data = (TaskData*)task_data->ptr;
			if( curr_task_data->_curr_barrier_node )
			{
				Graph::disconnectNodes( curr_task_data->_curr_barrier_node, curr_task_data->_curr_task_node );
				g_tdg->removeNode( curr_task_data->_curr_task_node->getId() );
				delete curr_task_data->_curr_task_node;
				
				if( thread_num == 0 )	// Only one thread should connect the last barrier to the sink node
				{
					Graph::connectNodes( curr_task_data->_curr_barrier_node, curr_task_data->_sink_node );
				}
			}
			else
			{
				curr_task_data->_curr_task_node->addTime( ftimer_msec() );
				Graph::connectNodes( curr_task_data->_curr_task_node, curr_task_data->_sink_node );
			}
			delete curr_task_data;
			task_data->ptr = NULL;
		}
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
		
		ParallelRegionData* par_info = new ParallelRegionData;
		par_info->_parent_task_data = (TaskData*)parent_task_data->ptr;
		par_info->_parent_task_data->_curr_task_node->addTime( ftimer_msec() );
		par_info->_sink_node = create_clean_node( Node::IMP_TASK, true );
		par_info->_team_size = requested_team_size;
		
		parallel_data->ptr = par_info;
	}

	void cb_parallel_end (
		ompt_data_t *parallel_data,        /* data of parallel region    */
		ompt_task_data_t *task_data,       /* data of task                 */
		ompt_invoker_t invoker,            /* who invokes master task?     */ 
		const void *codeptr_ra             
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK1("parallel end","parallel data",parallel_data->ptr);
#endif

		ParallelRegionData* par_info = (ParallelRegionData*)parallel_data->ptr;
		par_info->_parent_task_data->_curr_task_node = par_info->_sink_node;
		par_info->_sink_node->setLastTime( ftimer_msec() );
		g_finalNode = par_info->_sink_node;
		
		delete par_info;
		parallel_data->ptr = NULL;		
	}
	
	void cb_task_create (
		ompt_data_t *parent_task_data,          /* data of parent task         */
		const ompt_frame_t *parent_frame,       /* frame data for parent task   */
		ompt_data_t *new_task_data,             /* data of created task         */
		ompt_task_type_t type,
		int has_dependences,
		const void *codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("task create","type",type,"parent_task_data",(parent_task_data ? parent_task_data->ptr : 0),"new_task_data",new_task_data,"has_dependences",has_dependences);
#endif

		if( type == ompt_task_initial )
		{
			TaskData* task_data = new TaskData;
			task_data->_curr_task_node = create_new_node( Node::ROOT_TASK, NULL );
			new_task_data->ptr = task_data;
			g_finalNode = task_data->_curr_task_node;
		}
		if( type == ompt_task_explicit )
		{
			TaskData* parent_task = (TaskData*)parent_task_data->ptr;
			TaskData* task_data = new TaskData;
			task_data->_curr_task_node = 
				create_new_node( Node::EXP_TASK, parent_task ? parent_task->_curr_task_node : NULL );
			new_task_data->ptr = task_data;
		}
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

		TaskData* first_task = (TaskData*)first_task_data->ptr;
		TaskData* second_task = (TaskData*)second_task_data->ptr;
		
		if( first_task )
			first_task->_curr_task_node->addTime( ftimer_msec() );
		if( second_task )
			second_task->_curr_task_node->setLastTime( ftimer_msec() );
	}
	
	void cb_task_dependences (
		ompt_data_t *task_data,
		const ompt_task_dependence_t *deps,
		int ndeps
	)
	{
	}

	void cb_task_dependence (
		ompt_data_t *src_task_data,
		ompt_data_t *sink_task_data
	)
	{
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
		TRACE_CALLBACK3("worksharing","type",wstype,"endpoint",endpoint,"par data",parallel_data->ptr);
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
		TRACE_CALLBACK3("sync region","kind",kind,"endpoint",endpoint,"task data",task_data->ptr);
#endif

		TaskData* curr_task_data = (TaskData*)task_data->ptr;
		ParallelRegionData* par_info = (ParallelRegionData*)parallel_data->ptr;
		
		if( endpoint == ompt_scope_begin )
		{
			if( kind == ompt_sync_region_barrier )
			{
				if( par_info && par_info->_team_size > 1 )
				{
					curr_task_data->_curr_task_node->addTime( ftimer_msec() );
					Node* barrier_node = NULL;
					bool new_barrier = false;
					par_info->_barrier_mutex.lock();
					if( par_info->_barrier_cnt++ == 0 )		// first one to reach the barrier end
					{
						barrier_node = create_new_node( Node::BARRIER, curr_task_data->_curr_task_node );
						new_barrier = true;
						par_info->_curr_barrier_node = barrier_node;
					}
					else
					{
						barrier_node = par_info->_curr_barrier_node;
						Graph::connectNodes( curr_task_data->_curr_task_node, barrier_node );
						
						if( par_info->_barrier_cnt >= par_info->_team_size )	// all the threads reached the barrier
						{
							par_info->_barrier_cnt = 0;
						}
					}
					curr_task_data->_curr_barrier_node = barrier_node;
					par_info->_barrier_mutex.unlock();
					
					
					curr_task_data->_curr_task_node = create_new_node( Node::IMP_TASK, barrier_node );
					// If we want to measure the time each thread spent in the barrier we should add
					// here: curr_task_data->_curr_task_node->setLastTime( ftimer_msec() );
					// Otherwise, this line appears in the end of the barrier (endpoint == ompt_scope_end)
					
					if( new_barrier )
					{
						Graph::connectNodes( barrier_node, curr_task_data->_curr_task_node );
					}
				}
			}
		}
		if( endpoint == ompt_scope_end )
		{
			if( kind == ompt_sync_region_barrier )
			{
				if( par_info && par_info->_team_size > 1 )
				{
					curr_task_data->_curr_task_node->setLastTime( ftimer_msec() );	
				}
			}
		}
	}
	
	void cb_ext_loop (
		ext_loop_sched_t loop_sched,
		ompt_scope_endpoint_t endpoint,
		ompt_data_t * parallel_data,    /* The parallel region */
		ompt_data_t * task_data,        /* The implicit task of the worker */
		int64_t lower,                  /* Lower iteration bound */
		int64_t upper,                  /* Upper iteration bound */
		int64_t step,                   /* Increment */
		uint64_t chunk_size,            /* Chunk size */
		int64_t thread_lower,           /* Lower iter. bound for thread */
		const void * codeptr_ra
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("loop","lower",lower,"upper",upper,"chunk_size",chunk_size,"codeptr_ra",codeptr_ra);
#endif
		
		// codeptr_ra contains a code address that can be translated to source code filname and line
		// number with the 'addr2line' command
		// Use libunwind to get more precise address:
		// http://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
		
		TaskData* curr_task_data = (TaskData*)task_data->ptr;
		ompt_data_t* thread_data = (ompt_data_t*)g_get_thread_data_f();
		ThreadData* th_data = (ThreadData*)thread_data->ptr;
		
		if( endpoint == ompt_scope_begin )
		{
			curr_task_data->_curr_task_node->addTime( ftimer_msec() );
			
			WorksharingData* ws_data = new WorksharingData;
			ws_data->_start_node = create_new_node( Node::WS_TASK, curr_task_data->_curr_task_node );
			ws_data->_start_node->setLowerUpper( lower, upper );
			ws_data->_sink_node = create_clean_node( Node::IMP_TASK, true );
			
			curr_task_data->_curr_ws_data = ws_data;
		}
		
		if( endpoint == ompt_scope_end )
		{
			Node* last_chunk_node = curr_task_data->_curr_ws_data->_last_chunk_node;
			if( last_chunk_node )
			{
				last_chunk_node->endPapiCounters( th_data->_papiEventset );
				last_chunk_node->addTime( ftimer_msec() );
			}
			else
			{
				curr_task_data->_curr_ws_data->_start_node->addTime( ftimer_msec() );
				Graph::connectNodes( curr_task_data->_curr_ws_data->_start_node, 
									 curr_task_data->_curr_ws_data->_sink_node );
			}
			curr_task_data->_curr_task_node = curr_task_data->_curr_ws_data->_sink_node;
			curr_task_data->_curr_task_node->setLastTime( ftimer_msec() );
			
			delete curr_task_data->_curr_ws_data;
			curr_task_data->_curr_ws_data = NULL;
			
			++th_data->_loopCounter;	// Each thread increases the loop counter independently,
										// so the value should be the same for each thread
		}
	}
	
	void cb_ext_chunk (
		ompt_data_t *task_data,
		int64_t lower,
		int64_t upper,
		int last_chunk                  /* Is scheduled chunk last for thread? */
	)
	{
#ifdef LIBTDG_TRACE
		TRACE_CALLBACK4("chunk","lower",lower,"upper",upper,"task data",task_data->ptr,"last",last_chunk);
#endif

		TaskData* curr_task_data = (TaskData*)task_data->ptr;
		ompt_data_t* thread_data = (ompt_data_t*)g_get_thread_data_f();
		ThreadData* th_data = (ThreadData*)thread_data->ptr;
        
        if( curr_task_data->_curr_ws_data->_last_chunk_node ) 
        {
			if( !last_chunk )
			{
				Node* last_chunk_node = curr_task_data->_curr_ws_data->_last_chunk_node;
				last_chunk_node->endPapiCounters( th_data->_papiEventset );
				last_chunk_node->addTime( ftimer_msec() );
			}
		}
		else
		{
			curr_task_data->_curr_ws_data->_start_node->addTime( ftimer_msec() );
		}
		
		if( !last_chunk )
		{
			Node* chunk_node = create_new_node( Node::CHUNK_TASK, curr_task_data->_curr_ws_data->_start_node );
			chunk_node->setLowerUpper( lower, upper );
			chunk_node->setLoopCounter( th_data->_loopCounter );
			chunk_node->setThreadId( curr_task_data->_threadNum );
			curr_task_data->_curr_ws_data->_last_chunk_node = chunk_node;
			Graph::connectNodes( chunk_node, curr_task_data->_curr_ws_data->_sink_node );
			
			chunk_node->initPapiVals( libtdg::g_papiNumEvents );
			chunk_node->startPapiCounters( th_data->_papiEventset );
		}
	}
}
