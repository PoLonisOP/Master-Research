#include <vector>
#include <parallel/algorithm>
//changed
#include <map>
#include <exception>
#include <iostream>
#include "memory.hpp"

class Storage : public Memory
{
public: 
    //changed
    bool flag_in_memory;
    bool full;
    size_t min;
    size_t target_page_num;
    size_t memory_index;
    size_t page_num_cnt = 0;
    size_t ADJ_lru_pointer = 0;
    size_t CSR_lru_pointer = 0;
    std::vector<vid_t> CSR_vertex_map; // the first one is always 0, then record the number of edges of every vertices
    std::vector<vid_t> CSR_edge_map; // destination vertex of edges
    std::vector<size_t> CSR_lru_for_working_memory = std::vector<size_t>(128, 0);
    std::vector<size_t> CSR_working_memory = std::vector<size_t>(128); // {page_num}
    std::vector<size_t> ADJ_lru_for_working_memory = std::vector<size_t>(128, 0);
    std::vector<std::pair<vid_t, size_t>> ADJ_working_memory = std::vector<std::pair<vid_t, size_t>>(128); // {src, page_num}
    std::vector<std::vector<vid_t>> page_for_adj; // {dst1, dst2...}

    //changed
    void StorePageAdjMethod(vid_t, vid_t);
    void StorePageCsrMethod(vid_t, vid_t, size_t);
    void prt_CSR();
    map<vid_t, std::vector<size_t>> src_page_num_map; //the first is src vertex, and the second is page number of edge of src vertex
};