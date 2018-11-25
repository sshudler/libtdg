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

#ifndef __METRICS_H__
#define __METRICS_H__

#include "graph.h"


namespace libtdg
{
	class Metric {
	public:	
		Metric () : _tdg( NULL ) {}
		virtual ~Metric () {}
	
		virtual void init( Graph* tdg ) { _tdg = tdg; }
		virtual double getMetric () = 0;
		virtual void printMetric( std::ostream& out_stream ) = 0;
	
	protected:
		Graph*	_tdg;
	};

	class CriticalPathMetric : public Metric {
	public:
		CriticalPathMetric () : _critical_path_time_len( -1 ) {}
		virtual ~CriticalPathMetric () {}
	
		virtual double getMetric( );
		virtual void printMetric( std::ostream& out_stream );
	
	private:
		double _critical_path_time_len;
		int _critical_path_len;
	
		void computeCriticalPath( );
	};

	class TotalTimeMetric : public Metric {
	public:
		TotalTimeMetric () 
		: _nodes_total_time( -1 ), _numChunks( 0 ), _totalChunkTimes( 0.0 ),
		  _numExplicitTasks( 0 ), _totalExplicitTimes( 0.0 ) {}
	
		virtual void init( Graph* tdg );
		virtual double getMetric () { return _nodes_total_time; }
		virtual void printMetric( std::ostream& out_stream );
		
	private:
		double _nodes_total_time;	
		double _nodes_avg_time;
		double _nodes_time_stddev;
		double _nodes_med_time;
		std::vector<double> _node_times_arr;
		
		unsigned int 	_numChunks;
		double			_totalChunkTimes;
		
		unsigned int 	_numExplicitTasks;
		double 			_totalExplicitTimes;
	};

	class SimpleDotFileMetric : public Metric {
	public:
		SimpleDotFileMetric( const char* dotfile ) : _dotFilename( dotfile ) {}
	
		virtual double getMetric( ) { return 0.0; }
		virtual void printMetric( std::ostream& out_stream ) {	_tdg->printDotFile( _dotFilename ); }
	
	private:
		std::string     _dotFilename;
	};
	
	class LogFileMetric : public Metric
	{
	public:
		LogFileMetric( const char* logfile ) : _logFilename( logfile ) {}
		
		virtual double getMetric( ) { return 0.0; }
		virtual void printMetric( std::ostream& out_stream );
		
	private:
		std::string     _logFilename;
	};

}	// namespace libtdg

#endif		// __METRICS_H__
