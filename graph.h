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

#ifndef __GRAPH_H__
#define __GRAPH_H__


#include <stdint.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <list>
#include <mutex>
#include <atomic>
#include <string>


#define EPSILON     0.00001


namespace libtdg
{

	class Node;

	//=================================

	class Edge {
	public:
		// Ctor
		Edge (Node* source, Node* target) : _source (source), _target (target) {}
        
		Node* getSource( ) const { return _source; }
		Node* getTarget( ) const { return _target; }
    
	private:
		Node* _source;
		Node* _target;
	};        

	//=================================

	class Node {
	public:

		enum NodeType {
			ROOT_TASK,
			IMP_TASK,
			WS_TASK,
			CHUNK_TASK,
			EXP_TASK,
			BARRIER,
			TASKWAIT
		};
	
		class AdjacencyIterator {
		public:	
			bool operator== (const AdjacencyIterator& other) {
				return (_edge_iter == other._edge_iter);
			}
			bool operator!= (const AdjacencyIterator& other) {
				return !(*this == other);
			}
			Node* operator* () {
				return (_entry_iter ? (*_edge_iter)->getSource () : (*_edge_iter)->getTarget ());
			} 
			AdjacencyIterator& operator++ () {
				++_edge_iter;
				return *this;
			}
		
			static AdjacencyIterator beginAdjIter(Node* node, bool entry_iter) {
				return AdjacencyIterator (node, false, entry_iter);
			}
		
			static AdjacencyIterator endAdjIter(Node* node, bool entry_iter) {
				return AdjacencyIterator (node, true, entry_iter);
			}
	
		private:
			AdjacencyIterator (Node* node, bool end_iter, bool entry_iter) : _entry_iter(entry_iter) {
				if (end_iter) {
					_edge_iter = _entry_iter ? node->getEntries ().end () : node->getExits ().end ();
				}
				else {
					_edge_iter = _entry_iter ? node->getEntries ().begin () : node->getExits ().begin ();
				}
			}
			std::vector<Edge*>::const_iterator _edge_iter;
			bool _entry_iter;
		};
    
		// Ctor
		Node( int64_t id, NodeType type, double total_time )
			: _id( id ), _type( type ), _totalTime( total_time ), _visited( false ), 
			  _level( -1 ), _finishTime( 0.0 ), _lastTime( 0.0 ), _lower( 0 ), _upper( 0 ),
			  _isCritical( false ),_pathLength( 0 ), _pathTime( 0.0 ), _prevCritical( NULL ) 
#ifdef HAVE_PAPI
			  , _numPapiEvents(0), _papiValsArr( NULL )
#endif
			  {}
			  
		~Node()
		{
#ifdef HAVE_PAPI
			  delete[] _papiValsArr;
#endif
		}
        
		int64_t		getId() const 			{ return _id; 			}
		double 		getTotalTime() const 	{ return _totalTime; 	}
		bool 		getVisited() const 		{ return _visited; 		}
		int 		getLevel() const 		{ return _level; 		}
		double 		getFinishTime() const 	{ return _finishTime; 	}
		int			getPathLength() const 	{ return _pathLength;	}
		double  	getPathTime() const 	{ return _pathTime; 	}
		Node*		getPrevCritical() const { return _prevCritical;	}
		bool		getIsCritical() const 	{ return _isCritical;	}
		NodeType	getType() const			{ return _type;			}
		int64_t		getLower() const		{ return _lower;		}
		int64_t		getUpper() const		{ return _upper;		}
		std::vector<Edge*>& getEntries () 		{ return _entryEdges; 			}
		std::vector<Edge*>& getExits () 		{ return _exitEdges; 			}
		std::mutex&			getEntriesMutex() 	{ return _entriesMutex; 		}
		std::mutex&			getExitsMutex() 	{ return _exitsMutex; 			}
		const char*			getTypeStr() 		{ return _typeStrings[_type]; 	}
		const char*			getFillColor() 		{ return _fillColors[_type]; 	}
		uint64_t			getLoopCounter()	{ return _loopCounter;			}
		int					getThreadId()		{ return _threadId;				}
    
		void setLevel( int level ) 				{ _level = level; 				}
		void setVisited( bool visited ) 		{ _visited = visited; 			}
		void setLastTime( double last_time ) 	{ _lastTime = last_time; 		}
		void setIsCritical( bool is_critical )	{ _isCritical = is_critical; 	}
		void setPrevCritical( Node* prev )		{ _prevCritical = prev; 		}
		void setPathLength( int path_length )	{ _pathLength = path_length;	}
		void setPathTime( double path_time )	{ _pathTime = path_time;		}
		void setLowerUpper( int64_t lower, int64_t upper )	{ _lower = lower; _upper = upper; 	}
		void setLoopCounter( uint64_t loop_cnt )			{ _loopCounter = loop_cnt; 			}
		void setThreadId( int thread_id )		{ _threadId = thread_id; 		}
    
		double maxPredFinishTime ();
		bool isConnectedWith (Node* target);
		Edge* getConnection (Node* target) const;
		void addTime( double curr_time ) { _totalTime += (curr_time - _lastTime); _lastTime = curr_time; }
		void printToStream( std::ostream& str_stream );
		std::string papiValsToStr( const char* sep_str );
		std::string idToStr();
		
#ifdef HAVE_PAPI
		void initPapiVals( unsigned int num_papi_events );
		void startPapiCounters( int papi_eventset );
		void endPapiCounters( int papi_eventset );
#endif
    
		static int64_t nextId() { int64_t nid = _nextId.fetch_add( 1 ); return nid; }

	private:
		int64_t 	_id;
		NodeType	_type;
		double  	_totalTime;
		bool    	_visited;
		int     	_level;
		double  	_finishTime;
		double		_lastTime;
		
		// Iteration bounds for loops and chunks
		int64_t		_lower;
		int64_t		_upper;
		uint64_t	_loopCounter;
		int			_threadId;
				
		// Members for critical path computation
		bool			_isCritical;
		int 			_pathLength;
		double 			_pathTime;
        Node*           _prevCritical;
        
#ifdef HAVE_PAPI
		unsigned int	_numPapiEvents;
		long long*		_papiValsArr;
#endif

		std::vector<Edge*>	_entryEdges;
		std::vector<Edge*>	_exitEdges;
		std::mutex			_entriesMutex;
		std::mutex			_exitsMutex;
    
		//static int64_t		_nextId;
		static std::atomic<int64_t> _nextId;
		static const char* 	_typeStrings[10];
		static const char* 	_fillColors[10];
	};

	//=================================

	class Graph {
	public:
		typedef std::map<int64_t, Node*>::iterator NodesIterator;
	
		Graph() {}
    
		~Graph()
		{
			for( Graph::NodesIterator it = _graphNodes.begin(); it != _graphNodes.end(); ++it ) 
				delete it->second;
		}
    
		void addNode( int64_t id, Node* node ) { _addMutex.lock(); _graphNodes[id] = node; _addMutex.unlock(); }
    
		void removeNode( int64_t id ) { _addMutex.lock(); _graphNodes.erase( id ); _addMutex.unlock(); }
		
		//Node* getNode( int64_t id ) { Node* node = NULL; _addMutex.lock(); node = _graphNodes[id]; _addMutex.unlock(); return node; }
    
		std::map<int64_t, Node*>& getGraphNodes() { return _graphNodes; }
    
		void printDotFile( const std::string& file_name );
    
		void topoSort( std::list<Node*>& topo_list );
    
		static void connectNodes( Node* source, Node* target );
		
		static void disconnectNodes( Node* source, Node* target );
    
	private:
		void visitNode( Node* curr_node, std::list<Node*>& topo_list );

		std::map<int64_t, Node*> _graphNodes;
		std::mutex _addMutex;
	};


}	// namespace libtdg


#endif  // __GRAPH_H__

