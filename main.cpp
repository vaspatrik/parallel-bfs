#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"
#include <iostream>
#include <vector>
#include <atomic>



std::vector<int> frontier;
std::vector<std::vector<int>> neighbors;
std::vector<int> seen;
std::vector<std::atomic<int>> next;

template<typename T>
bool allZero(std::vector<T> &v)
{
	for (auto& e:v)
	{
		if (e != 0) {
			return false;
		}
	}
	return true;

}

template<typename T>
void resetVisit(std::vector<T> &v)
{
	for (auto& e:v)
	{
		e = 0;
	}
}

//top-down
class BfsTask {
public:
	BfsTask():taskVertices()
	{}

	void operator()() {
		int oldNext = -1;
		int newNext = -1;
		for (auto vertex: taskVertices) {
			if (!frontier[vertex]) continue;
			for (auto neighbor: neighbors[vertex]) {
				do {
					oldNext = next[neighbor];
					newNext = oldNext | frontier[vertex];
				} while(next[neighbor].compare_exchange_weak(oldNext, newNext));
			}
		}
		for (auto vertex: taskVertices) {
			if (!next[vertex]) continue;
			next[vertex] = next[vertex] & !seen[vertex];
			seen[vertex] = seen[vertex] | next[vertex];
			if (next[vertex]) {
				process(vertex);
			}
		} 
	}

	void process(int vertexIndex) {
		//do work
	}

	void addVertex(int i)
	{
		taskVertices.push_back(i);
	}
private:
	std::vector<int> taskVertices;
};

struct executor
{
	executor(std::vector<BfsTask>& t):_tasks(t)
	{}
	
	executor(executor& e,tbb::split):_tasks(e._tasks)
	{}

	void operator()(const tbb::blocked_range<size_t>& r) const {
		for (size_t i=r.begin();i!=r.end();++i)
		_tasks[i]();
	}

	std::vector<BfsTask>& _tasks;
};

int main() {

	tbb::task_scheduler_init init;  // Automatic number of threads
  // tbb::task_scheduler_init init(2);  // Explicit number of threads

	std::vector<BfsTask> tasks;
	for (int i=0;i<1000;++i)
		tasks.push_back(BfsTask());

	executor exec(tasks);
	while(!allZero(next)) {
		resetVisit(next);
		tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),exec);
	}
	std::cout << std::endl;

	return 0;
}