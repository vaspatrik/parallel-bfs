#ifndef TYPE_H
#define TYPE_H

#include <atomic>
#include <functional>
#include <thread>
#include <bitset>
#include <iostream>

#include <vector>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <chrono>

#include <lemon/list_graph.h>
#include <lemon/lgf_writer.h>
#include <lemon/lgf_reader.h>

using namespace lemon;

using Node = ListGraph::Node;
using Edge = ListGraph::Edge;
 
 
template < unsigned int bitsetSize > using Label = std::bitset<bitsetSize>;
using ParallelLabel = std::shared_ptr<std::atomic<int>>;

template < unsigned int bitsetSize > using PrintFunctionType = void(std::size_t, std::size_t, std::size_t, std::bitset<bitsetSize>);

template<typename T>
struct NeighbourTopDownExecutor
{
	NeighbourTopDownExecutor(std::vector<T>& t):_tasks(t)
	{}
	
	NeighbourTopDownExecutor(NeighbourTopDownExecutor& e,tbb::split):_tasks(e._tasks)
	{}

	void operator()(const tbb::blocked_range<size_t>& r) const {
		for (size_t i=r.begin();i!=r.end();++i)
		_tasks[i].getNeighboursTopDown();
	}

	std::vector<T>& _tasks;
};

template<typename T>
struct NodeProcessorTopDownExecutor
{
	NodeProcessorTopDownExecutor(std::vector<T>& t):_tasks(t)
	{}
	
	NodeProcessorTopDownExecutor(NodeProcessorTopDownExecutor& e,tbb::split):_tasks(e._tasks)
	{}

	void operator()(const tbb::blocked_range<size_t>& r) const {
		for (size_t i=r.begin();i!=r.end();++i)
		_tasks[i].processNodesTopDown();
	}

	std::vector<T>& _tasks;
};



template<typename T>
struct MsPBfsBottomUpExecutor
{
	MsPBfsBottomUpExecutor(std::vector<T>& t):_tasks(t)
	{}
	
	MsPBfsBottomUpExecutor(MsPBfsBottomUpExecutor& e,tbb::split):_tasks(e._tasks)
	{}

	void operator()(const tbb::blocked_range<size_t>& r) const {
		for (size_t i=r.begin();i!=r.end();++i)
		_tasks[i].doBottomUp();
	}

	std::vector<T>& _tasks;
};



template<typename T>
struct CleanerExecutor
{
	CleanerExecutor(std::vector<T>& t):_tasks(t)
	{}
	
	CleanerExecutor(CleanerExecutor& e,tbb::split):_tasks(e._tasks)
	{}

	void operator()(const tbb::blocked_range<size_t>& r) const {
		for (size_t i=r.begin();i!=r.end();++i)
		_tasks[i].cleanNext();
	}

	std::vector<T>& _tasks;
};


#endif