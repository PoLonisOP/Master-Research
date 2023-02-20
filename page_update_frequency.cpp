#include <cstdio>
#include <iostream>
#include <time.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include <list>
#include <utility>
//pagerank
#include <cstring>
#include <string>
#include <map>
#include <unordered_map>
#include<iomanip>

#define BFS 1

using namespace std;

//Adj list struct sample code
/*struct node{
    int vertex;
    struct node* next;
};

struct Graph{
    int numVertices;
    struct node** adjLists;
};*/

vector<vector<int>> page_for_rand;
vector<vector<int>> page_for_adj; // {src, out-degree, dst1, dst2...}
vector<vector<int>> cache_rand;
pair<vector<int>, vector<vector<int>>> cache_adj;

int page_size, cache_amount;
int cache_replacement_for_random = 0;
int cache_replacement_for_adj = 0;
int algo = 0, skip_the_page = 0;

void updateEdge_with_random(int, int, int);
void updateEdge_with_adj(int, int, int);
pair<pair<int, int>, pair<int, int>> findAdjPageOffset(int, int, int);
map<int, int> srcPageNum; //the first is src vertex, and the second is page number of src vertex
map<int, vector<pair<int, int>>> adjSrcPageNumberMap; //key is src, and the value is <pageNum, lastDst>...

class Graph {
    int numVertices;
    vector<vector<int>> adj;
    vector<vector<int>> rev_adj;
    vector<double> pk;
    
  public:
    Graph(int);
    void addEdge(int, int);
    void addEdge_rev(int, int);
    void prt_adjacencylist();
    void prt_rev_adjacencylist();
    void prt_pageRank();
    void StorePageAdjMethod(int);
    vector<int> getOutDegree();
    // prints BFS traversal from a given source s
    void bfs(int, int);
    void pageRank(int, int);
};

Graph::Graph(int numVertices) {
    this->numVertices = numVertices;
    this->adj = vector<vector<int>>(numVertices);
    this->rev_adj = vector<vector<int>>(numVertices);
}

// Add edge to adj
void Graph::addEdge(int s, int d) {
    this->adj[s].push_back(d);
}

void Graph::prt_adjacencylist() {
    cout << "Adjacency list: " << endl;
    for (int i = 0; i < this->numVertices; i++) {
        cout << i << "->";
        for (int x : this->adj[i]) {
            cout << x << " ";
        }
        cout << endl;
    }
    cout << endl;
}

//Add Edge to rev_adj
void Graph::addEdge_rev(int s, int d) {
    this->rev_adj[d].push_back(s);
}

void Graph::prt_rev_adjacencylist() {
    cout << "Reverse adjacency list: " << endl;
    for (int i = 0; i < this->numVertices; i++) {
        cout << i << "->";
        for (int x : this->rev_adj[i]) {
            cout << x << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void Graph::prt_pageRank() {
    for (int i = 0; i < numVertices; i++)
        cout << "Pagerank of node " << i << ": " << pk.at(i) << endl;
    cout << endl;
}

void prt_cache_rand_data() {
    for (auto it: cache_rand) {
        for (int i = 0; i < it.size(); i++)
            cout << it[i] << " ";
        cout << endl;
    }
    cout << endl;
}

void prt_cache_adj_data() {
    cout << "The page number in cache_adj: ";
    for (auto it: cache_adj.first)
        cout << it << " ";
    cout << endl << "The cache data in cache_adj: " << endl;
    for (auto it: cache_adj.second) {
        for (int i = 0; i < it.size(); i++)
            cout << it[i] << " ";
        cout << endl;
    }
    cout << endl;
}

void Graph::StorePageAdjMethod(int page_amount) {
    vector<int> outDegree = getOutDegree();   //get the vector of out degree
    int n = 0, tmp = 0;
    bool flag_for_map = true;
    int Ldst;
    for (int i = 0; i < numVertices; i++) {
        if (outDegree[i] == 0) continue;
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_adj[n][tmp++] = i;
        srcPageNum[i] = n;
        adjSrcPageNumberMap.insert(make_pair(i, vector<pair<int, int>>()));
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_adj[n][tmp++] = outDegree[i];
        for (int dst: adj[i]) {
            flag_for_map = true;
            if (tmp < page_size) {
                page_for_adj[n][tmp] = dst;
                Ldst = dst;
                if (tmp == page_size - 1) {
                    adjSrcPageNumberMap[i].push_back(make_pair(Ldst, n));
                    flag_for_map = false;
                }
                tmp++;
            }
            else if (n < page_amount) {
                tmp = 0, n++;
                page_for_adj[n][tmp++] = dst;
                Ldst = dst;
            }
            else
                assert(0);
        }
        if (flag_for_map) {
            adjSrcPageNumberMap[i].push_back(make_pair(Ldst, n));
            flag_for_map = true;
        }
    }
}

vector<int> Graph::getOutDegree() {
    vector<int> degree(numVertices , 0);
    for (int it = 0; it < numVertices; it++)
        degree[it] = adj[it].size();
    return degree;
}

void Graph::pageRank(int pow_iter, int cache_size) {
    vector<int> outDegree = getOutDegree();   //get the vector of out degree
    vector<double> init_vec(numVertices , 1.0 / numVertices);    //r(0)
    vector<double> rank_vec(numVertices , 0.0);       //init the rank

    if (pow_iter == 1) {
        for (auto it = 0; it < numVertices; it++) {
            cout << "Vertex " << it << " pagerank: ";
            cout << fixed << showpoint;
            cout << setprecision(2);
            cout << (1.0 / numVertices) << endl;
        }
    } else {
        for (int i = 0; i < pow_iter; i++) {
            if (i != 0) {
                init_vec = rank_vec;
            }
            for (int j = 0; j < numVertices; j++) {
                double sum = 0;
                for (int k = 0 ; k < rev_adj[j].size(); k++) {
                    updateEdge_with_random(rev_adj[j][k], j, cache_size);
                    updateEdge_with_adj(rev_adj[j][k], j, cache_size);
                    sum += (init_vec.at(rev_adj[j].at(k)) * (1.0 / outDegree.at(rev_adj[j].at(k))));
                }
                rank_vec.at(j) = sum;
				if (i == pow_iter - 1) {
                    pk.push_back(sum);
                    cout << "Vertex " << j << " pagerank: ";
					cout << fixed << showpoint;
					cout << setprecision(2);
					cout << sum << endl << endl;
				}
            }
        }
    }
}

void Graph::bfs(int s, int cache_size) {
    // Mark all the vertices as not visited
    vector<bool> visited;
    visited.resize(this->numVertices, false);
 
    // Create a queue for BFS
    list<int> queue;
 
    // Mark the current node as visited and enqueue it
    visited[s] = true;
    queue.push_back(s);

    cout << endl;
    vector<int> BFStraversal;

    while (!queue.empty()) {
        // Dequeue a vertex from queue and print it
        s = queue.front();
        BFStraversal.push_back(s);
        queue.pop_front();

        // Calculate the adj cache replacement times
        //int page_replace = adj[s].size() / (page_size - 1) - 1; // minus 1, cause we use the first element in every page to store the src
        //if (page_replace < 0) page_replace = 0;                                                            // and minus 1 in the end cause we replace the page when page_replace over 1
        //cache_replacement_for_adj += page_replace;

        for (auto adjecent : adj[s]) {
            updateEdge_with_random(s, adjecent, cache_size);
            // Get all adjacent vertices of the dequeued
            // vertex s. If a adjacent has not been visited,
            // then mark it visited and enqueue it
            if (!visited[adjecent]) {
                visited[adjecent] = true;
                queue.push_back(adjecent);
            }
        }
    }
    cout << "BFS traversal: " << endl;
    for (int i: BFStraversal)
        cout << i << " ";
    cout << endl << endl;
}

void updateEdge_with_random(int src, int dst, int cache_size) {
    //Mark if the edge is in caches
    bool flag = false;
    cout << "Current edge is: src: " << src << " dst: " << dst << endl;
    //find the edge if it is existing in cache
    for (auto it : cache_rand) {
        for (int cache_index = 0; cache_index < it.size(); cache_index += 2) {
            if (it[cache_index] == src && it[cache_index + 1] == dst) { 
                cout << "The current edge is in the cache_rand" << endl;
                cout << "Current cache_rand data: " << endl;
                //print the cache data
                prt_cache_rand_data();
                flag = true;
                break;
            }
        }
        if (flag) break;
    }
    //otherwise, update the edge from memory to cache_rand
    if (!flag) {
        for (auto& it : page_for_rand) {
            for (int i = 0; i < it.size(); i += 2) {
                if (it[i] == src && it[i + 1] == dst) {
                    cout << "Find the edge in memory but not in cache_rand" << endl;
                    cout << "Do cache_rand replacement!" << endl;
                    //using cache policy or not
                    //FIFO
                    cache_rand[cache_replacement_for_random++ % cache_size].assign(it.begin(), it.end());
                    cout << "After the cache replacement, current cache_rand data: " << endl;
                    //print the cache data
                    prt_cache_rand_data();
                    flag = true;
                    break;
                }
            }
            if (flag) break;
        }
    }
}

void updateEdge_with_adj(int src, int dst, int cache_size) {
    //Mark if the edge is in caches
    bool flag_for_src_in_cache = false, flag_for_dst_in_cache = false;
    int dstPageNum = INT_MAX;
    int theDecision;
    cout << "Current edge is: src: " << src << " dst: " << dst << endl;
    //find the page number of the dst
    cout << "The Ldst page number map of '" << src << "' is: " << endl;
    for (int i = 0; i < adjSrcPageNumberMap[src].size(); i++) {
        cout << adjSrcPageNumberMap[src][i].first << " " << adjSrcPageNumberMap[src][i].second << endl;
        if (adjSrcPageNumberMap[src][i].first >= dst)
            dstPageNum = adjSrcPageNumberMap[src][i].second;
    }
    //find the edge if it is existing in cache
    for (int it : cache_adj.first) {
        if (srcPageNum[src] == it)
            flag_for_src_in_cache = true;
        if (dstPageNum == it)
            flag_for_dst_in_cache = true;
    }
    if (flag_for_src_in_cache && flag_for_dst_in_cache)
        theDecision = 0;
    else if (flag_for_src_in_cache)
        theDecision = 1;
    else if (flag_for_dst_in_cache)
        theDecision = 2;
    else
        theDecision = 3;

    switch (theDecision)
    {
        case 0: //in cache_adj
            cout << "The current edge is in the cache_adj" << endl \
                << "Current cache_adj data: " << endl;
            cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            //print the cache data
            prt_cache_adj_data();
            break;
        case 1: //src in cache, but not dst
            cout << "The src of current edge is in the cache_adj" << endl \
                << "Do cache_adj replacement! (with only one page)" << endl;
            cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            if (cache_replacement_for_adj % cache_size == srcPageNum[src]) 
                cache_replacement_for_adj++, skip_the_page++;
            cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
            cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
            //print the cache data
            prt_cache_adj_data();
            break;
        case 2: //dst in cache, but not src
            cout << "The dst of current edge is in the cache_adj" << endl \
                << "Do cache_adj replacement! (with only one page)" << endl;
            cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            if (cache_replacement_for_adj % cache_size == dstPageNum) 
                cache_replacement_for_adj++, skip_the_page++;
            cache_adj.first[cache_replacement_for_adj % cache_size] = srcPageNum[src];
            cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[srcPageNum[src]].begin(), page_for_adj[srcPageNum[src]].end());
            //print the cache data
            prt_cache_adj_data();
            break;
        case 3: //otherwise, update the edge from memory to cache_adj
            if (srcPageNum[src] == dstPageNum) {
                cout << "Find the edge in memory but not in cache_adj" << endl \
                    << "Do cache_adj replacement! (with only one page)" << endl;
                cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
                //using cache policy or not
                //FIFO
                cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
                cout << "After the cache replacement, current cache_adj data: " << endl;
                //print the cache data
                prt_cache_adj_data();
            } else {
                cout << "Find the edge in memory but not in cache_adj" << endl \
                    << "Do cache_adj replacement! (with two pages)" << endl;
                cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
                //using cache policy or not
                //FIFO
                cache_adj.first[cache_replacement_for_adj % cache_size] = srcPageNum[src];
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[srcPageNum[src]].begin(), page_for_adj[srcPageNum[src]].end());
                cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
                cout << "After the cache replacement, current cache_adj data: " << endl;
                //print the cache data
                prt_cache_adj_data();
            }
        default:
            break;
    }
}

/*
pair<pair<int, int>, pair<int, int>> findAdjPageOffset(int src, int dst, int find_src) {
    if (page_for_adj[find_src / page_size][find_src % page_size] == src) {
        pair<int, int> src_offset(find_src / page_size, find_src % page_size);
        for (int i = 0; i < ++find_src; i++) 
            if (page_for_adj[++find_src / page_size][find_src % page_size] == dst) 
                return make_pair(src_offset, make_pair(find_src / page_size, find_src % page_size)); //return the dst page index and page offset
        cout << "Cannot find the edge " << src << " -> " << dst << endl;
    } else {
        ++find_src += page_for_adj[find_src / page_size][find_src % page_size];
    }
    return findAdjPageOffset(src, dst, find_src);
}
*/

/*
class LRUCache{
    size_t m_capacity;
    unordered_map<int,  list<pair<int, int>>::iterator> m_map; //m_map_iter->first: key, m_map_iter->second: list iterator;
    list<pair<int, int>> m_list;                               //m_list_iter->first: key, m_list_iter->second: value;
public:
    LRUCache(size_t capacity):m_capacity(capacity) {
    }
    int get(int key) {
        auto found_iter = m_map.find(key);
        if (found_iter == m_map.end()) //key doesn't exist
            return -1;
        m_list.splice(m_list.begin(), m_list, found_iter->second); //move the node corresponding to key to front
        return found_iter->second->second;                         //return value of the node
    }
    void set(int key, int value) {
        auto found_iter = m_map.find(key);
        if (found_iter != m_map.end()) //key exists
        {
            m_list.splice(m_list.begin(), m_list, found_iter->second); //move the node corresponding to key to front
            found_iter->second->second = value;                        //update value of the node
            return;
        }
        if (m_map.size() == m_capacity) //reached capacity
        {
           int key_to_del = m_list.back().first; 
           m_list.pop_back();            //remove node in list;
           m_map.erase(key_to_del);      //remove key in map
        }
        m_list.emplace_front(key, value);  //create new node in list
        m_map[key] = m_list.begin();       //create correspondence between key and node
    }
};
*/

int main() {
    unsigned long edge_num, node_num;
    //pagerank
    int num_of_lines, num_of_po;
    //int swap_times;


    srand( time(NULL) );

    //read the number of power iterations(num_of_po) from cin
    cout << "Input number of pagerank iterations: ";
    cin >> num_of_po;

    //Input the edge account and create the page size and the cache size
    cout << "Input the node amount: ";
    cin >> node_num;
    cout << "Input the edge amount: ";
    cin >> edge_num;
    //How many edges in one page
    cout << "Input the page size: ";
    cin >> page_size;

    vector<vector<int>> gnr_edge(node_num, vector<int>(node_num));
    //Randonly generate the edge with edge type 1 such like (src, dst)
    int page_amount = ceil((double)edge_num * 2 / page_size);

    cout << "Total page amounts: " << page_amount << endl;

    cout << "Input the cache size: ";
    cin >> cache_amount;
    
    page_for_rand = vector<vector<int>>(page_amount, vector<int>(page_size));
    page_for_adj = vector<vector<int>>(page_amount, vector<int>(page_size));
    cache_rand = vector<vector<int>>(cache_amount, vector<int>(page_size));
    cache_adj.first = vector<int>(cache_amount);
    cache_adj.second = vector<vector<int>>(cache_amount, vector<int>(page_size));

    for (int i = 0; i < edge_num; i++) {
        int src = rand() % node_num, dst = rand() % node_num;
        if (gnr_edge[src][dst] || src == dst) 
            i--;
        else {
            gnr_edge[src][dst]++;
            page_for_rand[(2 * i) / page_size][(2 * i) % page_size] = src;
            page_for_rand[(2 * i) / page_size][(2 * i + 1) % page_size] = dst;
        }
    }

    //print the page storage status
    cout << "Page with random access storage: " << endl;
    for (int i = 0; i < page_amount; i++) {
        cout << "p" << i << ": " << endl;
        for (int j = 0; j < page_size; j += 2)
            cout << "src: " << page_for_rand[i][j] << " dst: " << page_for_rand[i][j + 1] << endl;
    }

    cout << endl;
    //Also, generate the edge with edge type 2 such like adj list
    //vector<vector<int>> adjLists(node_num);
    Graph graph(node_num);

    for (int i = 0; i < node_num; i++) {
        for (int j = 0; j < node_num; j++) {
            if (gnr_edge[i][j] == 1)
                graph.addEdge(i, j);
            if (gnr_edge[j][i] == 1)
                graph.addEdge_rev(j, i);
        }
    }

    graph.StorePageAdjMethod(page_amount);

    graph.prt_adjacencylist();

    graph.prt_rev_adjacencylist();

    cout << "Page with adj list method storage: " << endl;
    for (int i = 0; i < page_amount; i++) {
        cout << "p" << i << ": " << endl;
        for (int j = 0; j < page_size; j++)
            cout << page_for_adj[i][j] << " ";
        cout << endl;
    }
    cout << endl;

    cout << "Input the algorithm u want: (the default is pageRank)" << endl;
    cin >> algo;

    switch (algo) {
    case BFS:
        graph.bfs(0, cache_amount);
        break;
    
    default:
        graph.pageRank(num_of_po, cache_amount);
        break;
    }
    
    graph.prt_pageRank();

    for (int i = 0; i < skip_the_page; i++)
        cache_replacement_for_adj--;
    cout << "Cache replacement times with randomly store the edge sequence: " << cache_replacement_for_random << endl;
    cout << "Cache replacement times with Adjacency list store the edge sequence: " << cache_replacement_for_adj << endl;

    return 0;
}