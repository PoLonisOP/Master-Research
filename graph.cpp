#include "graph.hpp"

void graph_t::build(const std::vector<edge_t> &edges, Storage &mem)
{
    // changed
    mem.Set_trace_files();
    mem.Set_trace_files2();
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
    // changed
    // mem.CSR_vertex_map.resize(num_vertices + 1, 0);
    for (auto it = mem.ADJ_working_memory.begin(); it < mem.ADJ_working_memory.end(); it++)
        it->first == UINT32_MAX;
    
    for (size_t i = 0; i < edges.size(); i++) {
        // changed
        // mem.StorePageCsrMethod(edges[i].first, edges[i].second, i + 1);
        // changed
        mem.StorePageAdjMethod(edges[i].first, edges[i].second);
        vdata[edges[i].first].push_back(i);
    }
}

void graph_t::build_reverse(const std::vector<edge_t> &edges, Storage &mem)
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
    // changed
    // mem.CSR_vertex_map.resize(num_vertices + 1, 0);
    for (size_t i = 0; i < edges.size(); i++) {
        // changed
        // mem.StorePageCsrMethod(edges[i].first, edges[i].second, i + 1);
        // changed
        mem.StorePageAdjMethod(edges[i].second, edges[i].first);
        vdata[edges[i].second].push_back(i);
    }
    // mem.prt_CSR();
}