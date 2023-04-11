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

vector<vector<unsigned long>> page_for_rand;
vector<vector<unsigned long>> page_for_adj; // {src, out-degree, dst1, dst2...}
vector<vector<unsigned long>> page_for_inv_adj; // {dst, in-degree, src1, src2...}
vector<vector<unsigned long>> cache_rand;
pair<vector<unsigned long>, vector<vector<unsigned long>>> cache_adj;
pair<vector<unsigned long>, vector<vector<unsigned long>>> cache_inv_adj;
map<unsigned long, unsigned long> srcPageNum; //the first is src vertex, and the second is page number of src vertex
map<unsigned long, vector<pair<unsigned long, unsigned long>>> adjSrcPageNumberMap; //key is src, and the value is <pageNum, lastDst>...
map<unsigned long, unsigned long> invDstPageNum; //the first is dst vertex, and the second is page number of dst vertex
map<unsigned long, vector<pair<unsigned long, unsigned long>>> invAdjDstPageNumberMap; //key is dst, and the value is <pageNum, lastSrc>...

unsigned long page_size, cache_amount;
int cache_replacement_for_random = 0;
int cache_replacement_for_adj = 0;
int cache_replacement_for_inv_adj = 0;
int algo = 0, skip_the_page = 0, r_skip_the_page = 0;

void updateEdge_with_random(unsigned long, unsigned long, unsigned long);
void updateEdge_with_adj(unsigned long, unsigned long, unsigned long);
void updateEdge_with_inv_adj(unsigned long, unsigned long, unsigned long);
pair<pair<unsigned long, unsigned long>, pair<unsigned long, unsigned long>> findAdjPageOffset(unsigned long, unsigned long, unsigned long);

class Graph {
    unsigned long numVertices;
    vector<vector<unsigned long>> adj;
    vector<vector<unsigned long>> inv_adj;
    vector<double> pk;
    
  public:
    Graph(unsigned long);
    void addEdge(unsigned long, unsigned long);
    void addEdge_inv(unsigned long, unsigned long);
    void prt_adjacencylist();
    void prt_inv_adjacencylist();
    void prt_pageRank();
    void StorePageAdjMethod(unsigned long);
    void StorePageinvAdjMethod(unsigned long);
    vector<unsigned long> getOutDegree();
    vector<unsigned long> getInDegree();
    // prints BFS traversal from a given source s
    void bfs(unsigned long, unsigned long);
    void pageRank(unsigned long, unsigned long);
};

Graph::Graph(unsigned long numVertices) {
    this->numVertices = numVertices;
    this->adj = vector<vector<unsigned long>>(numVertices);
    this->inv_adj = vector<vector<unsigned long>>(numVertices);
}

// Add edge to adj
void Graph::addEdge(unsigned long s, unsigned long d) {
    this->adj[s].push_back(d);
}

void Graph::prt_adjacencylist() {
    cout << "Adjacency list: " << endl;
    for (unsigned long i = 0; i < this->numVertices; i++) {
        cout << i << "->";
        for (unsigned long x : this->adj[i]) {
            cout << x << " ";
        }
        cout << endl;
    }
    cout << endl;
}

//Add Edge to inv_adj
void Graph::addEdge_inv(unsigned long s, unsigned long d) {
    this->inv_adj[d].push_back(s);
}

void Graph::prt_inv_adjacencylist() {
    cout << "Inverse adjacency list: " << endl;
    for (unsigned long i = 0; i < this->numVertices; i++) {
        cout << i << "->";
        for (unsigned long x : this->inv_adj[i]) {
            cout << x << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void Graph::prt_pageRank() {
    for (unsigned long i = 0; i < numVertices; i++)
        cout << "Pagerank of node " << i << ": " << pk.at(i) << endl;
    cout << endl;
}

void prt_cache_rand_data() {
    for (auto it: cache_rand) {
        for (unsigned long i = 0; i < it.size(); i++)
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
        for (unsigned long i = 0; i < it.size(); i++)
            cout << it[i] << " ";
        cout << endl;
    }
    cout << endl;
}

void prt_cache_inv_adj_data() {
    cout << "The page number in cache_inv_adj: ";
    for (auto it: cache_inv_adj.first)
        cout << it << " ";
    cout << endl << "The cache data in cache_inv_adj: " << endl;
    for (auto it: cache_inv_adj.second) {
        for (unsigned long i = 0; i < it.size(); i++)
            cout << it[i] << " ";
        cout << endl;
    }
    cout << endl;
}

void Graph::StorePageAdjMethod(unsigned long page_amount) {
    vector<unsigned long> outDegree = getOutDegree();   //get the vector of out degree
    unsigned long n = 0, tmp = 0;
    bool flag_for_map = true;
    unsigned long Ldst;
    for (unsigned long i = 0; i < numVertices; i++) {
        if (outDegree[i] == 0) continue;
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_adj[n][tmp++] = i;
        srcPageNum[i] = n;
        adjSrcPageNumberMap.insert(make_pair(i, vector<pair<unsigned long, unsigned long>>()));
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_adj[n][tmp++] = outDegree[i];
        for (unsigned long dst: this->adj[i]) {
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

void Graph::StorePageinvAdjMethod(unsigned long page_amount) {
    vector<unsigned long> inDegree = getInDegree();   //get the vector of in degree
    unsigned long n = 0, tmp = 0;
    bool flag_for_map = true;
    unsigned long Lsrc;
    for (unsigned long i = 0; i < numVertices; i++) {
        if (inDegree[i] == 0) continue;
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_inv_adj[n][tmp++] = i;
        invDstPageNum[i] = n;
        invAdjDstPageNumberMap.insert(make_pair(i, vector<pair<unsigned long, unsigned long>>()));
        if (tmp >= page_size) {
            tmp = 0, n++;
        }
        page_for_inv_adj[n][tmp++] = inDegree[i];
        for (unsigned long src: this->inv_adj[i]) {
            flag_for_map = true;
            if (tmp < page_size) {
                page_for_inv_adj[n][tmp] = src;
                Lsrc = src;
                if (tmp == page_size - 1) {
                    invAdjDstPageNumberMap[i].push_back(make_pair(Lsrc, n));
                    flag_for_map = false;
                }
                tmp++;
            }
            else if (n < page_amount) {
                tmp = 0, n++;
                page_for_inv_adj[n][tmp++] = src;
                Lsrc = src;
            }
            else
                assert(0);
        }
        if (flag_for_map) {
            invAdjDstPageNumberMap[i].push_back(make_pair(Lsrc, n));
            flag_for_map = true;
        }
    }
}

vector<unsigned long> Graph::getOutDegree() {
    vector<unsigned long> degree(numVertices , 0);
    for (unsigned long it = 0; it < numVertices; it++)
        degree[it] = adj[it].size();
    return degree;
}

vector<unsigned long> Graph::getInDegree() {
    vector<unsigned long> degree(numVertices , 0);
    for (unsigned long it = 0; it < numVertices; it++)
        degree[it] = inv_adj[it].size();
    return degree;
}

void Graph::pageRank(unsigned long pow_iter, unsigned long cache_size) {
    vector<unsigned long> outDegree = getOutDegree();   //get the vector of out degree
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
        for (unsigned long i = 0; i < pow_iter; i++) {
            if (i != 0) {
                init_vec = rank_vec;
            }
            for (unsigned long j = 0; j < numVertices; j++) {
                double sum = 0;
                for (unsigned long k = 0 ; k < inv_adj[j].size(); k++) {
                    updateEdge_with_random(inv_adj[j][k], j, cache_size);
                    updateEdge_with_adj(inv_adj[j][k], j, cache_size);
                    updateEdge_with_inv_adj(inv_adj[j][k], j, cache_size);
                    sum += (init_vec.at(inv_adj[j].at(k)) * (1.0 / outDegree.at(inv_adj[j].at(k))));
                }
                rank_vec.at(j) = sum;
				if (i == pow_iter - 1) {
                    pk.push_back(sum);
                    //cout << "Vertex " << j << " pagerank: ";
					cout << fixed << showpoint;
					cout << setprecision(2);
					//cout << sum << endl << endl;
				}
            }
        }
    }
}

void Graph::bfs(unsigned long s, unsigned long cache_size) {
    // Mark all the vertices as not visited
    vector<bool> visited;
    visited.resize(this->numVertices, false);
 
    // Create a queue for BFS
    list<unsigned long> queue;
 
    // Mark the current node as visited and enqueue it
    visited[s] = true;
    queue.push_back(s);

    cout << endl;
    vector<unsigned long> BFStraversal;

    while (!queue.empty()) {
        // Dequeue a vertex from queue and prunsigned long it
        s = queue.front();
        BFStraversal.push_back(s);
        queue.pop_front();

        // Calculate the adj cache replacement times
        //unsigned long page_replace = adj[s].size() / (page_size - 1) - 1; // minus 1, cause we use the first element in every page to store the src
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
    for (unsigned long i: BFStraversal)
        cout << i << " ";
    cout << endl << endl;
}

void updateEdge_with_random(unsigned long src, unsigned long dst, unsigned long cache_size) {
    //Mark if the edge is in caches
    bool flag = false;
    //cout << "Current edge is: src: " << src << " dst: " << dst << endl;
    //find the edge if it is existing in cache
    for (auto it : cache_rand) {
        for (unsigned long cache_index = 0; cache_index < it.size(); cache_index += 2) {
            if (it[cache_index] == src && it[cache_index + 1] == dst) { 
                //cout << "The current edge is in the cache_rand" << endl;
                //cout << "Current cache_rand data: " << endl;
                //print the cache data
                //prt_cache_rand_data();
                flag = true;
                break;
            }
        }
        if (flag) break;
    }
    //otherwise, update the edge from memory to cache_rand
    if (!flag) {
        for (auto& it : page_for_rand) {
            for (unsigned long i = 0; i < it.size(); i += 2) {
                if (it[i] == src && it[i + 1] == dst) {
                    //cout << "Find the edge in memory but not in cache_rand" << endl;
                    //cout << "Do cache_rand replacement!" << endl;
                    //using cache policy or not
                    //FIFO
                    cache_rand[cache_replacement_for_random++ % cache_size].assign(it.begin(), it.end());
                    //cout << "After the cache replacement, current cache_rand data: " << endl;
                    //print the cache data
                    //prt_cache_rand_data();
                    flag = true;
                    break;
                }
            }
            if (flag) break;
        }
    }
}

void updateEdge_with_adj(unsigned long src, unsigned long dst, unsigned long cache_size) {
    //Mark if the edge is in caches
    bool flag_for_src_in_cache = false, flag_for_dst_in_cache = false;
    unsigned long dstPageNum = INT_MAX;
    unsigned long theDecision;
    //cout << "Current edge is: src: " << src << " dst: " << dst << endl;
    //find the page number of the dst
    //cout << "The Ldst page number map of '" << src << "' is: " << endl;
    for (unsigned long i = 0; i < adjSrcPageNumberMap[src].size(); i++) {
        //cout << adjSrcPageNumberMap[src][i].first << " " << adjSrcPageNumberMap[src][i].second << endl;
        if (adjSrcPageNumberMap[src][i].first >= dst)
            dstPageNum = adjSrcPageNumberMap[src][i].second;
    }
    //find the edge if it is existing in cache
    for (unsigned long it : cache_adj.first) {
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
            //cout << "The current edge is in the cache_adj" << endl \
            //    << "Current cache_adj data: " << endl;
            //cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            //print the cache data
            //prt_cache_adj_data();
            break;
        case 1: //src in cache, but not dst
            //cout << "The src of current edge is in the cache_adj" << endl \
            //    << "Do cache_adj replacement! (with only one page)" << endl;
            //cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            if (cache_replacement_for_adj % cache_size == srcPageNum[src]) 
                cache_replacement_for_adj++, skip_the_page++;
            cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
            cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
            //print the cache data
            //prt_cache_adj_data();
            break;
        case 2: //dst in cache, but not src
            //cout << "The dst of current edge is in the cache_adj" << endl \
            //    << "Do cache_adj replacement! (with only one page)" << endl;
            //cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
            if (cache_replacement_for_adj % cache_size == dstPageNum) 
                cache_replacement_for_adj++, skip_the_page++;
            cache_adj.first[cache_replacement_for_adj % cache_size] = srcPageNum[src];
            cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[srcPageNum[src]].begin(), page_for_adj[srcPageNum[src]].end());
            //print the cache data
            //prt_cache_adj_data();
            break;
        case 3: //otherwise, update the edge from memory to cache_adj
            if (srcPageNum[src] == dstPageNum) {
                //cout << "Find the edge in memory but not in cache_adj" << endl \
                //    << "Do cache_adj replacement! (with only one page)" << endl;
                //cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
                //using cache policy or not
                //FIFO
                cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
                //cout << "After the cache replacement, current cache_adj data: " << endl;
                //print the cache data
                //prt_cache_adj_data();
            } else {
                //cout << "Find the edge in memory but not in cache_adj" << endl \
                //    << "Do cache_adj replacement! (with two pages)" << endl;
                //cout << "The page number of src: " << srcPageNum[src] << ", and the page number of dst: " << dstPageNum << endl;
                //using cache policy or not
                //FIFO
                cache_adj.first[cache_replacement_for_adj % cache_size] = srcPageNum[src];
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[srcPageNum[src]].begin(), page_for_adj[srcPageNum[src]].end());
                cache_adj.first[cache_replacement_for_adj % cache_size] = dstPageNum;
                cache_adj.second[cache_replacement_for_adj++ % cache_size].assign(page_for_adj[dstPageNum].begin(), page_for_adj[dstPageNum].end());
                //cout << "After the cache replacement, current cache_adj data: " << endl;
                //print the cache data
                //prt_cache_adj_data();
            }
        default:
            break;
    }
}

void updateEdge_with_inv_adj(unsigned long src, unsigned long dst, unsigned long cache_size) {
    //Mark if the edge is in caches
    bool flag_for_src_in_cache = false, flag_for_dst_in_cache = false;
    unsigned long invSrcPageNum = INT_MAX;
    unsigned long theDecision;
    //cout << "Current edge is: src: " << src << " dst: " << dst << endl;
    //find the page number of the dst
    //cout << "The Lsrc page number map of '" << dst << "' is: " << endl;
    for (unsigned long i = 0; i < invAdjDstPageNumberMap[dst].size(); i++) {
        //cout << invAdjDstPageNumberMap[dst][i].first << " " << invAdjDstPageNumberMap[dst][i].second << endl;
        if (invAdjDstPageNumberMap[dst][i].first >= src)
            invSrcPageNum = invAdjDstPageNumberMap[dst][i].second;
    }
    //find the edge if it is existing in cache
    for (unsigned long it : cache_inv_adj.first) {
        if (invSrcPageNum == it)
            flag_for_src_in_cache = true;
        if (invDstPageNum[dst] == it)
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
        case 0: //in cache_inv_adj
            //cout << "The current edge is in the cache_inv_adj" << endl \
            //    << "Current cache_inv_adj data: " << endl;
            //cout << "The page number of src: " << invSrcPageNum << ", and the page number of dst: " << invDstPageNum[dst] << endl;
            //print the cache data
            //prt_cache_inv_adj_data();
            break;
        case 1: //src in cache, but not dst
            //cout << "The src of current edge is in the cache_inv_adj" << endl \
            //    << "Do cache_inv_adj replacement! (with only one page)" << endl;
            //cout << "The page number of src: " << invSrcPageNum << ", and the page number of dst: " << invDstPageNum[dst] << endl;
            if (cache_replacement_for_inv_adj % cache_size == invSrcPageNum)
                cache_replacement_for_inv_adj++, r_skip_the_page++;
            cache_inv_adj.first[cache_replacement_for_inv_adj % cache_size] = invDstPageNum[dst];
            cache_inv_adj.second[cache_replacement_for_inv_adj++ % cache_size].assign(page_for_adj[invDstPageNum[dst]].begin(), page_for_adj[invDstPageNum[dst]].end());
            //print the cache data
            //prt_cache_inv_adj_data();
            break;
        case 2: //dst in cache, but not src
            //cout << "The dst of current edge is in the cache_inv_adj" << endl \
            //    << "Do cache_inv_adj replacement! (with only one page)" << endl;
            //cout << "The page number of src: " << invSrcPageNum << ", and the page number of dst: " << invDstPageNum[dst] << endl;
            if (cache_replacement_for_inv_adj % cache_size == invDstPageNum[dst]) 
                cache_replacement_for_inv_adj++, r_skip_the_page++;
            cache_inv_adj.first[cache_replacement_for_inv_adj % cache_size] = invSrcPageNum;
            cache_inv_adj.second[cache_replacement_for_inv_adj++ % cache_size].assign(page_for_adj[invSrcPageNum].begin(), page_for_adj[invSrcPageNum].end());
            //print the cache data
            //prt_cache_inv_adj_data();
            break;
        case 3: //otherwise, update the edge from memory to cache_inv_adj
            if (srcPageNum[src] == invSrcPageNum) {
                //cout << "Find the edge in memory but not in cache_inv_adj" << endl \
                //    << "Do cache_inv_adj replacement! (with only one page)" << endl;
                //cout << "The page number of src: " << invSrcPageNum << ", and the page number of dst: " << invDstPageNum[dst] << endl;
                //using cache policy or not
                //FIFO
                cache_inv_adj.first[cache_replacement_for_inv_adj % cache_size] = invSrcPageNum;
                cache_inv_adj.second[cache_replacement_for_inv_adj++ % cache_size].assign(page_for_adj[invSrcPageNum].begin(), page_for_adj[invSrcPageNum].end());
                //cout << "After the cache replacement, current cache_inv_adj data: " << endl;
                //print the cache data
                //prt_cache_inv_adj_data();
            } else {
                //cout << "Find the edge in memory but not in cache_inv_adj" << endl \
                //    << "Do cache_inv_adj replacement! (with two pages)" << endl;
                //cout << "The page number of src: " << invSrcPageNum << ", and the page number of dst: " << invDstPageNum[dst] << endl;
                //using cache policy or not
                //FIFO
                cache_inv_adj.first[cache_replacement_for_inv_adj % cache_size] = invSrcPageNum;
                cache_inv_adj.second[cache_replacement_for_inv_adj++ % cache_size].assign(page_for_adj[invSrcPageNum].begin(), page_for_adj[invSrcPageNum].end());
                cache_inv_adj.first[cache_replacement_for_inv_adj % cache_size] = invDstPageNum[dst];
                cache_inv_adj.second[cache_replacement_for_inv_adj++ % cache_size].assign(page_for_adj[invDstPageNum[dst]].begin(), page_for_adj[invDstPageNum[dst]].end());
                //cout << "After the cache replacement, current cache_inv_adj data: " << endl;
                //print the cache data
                //prt_cache_inv_adj_data();
            }
        default:
            break;
    }
}

/*
pair<pair<unsigned long, unsigned long>, pair<unsigned long, unsigned long>> findAdjPageOffset(unsigned long src, unsigned long dst, unsigned long find_src) {
    if (page_for_adj[find_src / page_size][find_src % page_size] == src) {
        pair<unsigned long, unsigned long> src_offset(find_src / page_size, find_src % page_size);
        for (unsigned long i = 0; i < ++find_src; i++) 
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
    unordered_map<unsigned long,  list<pair<unsigned long, unsigned long>>::iterator> m_map; //m_map_iter->first: key, m_map_iter->second: list iterator;
    list<pair<unsigned long, unsigned long>> m_list;                               //m_list_iter->first: key, m_list_iter->second: value;
public:
    LRUCache(size_t capacity):m_capacity(capacity) {
    }
    unsigned long get(unsigned long key) {
        auto found_iter = m_map.find(key);
        if (found_iter == m_map.end()) //key doesn't exist
            return -1;
        m_list.splice(m_list.begin(), m_list, found_iter->second); //move the node corresponding to key to front
        return found_iter->second->second;                         //return value of the node
    }
    void set(unsigned long key, unsigned long value) {
        auto found_iter = m_map.find(key);
        if (found_iter != m_map.end()) //key exists
        {
            m_list.splice(m_list.begin(), m_list, found_iter->second); //move the node corresponding to key to front
            found_iter->second->second = value;                        //update value of the node
            return;
        }
        if (m_map.size() == m_capacity) //reached capacity
        {
           unsigned long key_to_del = m_list.back().first; 
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
    unsigned long num_of_lines, num_of_po;
    //int swap_times;


    srand( time(NULL) );

    //Input the edge account and create the page size and the cache size
    cout << "Input the node amount: ";
    cin >> node_num;
    cout << "Input the edge amount: ";
    cin >> edge_num;
    //How many edges in one page
    cout << "Input the page size: ";
    cin >> page_size;

    vector<vector<unsigned long>> gnr_edge(node_num, vector<unsigned long>(node_num));
    //Randonly generate the edge with edge type 1 such like (src, dst)
    unsigned long page_amount = ceil((double)edge_num * 2 / page_size);

    cout << "Total page amounts: " << page_amount << endl;

    cout << "Input the cache size: ";
    cin >> cache_amount;
    
    page_for_rand = vector<vector<unsigned long>>(page_amount, vector<unsigned long>(page_size));
    page_for_adj = vector<vector<unsigned long>>(page_amount, vector<unsigned long>(page_size));
    page_for_inv_adj = vector<vector<unsigned long>>(page_amount, vector<unsigned long>(page_size));
    cache_rand = vector<vector<unsigned long>>(cache_amount, vector<unsigned long>(page_size));
    cache_adj.first = vector<unsigned long>(cache_amount, INT_MAX);
    cache_adj.second = vector<vector<unsigned long>>(cache_amount, vector<unsigned long>(page_size));
    cache_inv_adj.first = vector<unsigned long>(cache_amount, INT_MAX);
    cache_inv_adj.second = vector<vector<unsigned long>>(cache_amount, vector<unsigned long>(page_size));

    for (unsigned long i = 0; i < edge_num; i++) {
        unsigned long src = rand() % node_num, dst = rand() % node_num;
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
    for (unsigned long i = 0; i < page_amount; i++) {
        cout << "p" << i << ": " << endl;
        for (unsigned long j = 0; j < page_size; j += 2)
            cout << "src: " << page_for_rand[i][j] << " dst: " << page_for_rand[i][j + 1] << endl;
    }
    cout << endl;

    //Also, generate the edge with edge type 2 such like adj list
    //vector<vector<unsigned long>> adjLists(node_num);
    Graph graph(node_num);

    for (unsigned long i = 0; i < node_num; i++) {
        for (unsigned long j = 0; j < node_num; j++) {
            if (gnr_edge[i][j] == 1)
                graph.addEdge(i, j);
            if (gnr_edge[j][i] == 1)
                graph.addEdge_inv(j, i);
        }
    }

    graph.StorePageAdjMethod(page_amount);

    graph.StorePageinvAdjMethod(page_amount);

    graph.prt_adjacencylist();

    cout << "Page with adj list method storage: " << endl;
    for (unsigned long i = 0; i < page_amount; i++) {
        cout << "p" << i << ": " << endl;
        for (unsigned long j = 0; j < page_size; j++)
            cout << page_for_adj[i][j] << " ";
        cout << endl;
    }
    cout << endl;

    graph.prt_inv_adjacencylist();

    cout << "Page with inv adj list method storage: " << endl;
    for (unsigned long i = 0; i < page_amount; i++) {
        cout << "p" << i << ": " << endl;
        for (unsigned long j = 0; j < page_size; j++)
            cout << page_for_inv_adj[i][j] << " ";
        cout << endl;
    }
    cout << endl;

    cout << "Input the algorithm u want: (the default is pageRank) ";
    cin >> algo;

    switch (algo) {
    case BFS:
        graph.bfs(0, cache_amount);
        break;
    
    default:
        //read the number of power iterations(num_of_po) from cin
        cout << "Input number of pagerank iterations: ";
        cin >> num_of_po;
        graph.pageRank(num_of_po, cache_amount);
        graph.prt_pageRank();
        break;
    }

    for (unsigned long i = 0; i < skip_the_page; i++)
        cache_replacement_for_adj--;
    for (unsigned long i = 0; i < r_skip_the_page; i++)
        cache_replacement_for_inv_adj--;
    cout << "Cache replacement times with randomly store the edge sequence: " << cache_replacement_for_random << endl;
    cout << "Cache replacement times with Adjacency list store the edge sequence: " << cache_replacement_for_adj << endl;
    cout << "Cache replacement times with Inverse adjacency list store the edge sequence: " << cache_replacement_for_inv_adj << endl;

    return 0;
}