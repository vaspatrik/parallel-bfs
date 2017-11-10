
#include "MsPbfs.h"



//#include <lemon/bfs.h>
 
const std::size_t sourceNum = 3;
  
 
template <unsigned int bitsetSize>
void printNodeFound(std::size_t level, std::size_t nodeId, std::size_t maxNodeId, std::bitset<bitsetSize> foundIn)
{
    std::cout << nodeId << " is found on level\t" << level << "\tin the following BFS(s):\t"; // we want to reverse ids so we see those of the original graph
    //std::cout << nodeId + 1 << " is found on level\t" << level << "\tin the following BFS(s):\t";
    for(std::size_t i = 0; i < foundIn.size(); ++i)
    {
        if(foundIn[i])
        {
            std::cout << i + 1 << " ";
        }
    }
    std::cout << std::endl;
}

 
template <unsigned int bitsetSize>
void TopDownMsBfs(const ListGraph& g, const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback)
{
    // during the loop we always use the previous 'next' as the new 'frontier'. To avoid data copying we are simply swapping two maps in each iteration.
    ListGraph::NodeMap<Label<bitsetSize>> map1(g, 0); // we use map1 as the frontier at first
    ListGraph::NodeMap<Label<bitsetSize>> map2(g, 0);
    ListGraph::NodeMap<Label<bitsetSize>> seen(g, 0);
   
    // initializing start nodes
    for(std::size_t i = 0; i < sources.size(); ++i)
    {
        Node s = sources[i];
        map1[s][i] = 1; // set frontier for sources
        seen[s][i] = 1; // set seen for sources
    }
     
    bool foundNewNode = true;
    std::size_t iterationNum = 1;
   
    while(foundNewNode)
    {
        // these parts were omitted from the paper:
        //  condition for the loop: we check whether we found any new nodes last time (if not then we can have at most one additional round but still way cheaper than checking each bit in each round) - this is a small deviation from the original algorithm
        //  we have to use the previous 'next' as the new 'frontier'
        //  have to set all 'next' values to 0
        foundNewNode = false;
       
        ListGraph::NodeMap<Label<bitsetSize>>& frontier = (iterationNum % 2 == 1 ? map1 : map2);
        ListGraph::NodeMap<Label<bitsetSize>>& next     = (iterationNum % 2 == 0 ? map1 : map2);
       
        for(typename ListGraph::NodeIt v(g); v != INVALID; ++v)
        {
            next[v].reset();
        }
       
        // body of the algorithm (Listing 1)
        for (typename ListGraph::NodeIt v(g); v != INVALID; ++v)
        {
            if(frontier[v].none())
            {
                continue;
            }
           
            for (ListGraph::IncEdgeIt e(g, v); e!=INVALID; ++e) //iterating edges starting from v
            {
                Node neighbour = g.runningNode(e); // 'other' end of edge (ie neighbours)
                next[neighbour] |= frontier[v];
            }
        }
       
        for (typename ListGraph::NodeIt v(g); v != INVALID; ++v)
        {
            if(next[v].none())
            {
                continue;
            }
           
            next[v] &= ~(seen[v]);
            seen[v] |= next[v];
           
            if(next[v].any())
            {
                callback(iterationNum, g.id(v), g.maxNodeId(), next[v]);
                foundNewNode = true;
            }
        }
       
        ++iterationNum;
    }
   
}

template <unsigned int bitsetSize>
void BottomUpMsBfs(const ListGraph& g, const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback)
{
    // during the loop we always use the previous 'next' as the new 'frontier'. To avoid data copying we are simply swapping two maps in each iteration.
    ListGraph::NodeMap<Label<bitsetSize>> map1(g, 0); // we use map1 as the frontier at first
    ListGraph::NodeMap<Label<bitsetSize>> map2(g, 0);
    ListGraph::NodeMap<Label<bitsetSize>> seen(g, 0);
   
    // initializing start nodes
    for(std::size_t i = 0; i < sources.size(); ++i)
    {
        Node s = sources[i];
        map1[s][i] = 1; // set frontier for sources
        seen[s][i] = 1; // set seen for sources
    }
     
    bool foundNewNode = true;
    std::size_t iterationNum = 1;
   
    while(foundNewNode)
    {
        foundNewNode = false;
       
        ListGraph::NodeMap<Label<bitsetSize>>& frontier = (iterationNum % 2 == 1 ? map1 : map2);
        ListGraph::NodeMap<Label<bitsetSize>>& next     = (iterationNum % 2 == 0 ? map1 : map2);
       
        for(typename ListGraph::NodeIt v(g); v != INVALID; ++v)
        {
            next[v].reset();
        }
       
        // body of the algorithm (Listing 2)
        for (typename ListGraph::NodeIt v(g); v != INVALID; ++v)
        {
            if(seen[v].all())
            {
                continue;
            }
           
            for (ListGraph::IncEdgeIt e(g, v); e!=INVALID; ++e) //iterating edges starting from v
            {
                Node neighbour = g.runningNode(e); // 'other' end of edge (ie neighbours)
                next[v] |= frontier[neighbour];
            }
           
            next[v] &= ~(seen[v]);
            seen[v] |= next[v];
           
            if(next[v].any())
            {
                callback(iterationNum, g.id(v), g.maxNodeId(), next[v]);
                foundNewNode = true;
            }
        }
       
        ++iterationNum;
    }
   
}
 
template <unsigned int bitsetSize>
void TopDownMsPBfs(const ListGraph& g, const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback)
{
    MsBfs<sourceNum> msbfs(g);
    msbfs.topDownMsPbfs(sources, callback);
}

template <unsigned int bitsetSize>
void BottomUpMsPBfs(const ListGraph& g, const std::vector<Node>& sources, std::function<PrintFunctionType<bitsetSize>> callback)
{
    MsBfs<sourceNum> msbfs(g);
    msbfs.bottomUpMsPbfs(sources, callback);
}
 
// @param file name to lgf file
int main(int argc, char** argv)
{
    if(argc != 2)
    {
        std::cout << "Usage: executable_name.exe path_and_filename_to_input_graph";
        exit(1);
    }
 
    // read in and initialize graph structure (graph, sources, node labels)
    // note: sources are marked in lgf file (source1, source2, source3, ...)
    ListGraph readInGraph;

    std::vector<Node> sources(sourceNum);
   
    GraphReader<ListGraph> reader(readInGraph, argv[1]);
    for(std::size_t i = 1; i <= sourceNum; ++i)
    {
        std::string attributeName = "source" + std::to_string(i);
        reader.node(attributeName, sources[i - 1]); // read ith source into sources
    }
    reader.run();
   
    std::cout << "TopDownMsBfs: " << std::endl;

    // Top-down MS-BFS
    { // cannot delete NodeMap from graphs so we copy the whole graph each time so that we can label it anew
        ListGraph g;
       
        GraphCopy<ListGraph, ListGraph>(readInGraph, g).run();
        std::function<PrintFunctionType<sourceNum>> callback = printNodeFound<sourceNum>;
        TopDownMsBfs<sourceNum>(g, sources, callback);
    }

    std::cout << "BottomUpMsBfs: " << std::endl;

    // Bottom Up MS-BFS
    { 
        ListGraph g;
       
        GraphCopy<ListGraph, ListGraph>(readInGraph, g).run();
        std::function<PrintFunctionType<sourceNum>> callback = printNodeFound<sourceNum>;
        BottomUpMsBfs<sourceNum>(g, sources, callback);
    }

    std::cout << std::endl;    
    std::cout << "TopDownMsPBfs: " << std::endl;
    
    {
        ListGraph g;
        GraphCopy<ListGraph, ListGraph>(readInGraph, g).run();
        std::function<PrintFunctionType<sourceNum>> callback = printNodeFound<sourceNum>;
        TopDownMsPBfs<sourceNum>(g, sources, callback);
        
    }

    std::cout << "BottomUpMsPBfs: " << std::endl;
    { 
        ListGraph g;
        GraphCopy<ListGraph, ListGraph>(readInGraph, g).run();
        std::function<PrintFunctionType<sourceNum>> callback = printNodeFound<sourceNum>;
        BottomUpMsPBfs<sourceNum>(g, sources, callback);
    }      
   
   
   
    return 0;
}