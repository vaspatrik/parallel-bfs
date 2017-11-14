#ifndef MSPBFS_H
#define MSPBFS_H

#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/task_scheduler_init.h"

#include "Types.h"


template <unsigned int bitsetSize, typename Call>
class MsBfsTask;

template <unsigned int bitsetSize>
class MsBfs
{
public:
    static const int NODE_PER_WORKER;

    MsBfs(const ListGraph& g_):g(g_) {
    }

    void topDownMsPbfs(const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback);
    void bottomUpMsPbfs(const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback);
    void foundNew();
    ListGraph::NodeMap<ParallelLabel>& seen();
    ListGraph::NodeMap<ParallelLabel>& frontier();
    ListGraph::NodeMap<ParallelLabel>& next();
    const ListGraph& getGraph();
    std::size_t getIterationNum();
private:
    void initTasks(std::function<PrintFunctionType<bitsetSize>> callback);
    void getOrderNodesDegree(std::vector<Node> &degreeOrderedNodes);
    const ListGraph& g;
    std::vector<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> tasks;   
    ListGraph::NodeMap<ParallelLabel>* ptrFrontier;
    ListGraph::NodeMap<ParallelLabel>* ptrNext;
    ListGraph::NodeMap<ParallelLabel>* ptrSeen;
    std::atomic<bool> foundNewNode;
    std::size_t iterationNum;
    std::size_t test;
};

template <unsigned int bitsetSize>
const int MsBfs<bitsetSize>::NODE_PER_WORKER = 3;//256;

template <unsigned int bitsetSize>
void MsBfs<bitsetSize>::getOrderNodesDegree(std::vector<Node> &degreeOrderedNodes)
{
    
    InDegMap<ListGraph> degrees(g); 
  
    for (ListGraph::NodeIt n(g); n!=INVALID; ++n) {
        degreeOrderedNodes.push_back(n);
    }
    std::sort(degreeOrderedNodes.begin(), degreeOrderedNodes.end(),
        [&degrees] (const auto& lhs, const auto& rhs) {return degrees[lhs] > degrees[rhs];});
}

template <unsigned int bitsetSize, typename Call>
class MsBfsTask {
public:
    MsBfsTask(MsBfs<bitsetSize>* mspbfs_, Call callback_): taskNodes(), mspbfs(mspbfs_), callback(callback_)

    {
        taskNodes.reserve(MsBfs<bitsetSize>::NODE_PER_WORKER);        
    }

    void getNeighboursTopDown() {
        auto& g = mspbfs->getGraph();
        auto& seen = mspbfs->seen();
        auto& frontier = mspbfs->frontier();
        auto& next = mspbfs->next();
        int oldNext;
        int newNext;
        // body of the algorithm (Listing 1)
        for (auto v: taskNodes)
        {
            if(frontier[v]->load() == 0)
            {
                continue;
            }

            for (ListGraph::IncEdgeIt e(g, v); e!=INVALID; ++e) //iterating edges starting from v
            {

                Node neighbour = g.runningNode(e); // 'other' end of edge (ie neighbours)
                do {
                    oldNext = next[neighbour]->load();
                    newNext = oldNext | frontier[v]->load();
                } while(!next[neighbour]->compare_exchange_weak(oldNext, newNext));
            }
        }
    }

    void processNodesTopDown() {
        auto& g = mspbfs->getGraph();
        auto& next = mspbfs->next();
        auto& seen = mspbfs->seen();

        for (auto v: taskNodes)
        {   
             // 
            if(next[v]->load() == 0)
            {
                continue;
            }

            *(next[v]) &= ~(seen[v]->load());
            *(seen[v]) |= next[v]->load();

            if(next[v]->load() != 0)
            {
                callback(mspbfs->getIterationNum(), g.id(v), g.maxNodeId(), next[v]->load());
                mspbfs->foundNew();
            }
        }

    }

    void doBottomUp() {
        auto& g = mspbfs->getGraph();
        auto& seen = mspbfs->seen();
        auto& frontier = mspbfs->frontier();
        auto& next = mspbfs->next();

        for (auto v: taskNodes)
        {
            if(seen[v]->load() == ~(-1 << bitsetSize))
            {
                continue;
            }

            for (ListGraph::IncEdgeIt e(g, v); e!=INVALID; ++e) //iterating edges starting from v
            {

                Node neighbour = g.runningNode(e); // 'other' end of edge (ie neighbours)
                *next[v] |= frontier[neighbour]->load();
            }
            *next[v] &= ~(seen[v]->load());
            *seen[v] |= next[v]->load();
            
            if(next[v]->load() != 0)
            {
                callback(mspbfs->getIterationNum(), g.id(v), g.maxNodeId(), next[v]->load());
                mspbfs->foundNew();
            }
        }
    }

    void cleanNext() {
        auto& g = mspbfs->getGraph();
        auto& next = mspbfs->next();
        for(auto v: taskNodes)
        {
            next[v]->store(0);
        }
    }

    void addNode(Node n)
    {
        taskNodes.push_back(n);
    }
    private:
        std::vector<Node> taskNodes;
        MsBfs<bitsetSize>* mspbfs;
        Call callback;
};

template <unsigned int bitsetSize>
void MsBfs<bitsetSize>::initTasks(std::function<PrintFunctionType<bitsetSize>> callback)
{
    int nodeNumber = countNodes(g);
    int taskNumber = (nodeNumber > NODE_PER_WORKER) ? (nodeNumber / NODE_PER_WORKER) : 1;

    std::vector<Node> degreeOrderedNodes;

    degreeOrderedNodes.reserve(nodeNumber);
    tasks.clear();
    tasks.reserve(taskNumber);
    
    getOrderNodesDegree(degreeOrderedNodes);

    for (int i = 0; i < taskNumber; ++i)
    {
        tasks.emplace_back(MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>(this, callback));
    }
    
    for (int i = 0; i < nodeNumber; ++i) {
        tasks[i%taskNumber].addNode(degreeOrderedNodes[i]);
    }
}

template <unsigned int bitsetSize>
void MsBfs<bitsetSize>::topDownMsPbfs(const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback) 
{
      // during the loop we always use the previous 'next' as the new 'frontier'. To avoid data copying we are simply swapping two maps in each iteration.
    ListGraph::NodeMap<ParallelLabel> map1(g, std::shared_ptr<std::atomic<int>>()); // we use map1 as the frontier at first
    ListGraph::NodeMap<ParallelLabel> map2(g, std::shared_ptr<std::atomic<int>>());
    ListGraph::NodeMap<ParallelLabel> seen_map(g, std::shared_ptr<std::atomic<int>>());
    
    for(typename ListGraph::NodeIt v(g); v != INVALID; ++v)
    {
        map1[v].reset(new std::atomic<int>(0));
        map2[v].reset(new std::atomic<int>(0));
        seen_map[v].reset(new std::atomic<int>(0));
    }

    ptrFrontier = &map1;
    ptrNext     = &map2;
    ptrSeen     = &seen_map;
    foundNewNode.store(true);
    iterationNum = 1;

    initTasks(callback);
    
    NeighbourTopDownExecutor<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> neighbourExecutor(tasks);
    NodeProcessorTopDownExecutor<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> nodeProcessorExecutor(tasks);
    CleanerExecutor<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> cleanerExecutor(tasks);

    // initializing start nodes
    for(std::size_t i = 0; i < sources.size(); ++i)
    {
        Node s = sources[i];
        map1[s]->store(1 << i); // set frontier for sources
        seen_map[s]->store(1 << i); // set seen for sources
    }
     
   
    while(foundNewNode)
    {
        ptrFrontier = iterationNum % 2 == 1 ? &map1 : &map2;
        ptrNext     = iterationNum % 2 == 0 ? &map1 : &map2;
        foundNewNode.store(false);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),cleanerExecutor);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),neighbourExecutor);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),nodeProcessorExecutor);
        ++iterationNum; 
    }
}

template <unsigned int bitsetSize>
void MsBfs<bitsetSize>::bottomUpMsPbfs(const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback) 
{
      // during the loop we always use the previous 'next' as the new 'frontier'. To avoid data copying we are simply swapping two maps in each iteration.
    ListGraph::NodeMap<ParallelLabel> map1(g, std::shared_ptr<std::atomic<int>>()); // we use map1 as the frontier at first
    ListGraph::NodeMap<ParallelLabel> map2(g, std::shared_ptr<std::atomic<int>>());
    ListGraph::NodeMap<ParallelLabel> seen_map(g, std::shared_ptr<std::atomic<int>>());
    
    for(typename ListGraph::NodeIt v(g); v != INVALID; ++v)
    {
        map1[v].reset(new std::atomic<int>(0));
        map2[v].reset(new std::atomic<int>(0));
        seen_map[v].reset(new std::atomic<int>(0));
    }

    ptrFrontier = &map1;
    ptrNext     = &map2;
    ptrSeen     = &seen_map;
    foundNewNode.store(true);
    iterationNum = 1;

    initTasks(callback);
    
    MsPBfsBottomUpExecutor<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> bottomUpExecutor(tasks);
    CleanerExecutor<MsBfsTask<bitsetSize, std::function<PrintFunctionType<bitsetSize>>>> cleanerExecutor(tasks);

    // initializing start nodes
    for(std::size_t i = 0; i < sources.size(); ++i)
    {
        Node s = sources[i];
        map1[s]->store(1 << i); // set frontier for sources
        seen_map[s]->store(1 << i); // set seen for sources
    }
     
   
    while(foundNewNode)
    {
        ptrFrontier = iterationNum % 2 == 1 ? &map1 : &map2;
        ptrNext     = iterationNum % 2 == 0 ? &map1 : &map2;
        foundNewNode.store(false);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),cleanerExecutor);
        tbb::parallel_for(tbb::blocked_range<size_t>(0,tasks.size()),bottomUpExecutor);
        ++iterationNum; 
    }
}


template <unsigned int bitsetSize>
void MsBfs<bitsetSize>::foundNew() {
    foundNewNode.store(true);   
}

template <unsigned int bitsetSize>
const ListGraph& MsBfs<bitsetSize>::getGraph()
{
    return g;
}

template <unsigned int bitsetSize>
std::size_t MsBfs<bitsetSize>::getIterationNum()
{
    return iterationNum;
}

template <unsigned int bitsetSize>
ListGraph::NodeMap<ParallelLabel>& MsBfs<bitsetSize>::seen() {
    return *ptrSeen;
}

template <unsigned int bitsetSize>
ListGraph::NodeMap<ParallelLabel>& MsBfs<bitsetSize>::frontier() {
    return *ptrFrontier;
}

template <unsigned int bitsetSize>
ListGraph::NodeMap<ParallelLabel>& MsBfs<bitsetSize>::next() {
    return *ptrNext;
}



#endif