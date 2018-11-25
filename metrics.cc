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

#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "metrics.h"


using namespace libtdg;


extern void compute_stats( std::vector<double>& arr, double& total, double& avg, double& stddev, double& median );


//===================== CriticalPathMetric =============================

double CriticalPathMetric::getMetric( ) 
{
	if (_critical_path_time_len < 0)
		computeCriticalPath ();
			
	return _critical_path_time_len;
}


void CriticalPathMetric::printMetric( std::ostream& out_stream )
{
	getMetric( );	// Ensure that the critical path is computed
	
	out_stream << "Longest (critical) path (time ms): " << _critical_path_time_len << std::endl;
	out_stream << "Longest (critical) path (length): " << _critical_path_len << std::endl;
}


void CriticalPathMetric::computeCriticalPath( )
{
	_critical_path_time_len = 0.0;
	_critical_path_len = 0;
	std::list<Node*> topo_list;
	_tdg->topoSort( topo_list );
	std::cerr << "CriticalPathMetric - finished topoSort" << std::endl;
        
	/***
	std::cerr << "CriticalPathMetric - topoSort list:" << std::endl;
	for (std::list<Node*>::iterator l_iter = topo_list.begin ();
		l_iter != topo_list.end (); l_iter++) {
		Node* curr_node = *l_iter;
		if (curr_node->is_taskwait () || curr_node->is_barrier ())
			continue;
		std::cerr << curr_node->get_wd_id() << "   ";
	}
	std::cerr << std::endl;
	***/
		
	Node* last_critical_node = NULL;
      
	//std::cerr << "CriticalPathMetric - scanning nodes:" << std::endl;
	for (std::list<Node*>::iterator l_iter = topo_list.begin ();
		l_iter != topo_list.end (); l_iter++) {
		Node* curr_node = *l_iter;
		//if (curr_node->ignore_node_for_metrics ())
		//	continue;
		double max_curr_path_time = curr_node->getTotalTime();
		int max_curr_path_len = 0;
		Node::AdjacencyIterator adj_iter = 
			Node::AdjacencyIterator::beginAdjIter (curr_node, true);     // Iterate over entry edges
		Node::AdjacencyIterator adj_iter_end = 
			Node::AdjacencyIterator::endAdjIter (curr_node, true);   // Iterate over entry edges
		while (adj_iter != adj_iter_end) {
			Node* source_node = *adj_iter;
			int path_inc = source_node->getPathLength() + 1;
			if( path_inc > max_curr_path_len ) 
			{
				max_curr_path_len = path_inc;
				//curr_node->_prev_critical = source_node;
			}
			double time_inc = source_node->getPathTime() + curr_node->getTotalTime();
			if( time_inc > max_curr_path_time ) 
			{
				max_curr_path_time = time_inc;
				curr_node->setPrevCritical( source_node );
			}
			++adj_iter;
		}
		curr_node->setPathLength( max_curr_path_len );
		curr_node->setPathTime( max_curr_path_time );
			
		//std::cerr << "Curr node: " << curr_node->get_wd_id () << ", path length: " << curr_node->_path_length << std::endl;
		//std::cerr << max_curr_path_len << "   " << max_curr_path_time << std::endl;
		
		if (max_curr_path_len > _critical_path_len) {
			_critical_path_len = max_curr_path_len;
			//last_critical_node = curr_node;
		}
		if (max_curr_path_time > _critical_path_time_len) {
			_critical_path_time_len = max_curr_path_time;
			last_critical_node = curr_node;
		}
	}
	
	std::cerr << "CriticalPathMetric - marking critical nodes" << std::endl;
	double total_time_on_criticial_path = 0.0;
	while (last_critical_node) {
		last_critical_node->setIsCritical( true );
		total_time_on_criticial_path += last_critical_node->getTotalTime();
		last_critical_node = last_critical_node->getPrevCritical();
	}
	std::cerr << "CriticalPathMetric - total time on critical path: " << total_time_on_criticial_path << std::endl;
	std::cerr << "CriticalPathMetric - finished the critical path computation" << std::endl;
}


//======================= TotalTimeMetric ==============================

void TotalTimeMetric::init( Graph* tdg )
{
	Metric::init( tdg );
	
	std::map<int64_t, Node*>& graph_nodes = _tdg->getGraphNodes();
	
	_nodes_total_time = 0;
	for( std::map<int64_t, Node*>::const_iterator it = graph_nodes.begin(); 
		 it != graph_nodes.end(); ++it ) 
	{
		Node* curr_node = it->second;
		//if (curr_node->ignore_node_for_metrics ())
		//	continue;
		_node_times_arr.push_back( curr_node->getTotalTime() );
		
		if( curr_node->getType() == Node::CHUNK_TASK )
		{
			_numChunks++;
			_totalChunkTimes += curr_node->getTotalTime();
		}
		
		if( curr_node->getType() == Node::EXP_TASK )
		{
			_numExplicitTasks++;
			_totalExplicitTimes += curr_node->getTotalTime();
		}
	}
        
    if( _node_times_arr.size() > 0 )
		compute_stats( _node_times_arr, _nodes_total_time, _nodes_avg_time, _nodes_time_stddev, _nodes_med_time );
}


void TotalTimeMetric::printMetric( std::ostream& out_stream )
{
	out_stream << "Total time (ms): " << _nodes_total_time << std::endl;
	out_stream << "Total tasks: " << _node_times_arr.size() << std::endl;
	out_stream << "Total chunks time: " << _totalChunkTimes << std::endl;
	out_stream << "Total chunks: " << _numChunks << std::endl;
	out_stream << "Total explicit time: " << _totalExplicitTimes << std::endl;
	out_stream << "Total explicit tasks: " << _numExplicitTasks << std::endl;
	out_stream << "Average time per task: " << _nodes_avg_time << std::endl;
	out_stream << "Task time stddev: " << _nodes_time_stddev << std::endl;
	out_stream << "Median time: " << _nodes_med_time << std::endl;
	if( _node_times_arr.size() > 0 )
	{
		out_stream << "Min task time: " << _node_times_arr[0] << std::endl;
		out_stream << "Max task time: " << _node_times_arr[_node_times_arr.size()-1] << std::endl;
		out_stream << "1st quartile: " << _node_times_arr[_node_times_arr.size()/4] << std::endl;
		out_stream << "2nd quartile (median): " << _node_times_arr[_node_times_arr.size()/2] << std::endl;
		out_stream << "3rd quartile: " << _node_times_arr[(_node_times_arr.size()*3)/4] << std::endl;
	}
}

//======================= LogFileMetric ==============================

void LogFileMetric::printMetric( std::ostream& out_stream )
{
	std::ofstream log_file;
	log_file.open( _logFilename.c_str() );
	
	if( !log_file.is_open() ) 
	{
		std::cerr << "libtdg: error opening log file " << _logFilename << std::endl;
		exit( -2 );
	}
	
	std::map<int64_t, Node*>& graph_nodes = _tdg->getGraphNodes();
	
	for( std::map<int64_t, Node*>::const_iterator it = graph_nodes.begin(); 
		 it != graph_nodes.end(); ++it ) 
	{
		Node* curr_node = it->second;
		
		if( curr_node->getType() == Node::CHUNK_TASK )
		{
			log_file << curr_node->getId() << "  " 
			         << curr_node->getTotalTime() << "  "
			         << curr_node->getThreadId() << "  " 
			         << curr_node->getLoopCounter() << "  [" 
			         << curr_node->getLower() << "," 
			         << curr_node->getUpper() << "] " 
			         << curr_node->papiValsToStr( " " ) << std::endl; 
		}
	}

	log_file.close();
}


