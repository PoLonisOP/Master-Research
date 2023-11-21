#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <random>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <queue>

//#include "dense_bitset.hpp"
#include "util.hpp"
#include "partitioner.hpp"
#include "memory_for_pre.hpp"
#include <list>
#include <algorithm> 


using namespace std;

class PreprocessingPartitioner : public Partitioner
{
private:
    vid_t num_vertices;
    size_t num_edges;
    size_t filesize;
    uint32_t page_edge_cnt;
    uint32_t p;
    uint32_t least_pages_per_partition;
    ifstream fin;
    uint32_t num_batches;
    uint32_t num_edges_per_batch;
    uint32_t cycle_edges = 0;

    Memory_Pre mem;

    vector<vid_t> degrees;
    vector<vid_t> part_degrees;
    vector<uint32_t> occupied;
    vector<set<vid_t>> page_set;
    set<vid_t> boundary_vertices_set;
    set<vid_t> remaining_pages;
    vector<vector<uint32_t>> partition;

    vector<set<vid_t>> AdjList;
    vid_t *color,             // 0:white, 1:gray, 2:black
          *predecessor,
          *discover,
          *finish;

    ofstream partition_fout;

protected:
    void DFSVisit(vid_t, int &);
    void batch_write(string opt_name);
    void batch_DFS(uint32_t);
    void batch_node_assignment(vector<edge_t> &edges);
    void addEdge(vid_t, vid_t);
    void prt_adjacencylist();
    void resize_mapping();
    void batch_read();
    size_t count_mirrors();
    void BFS_partition(vid_t, uint32_t, uint32_t);
public:
    uint32_t cnt_for_visit_time = 0;
    uint32_t cnt_for_bvs = 0;

    PreprocessingPartitioner(string basefilename, string method, uint32_t pnum, int memsize);

    void split();
};
