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

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef HAVE_PAPI
#include <papi.h>
#endif

#include "graph.h"


using namespace libtdg;


//========================== Node ======================================

std::atomic<int64_t> Node::_nextId( 1 );
const char* Node::_typeStrings[10] = 
	{"ROOT_TASK", "IMP_TASK", "WS_TASK", "CHUNK_TASK", "EXP_TASK", "BARRIER", "TASKWAIT"};
const char* Node::_fillColors[10] = 
	{"dimgray", "darkgoldenrod1", "darkolivegreen3", "bisque", "deepskyblue", "firebrick1", "crimson"};


bool Node::isConnectedWith( Node* target ) 
{
	bool res = false;
	_exitsMutex.lock();
	for (std::vector<Edge*>::const_iterator it = _exitEdges.begin (); it != _exitEdges.end (); ++it) {
		if ((*it)->getTarget () == target) {
			res = true;
			break;
		}
	}
	_exitsMutex.unlock();
	return res;
}


Edge* Node::getConnection( Node* target ) const 
{
	Edge* result = NULL;
	for (std::vector<Edge*>::const_iterator it = _exitEdges.begin (); it != _exitEdges.end (); ++it ) {
		if ((*it)->getTarget () == target) {
			result = *it;
			break;
		}
	}
	return result;
}


double Node::maxPredFinishTime( ) 
{
	double max_finish_time = 0.0;
	Node::AdjacencyIterator adj_iter =
			Node::AdjacencyIterator::beginAdjIter (this, true);    // Iterate over entry edges
			Node::AdjacencyIterator adj_iter_end =
					Node::AdjacencyIterator::endAdjIter (this, true);      // Iterate over entry edges
			while (adj_iter != adj_iter_end) {
				Node* source_node = *adj_iter;
				max_finish_time = std::max (source_node->getFinishTime (), max_finish_time);
				++adj_iter;
			}

			return max_finish_time;
}

std::string Node::idToStr()
{
	std::stringstream str_stream;
	
	if( _type == Node::CHUNK_TASK )
	{
		str_stream << _loopCounter << " [" << _lower << ", " << _upper << "]";
	}
	else
	{
		str_stream << _id;
	}
	           
	return str_stream.str();
}

std::string Node::papiValsToStr( const char* sep_str )
{
	std::stringstream str_stream;
	
#ifdef HAVE_PAPI
	for( unsigned int i = 0; i < _numPapiEvents; ++i )
	{
		str_stream << sep_str << _papiValsArr[i];
	}
#endif

	return str_stream.str();
}

void Node::printToStream( std::ostream& str_stream )
{
	const char* sep_str = " * ";
	
	str_stream << getId() << " [style=\"filled\" label=\"" 
	// Label:
               << getTotalTime()
	           << sep_str
	           << idToStr()
               << papiValsToStr( sep_str )
	//-------
	           << "\" type=\"" << getTypeStr() << "\""
	           << (getIsCritical() ? " shape=\"doublecircle\"" : "")
               << " fillcolor=\"" << getFillColor() << "\"];" << std::endl;
}

#ifdef HAVE_PAPI

void Node::initPapiVals( unsigned int num_papi_events )
{
	_numPapiEvents = num_papi_events;
	if( _numPapiEvents > 0 )
	{
		_papiValsArr = new long long[_numPapiEvents];
	}
	std::fill( _papiValsArr, _papiValsArr + _numPapiEvents, 0 );
}
		
void Node::startPapiCounters( int papi_eventset )
{
	PAPI_start( papi_eventset );
}


void Node::endPapiCounters( int papi_eventset )
{
	PAPI_stop( papi_eventset, _papiValsArr );
}

#endif

//========================= Graph ======================================

void Graph::visitNode( Node* curr_node, std::list<Node*>& topo_list ) 
{
	curr_node->setVisited( true );

	Node::AdjacencyIterator adj_iter =
			Node::AdjacencyIterator::beginAdjIter (curr_node, false);   // Iterate over exit edges
	Node::AdjacencyIterator adj_iter_end =
			Node::AdjacencyIterator::endAdjIter (curr_node, false);     // Iterate over exit edges
	while (adj_iter != adj_iter_end) {
		Node* adj_node = *adj_iter;
		if (!adj_node->getVisited ())
			visitNode (adj_node, topo_list);
		++adj_iter;
	}

	topo_list.push_front (curr_node);
}


void Graph::topoSort( std::list<Node*>& topo_list )
{
	for( Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it ) 
	{
		Node* curr_node = it->second;
		if (!curr_node->getVisited ())  // Unmarked node
			visitNode (curr_node, topo_list);
	}

	int max_level = -1;
	for( std::list<Node*>::iterator l_iter = topo_list.begin (); l_iter != topo_list.end (); l_iter++ ) 
	{
		Node* curr_node = *l_iter;

		int max_curr_level = 0;
		Node::AdjacencyIterator adj_iter =
				Node::AdjacencyIterator::beginAdjIter( curr_node, true );    // Iterate over entry edges
		Node::AdjacencyIterator adj_iter_end =
				Node::AdjacencyIterator::endAdjIter( curr_node, true );      // Iterate over entry edges
		while (adj_iter != adj_iter_end) {
			Node* source_node = *adj_iter;
			max_curr_level = std::max( source_node->getLevel() + 1, max_curr_level );
			++adj_iter;
		}
		curr_node->setLevel( max_curr_level );
		max_level = std::max( max_level, max_curr_level );
	}

	std::list<Node*>::iterator start_iter = topo_list.begin();
	for (int i = 0; i < max_level; ++i) 
	{
		while( start_iter != topo_list.end() && (*start_iter)->getLevel() == i )
			++start_iter;
		if( start_iter == topo_list.end() )
			break;
		std::list<Node*>::iterator l_iter = start_iter;
		++l_iter;
		while( l_iter != topo_list.end() )
		{
			Node* curr_node = *l_iter;
			if( curr_node->getLevel() == i )
			{
				std::list<Node*>::iterator next_iter = l_iter;
				++next_iter;
				topo_list.erase( l_iter );
				topo_list.insert( start_iter, curr_node );
				l_iter = next_iter;
			}
			else 
			{
				++l_iter;
			}
		}
	}
}


void Graph::connectNodes( Node* source, Node* target ) 
{
	if (!source->isConnectedWith( target ))
	{
		Edge* new_edge = new Edge( source, target );
		source->getExitsMutex().lock();
		target->getEntriesMutex().lock();
		source->getExits().push_back( new_edge );
		target->getEntries().push_back( new_edge );
		target->getEntriesMutex().unlock();
		source->getExitsMutex().unlock();
	}
}


void Graph::disconnectNodes( Node* source, Node* target ) 
{
	source->getExitsMutex().lock();
	std::vector<Edge*>& exit_edges = source->getExits();
	std::vector<Edge*>::iterator it;
	for( it = exit_edges.begin(); it != exit_edges.end(); ++it )
	{
		if( (*it)->getTarget( ) == target ) 
		{
			exit_edges.erase( it );
            break;
        }
    }
    source->getExitsMutex().unlock();
            
    target->getEntriesMutex().lock();
    std::vector<Edge*>& entry_edges = target->getEntries();
    for( it = entry_edges.begin(); it != entry_edges.end(); ++it )
    {
		if( (*it)->getSource() == source ) 
		{
			entry_edges.erase( it );
            break;
        }
    }
    target->getEntriesMutex().unlock();
}

		
void Graph::printDotFile( const std::string& file_name ) 
{
	std::ofstream dot_file;
	dot_file.open( file_name.c_str() );
	
	if( !dot_file.is_open() ) 
	{
		std::cerr << "libtdg: error opening dot file " << file_name << std::endl;
		exit( -2 );
	}
	
	dot_file << "digraph {" << std::endl;
	for (Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it) 
	{
		Node* curr_node = it->second;
				
		curr_node->printToStream( dot_file );

		Node::AdjacencyIterator adj_iter =
				Node::AdjacencyIterator::beginAdjIter (curr_node, false);   // Iterate over exit edges
		Node::AdjacencyIterator adj_iter_end =
				Node::AdjacencyIterator::endAdjIter (curr_node, false);     // Iterate over exit edges
		while (adj_iter != adj_iter_end) 
		{
			Node* adj_node = *adj_iter;
			dot_file << curr_node->getId () << " -> " << adj_node->getId () << ";" << std::endl;
			++adj_iter;
		}
	}
	dot_file << "}" << std::endl;

	dot_file.close();
}

//int64_t Graph::getMaxInternalId( ) 
//{
//	int64_t max_id = -1;
//
//	for (Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it) 
//	{
//		Node* curr_node = it->second;
//		max_id = std::max (curr_node->getId (), max_id);
//	}
//	return max_id;
//}


//========================= functions ==================================

void compute_stats( std::vector<double>& arr, double& total, double& avg, double& stddev, double& median ) 
{
	unsigned int arr_size = arr.size ();
	total = 0.0;
	for (unsigned int i = 0; i < arr_size; i++)
		total += arr[i];
	avg = total / arr_size;

	double variance = 0.0;
	for (unsigned int i = 0; i < arr_size; i++) {
		double diff = arr[i] - avg;
		variance += (diff * diff);
	}
	variance /= arr_size;

	stddev = std::sqrt (variance);
	
	std::sort( arr.begin(), arr.end() );
	median = arr[arr.size() / 2];
    if ( arr.size() % 2 == 0 ) {
		median = (median + arr[arr.size() / 2 - 1]) / 2.0;
	}
}

