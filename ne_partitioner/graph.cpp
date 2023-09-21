#include "graph.hpp"

void graph_t::build(const std::vector<edge_t> &edges)
{
    //可能存在孤立节点，在有边的时候才创建
    if (edges.size() > nedges)
        neighbors = (uint40_t *)realloc(neighbors, sizeof(uint40_t) * edges.size());
    CHECK(neighbors) << "allocation failed";
    nedges = edges.size();

    std::vector<size_t> count(num_vertices, 0);
    for (size_t i = 0; i < nedges; i++)
        count[edges[i].first]++;

    vdata[0] = adjlist_t(neighbors);
    for (vid_t v = 1; v < num_vertices; v++) {
        count[v] += count[v-1];
        vdata[v] = adjlist_t(neighbors + count[v-1]);
    }
    for (size_t i = 0; i < edges.size(); i++) {
        //changed
        StorePageAdjMethod(edges[i].first, edges[i].second);
        vdata[edges[i].first].push_back(i);
    }
}

void graph_t::build_reverse(const std::vector<edge_t> &edges)
{
    if (edges.size() > nedges)
        neighbors = (uint40_t *)realloc(neighbors, sizeof(uint40_t) * edges.size());
    CHECK(neighbors) << "allocation failed";
    nedges = edges.size();

    std::vector<size_t> count(num_vertices, 0);
    for (size_t i = 0; i < nedges; i++)
        count[edges[i].second]++;

    vdata[0] = adjlist_t(neighbors);
    for (vid_t v = 1; v < num_vertices; v++) {
        count[v] += count[v-1];
        vdata[v] = adjlist_t(neighbors + count[v-1]);
    }
    for (size_t i = 0; i < edges.size(); i++) {
        //changed
        StorePageAdjMethod(edges[i].second, edges[i].first);
        vdata[edges[i].second].push_back(i);
    }
}

//changed
void graph_t::StorePageAdjMethod(vid_t src, vid_t dst) {
    flag_in_memory = 0;
    if (lru_pointer > 1024)
        for (size_t index: lru_for_working_memory)
            index -= 1024;
    //check the page is in the working memory or not
    for (auto it = working_memory.begin(); it < working_memory.end(); it++) 
        if (it->first == src) { 
            flag_in_memory = 1;
            //std::cout << "C" << std::endl;
        }   
    if (!flag_in_memory) {
        //check the map whether the page is exsited or not
        if (src_page_num_map.find(src) != src_page_num_map.end()) {
            //std::cout << "a" << std::endl;
            for (int j = 0; j < src_page_num_map[src].size(); j++) {
                //index of max element
                //std::cout << "b" << std::endl;
                uint32_t min = min_element(lru_for_working_memory.begin(), lru_for_working_memory.end()) - lru_for_working_memory.begin();
                std::cout << "Minimum LRU index: " << min << std::endl;
                working_memory[min] = std::make_pair(src, j);
                lru_for_working_memory[min] = ++lru_pointer;
                Trace_read();
            }
        } else {
            // create new page, update the map
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            src_page_num_map[src].push_back(page_num_cnt++);
            Trace_write();
        }
    } else {
        //std::cout << "Input the edge" << std::endl;
        //std::cout << src_page_num_map[src].size() << std::endl;
        if (src_page_num_map[src].size() == 0) {
            //create new page, update the map
            //std::cout << "huzzah" << std::endl;
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            //std::cout << "finnnnishQ" << std::endl;
            src_page_num_map[src].push_back(page_num_cnt++);
            //std::cout << "finnnnishQ" << std::endl;
            Trace_write();
        } else if (page_for_adj[src_page_num_map[src].back()].size() == page_size) {
            //std::cout << "Poggg" << std::endl;
            //create new page, update the map
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            src_page_num_map[src].push_back(page_num_cnt++);
            Trace_write();
        } else {
            //input the edge in the last page
            //std::cout << "Ayaya" << std::endl;
            page_for_adj[src_page_num_map[src].back()].push_back(dst);
        }
    }
    //std::cout << "Working memory state: " << 
}

std::vector<size_t> graph_t::getOutDegree() {
    std::vector<size_t> degree(num_vertices , 0);
    for (auto it = 0; it < num_vertices; it++)
        degree[it] = vdata[it].size();
    return degree;
}

std::vector<size_t> graph_t::getInDegree() {
    std::vector<size_t> degree(num_vertices , 0);
    for (int it = 0; it < num_vertices; it++)
        degree[it] = vdata[it].size();
    return degree;
}