#include "pre_processing.hpp"

PreprocessingPartitioner::PreprocessingPartitioner(string basefilename, string method, int pnum, int memsize)
{
    //changed
    page_edge_cnt = mem.trace_write_cnt;
    p = pnum;
    set_write_files(basefilename, method, pnum);    
    partition_fout.open(pagesave_name(basefilename, method, pnum));

    total_time.start();
    //edge file
    fin.open(binedgelist_name(basefilename),std::ios::binary | std::ios::ate);
    filesize = fin.tellg();
    fin.seekg(0, std::ios::beg);

    fin.read((char *)&num_vertices, sizeof(num_vertices));
    fin.read((char *)&num_edges, sizeof(num_edges));
    //degree file
    degrees.resize(num_vertices);
    std::ifstream degree_file(degree_name(basefilename), std::ios::binary);
    degree_file.read((char *)&degrees[0], num_vertices * sizeof(vid_t));
    degree_file.close();

    fill(mem.vid_for_page_range.begin(), mem.vid_for_page_range.end(), vector<vid_t>());
    // LOG(INFO) << "constructor current time: " << total_time.get_time();
    num_batches = (filesize / ((std::size_t)memsize * mem.cache_buffer)) + 1;
    num_edges_per_batch = (num_edges / num_batches) + 1;
    page_set.assign(1, set<vid_t>());
    mem.LPN_Boundary_vertices_set_map.assign(1, set<vid_t>());
}

void PreprocessingPartitioner::resize_mapping() { // page number -> related pages for partitioning
    // LOG(INFO) << "resize mapping time: " << total_time.get_time();
    // LOG(INFO) << "mem.page_num: " << mem.page_num;
    // LOG(INFO) << "mem.LPN_Boundary_vertices_set_map.size(): " << mem.LPN_Boundary_vertices_set_map.size();
    mem.related_pages_map.assign(mem.page_num, set<uint32_t>());
    set<uint32_t> tmp_related_pages;
    for (uint32_t v_i = 0; v_i < num_vertices; v_i++) { 
        for (uint32_t page_i = 0; page_i < mem.page_num; page_i++) {
            if (*mem.LPN_Boundary_vertices_set_map[page_i].begin() == v_i) {
                // LOG(INFO) << "v_i: " << v_i;
                // LOG(INFO) << "page_i: " << page_i;
                tmp_related_pages.emplace(page_i);
            }
        }
        for (set<uint32_t>::iterator it = tmp_related_pages.begin(); it != tmp_related_pages.end(); it++) {
            mem.related_pages_map.at(*it).insert(tmp_related_pages.begin(), tmp_related_pages.end());
        }
    }
    // LOG(INFO) << "resize mapping finish time: " << total_time.get_time();
}

void PreprocessingPartitioner::batch_read() {
    least_pages_per_partition = mem.page_num / p;
    int remainder = mem.page_num % p;
    rep (i, remainder) {
        occupied[i] = least_pages_per_partition + 1;
    }
    int min_related_page_size = INT32_MAX;
    int min_page_num = -1;
    partition.resize(p, vector<uint32_t>());
    rep (i, mem.page_num) { //  the first time initialize
        remaining_pages.emplace(i);
        if (mem.related_pages_map[i].size() < min_related_page_size) {
            min_related_page_size = mem.related_pages_map[i].size();
            min_page_num = i;
        }
    }
    rep (part, p) {
        int pages_for_forming_partition = 0;
        bool tsudzuku = true;
        partition_fout << min_page_num << " " << part << endl;
        partition[part].emplace_back(min_page_num);
        remaining_pages.erase(min_page_num);
        //do the partitioning
        for (set<uint32_t>::iterator it = mem.related_pages_map[min_page_num].begin(); it != mem.related_pages_map[min_page_num].end(); it++) {
            partition_fout << *it << " " << part << endl;
            partition[part].emplace_back(*it);
            remaining_pages.erase(*it);
            if (++pages_for_forming_partition == least_pages_per_partition) {
                remainder--;
                if (!remainder) {
                    least_pages_per_partition--;
                    remainder--;
                }
                tsudzuku = false;
                break;
            }
        }
        //if the partitioning is not done, insert the related page of the first related page of initial page
        if (tsudzuku) {
            for (set<uint32_t>::iterator it = mem.related_pages_map[min_page_num].begin(); it != mem.related_pages_map[min_page_num].end(); it++) {
                for (set<uint32_t>::iterator it_2 = mem.related_pages_map[*it].begin(); it_2 != mem.related_pages_map[*it].end(); it_2++) {
                    partition_fout << *it_2 << " " << part << endl;
                    partition[part].emplace_back(*it_2);
                    remaining_pages.erase(*it_2);
                    if (++pages_for_forming_partition == least_pages_per_partition) {
                        remainder--;
                        if (!remainder) {
                            least_pages_per_partition--;
                            remainder--;
                        }
                        tsudzuku = true;
                        break;
                    }
                }
                if (tsudzuku)
                    break;
            }
        }
        //insert the new relation of the pages in partition, simultaneously delete the relation between the pages that inside and outside of partition
        int iter = 0;
        partition[part].emplace_back(UINT32_MAX);
        rep (i, mem.page_num) {
            if (partition[part][iter] == i) {
                mem.related_pages_map[i].clear();
                rep (j, least_pages_per_partition) {
                    mem.related_pages_map[i].insert(partition[part][j]);
                }
                iter++;
            } else {
                rep (j, least_pages_per_partition) {
                    mem.related_pages_map[i].erase(partition[part][j]);
                }
            }
        }
        partition[part].pop_back();
        min_page_num = -1;  // the rest of initialization
        for (set<vid_t>::iterator re = remaining_pages.begin(); re != remaining_pages.end(); re++) { 
            if (mem.related_pages_map[*re].size() < min_related_page_size) {
                min_related_page_size = mem.related_pages_map[*re].size();
                min_page_num = *re;
            }
        }
    }
}

void PreprocessingPartitioner::batch_write(string opt_name){
    fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);
    std::vector<edge_t> edges;
    auto num_edges_left = num_edges;
    // LOG(INFO) << "Batch write current time: " << total_time.get_time();

    for (uint32_t i = 0; i < num_batches; i++) {
        auto edges_per_batch = num_edges_per_batch < num_edges_left ? num_edges_per_batch : num_edges_left;
        //changed
        // uint32_t page_amt = num_edges_per_batch < num_edges_left ? 1024 * 1024 : num_edges_left * 2 * 8 / page_size; //2 vertices for 1 edge, 8 bytes for i long unsigned int
        // for (i = 0; i < page_amt; i++) {
        //     //1M pages equal to 1 batch
        //     Trace_read();
        // }
        edges.resize(edges_per_batch);
        fin.read((char *) &edges[0], sizeof(edge_t) * edges_per_batch);
        part_degrees.assign(num_vertices, 0);
        AdjList.assign(num_vertices, set<vid_t>());
        // LOG(INFO) << "current time: " << total_time.get_time();
        LOG(INFO) << "edges.size(): " << edges.size();
        //create partial adj list
        for(auto &e : edges) {
            addEdge(e.first, e.second);
            //LOG(INFO) << "e.first: " << e.first << ", e.second: " << e.second;
        }
        //prt_adjacencylist();
        if (opt_name == "polon") {
            batch_DFS(i);
        } else {
            LOG(ERROR) << "no valid opt function";
        }
        num_edges_left -= edges_per_batch;
    }
}

void PreprocessingPartitioner::batch_DFS(uint32_t batches) {
    color = new vid_t[num_vertices];           // 配置記憶體位置
    discover = new vid_t[num_vertices];
    finish = new vid_t[num_vertices];
    predecessor = new vid_t[num_vertices];
    uint32_t tmp_index_for_page_buffer = 0; // index of set number for tmp
    vector<set<vid_t>> tmp_page_buffer;

    // LOG(INFO) << "Batch DFS current time: " << total_time.get_time();
    int time = 0;                          // 初始化
    for (vid_t i = 0; i < num_vertices; i++) { 
        color[i] = 0;
        discover[i] = 0;
        finish[i] = 0;
        predecessor[i] = -1;
    }

    vid_t minPosition, minni = UINT32_MAX;
    for (vid_t pos = 0; pos < num_vertices; pos++) {
        if (!part_degrees[pos]) {
            continue;
        } else if (part_degrees[pos] < minni) {
            minPosition = pos;
            minni = part_degrees[pos];
        }
    }

    bool flag = 0;
    
    while (!flag) { // use flag to inspect all edges in AdjList is cleared
        int cnt_for_vectices_size = 0;
        for (int k = 0; k < AdjList.size() - 1; k++) { // 檢查所有Adj list中的edges all clear
            if (!AdjList[k].empty())
                break;
            else
                cnt_for_vectices_size++;
        }
        if (cnt_for_vectices_size == AdjList.size() - 1) {
            LOG(INFO) << "OWARI";
            flag = 1;
        }
        else {
            // LOG(INFO) << "minPosition: " << minPosition;
            if (color[minPosition] == 0) {               // 若vertex不是白色, 則進行以該vertex作為起點之搜尋
                DFSVisit(minPosition, time);
            }
            // LOG(INFO) << "page_num: " << mem.page_num;
            // LOG(INFO) << "cnt_for_visit_time: " << cnt_for_visit_time;
            // LOG(INFO) << "page_edge_cnt: " << page_edge_cnt;
            if (cnt_for_visit_time != page_edge_cnt) { // page is not full, stored into the page_buffer
                //LOG(INFO) << "Store into page buffer";
                mem.page_buffer.emplace_back(set<vid_t>());
                if (cnt_for_visit_time + mem.cnt_for_page_buffer < mem.page_buffer_size) {
                    mem.page_buffer[mem.index_for_page_buffer++] = boundary_vertices_set;
                    boundary_vertices_set.clear();
                    mem.cnt_for_edges_in_page_buffer.push_back(cnt_for_visit_time);
                    mem.cnt_for_page_buffer += cnt_for_visit_time;
                    //LOG(INFO) << "2";
                } else { // flushed the page_buffer into pages, boundary set compare with each other
                    //LOG(INFO) << "3";
                    mem.cnt_for_page_buffer = 0;
                    for (vid_t v_i = 0; v_i < num_vertices; v_i++) { // accumulate the related sets and sorted in descending order of vid, then flush
                        for (uint32_t index_i = 0; index_i < mem.index_for_page_buffer; index_i++) { // travese sets
                            if (*mem.page_buffer[index_i].begin() == v_i) {
                                tmp_page_buffer.emplace_back(set<vid_t>());
                                tmp_page_buffer[tmp_index_for_page_buffer++] = mem.page_buffer[index_i];
                            }
                        }
                    }
                    //LOG(INFO) << "4";
                    mem.index_for_page_buffer = 0;
                    mem.page_buffer.clear();
                    //merge all the set in a size of page for all page buffer
                    for (uint32_t set_tmp_index = 0; set_tmp_index < tmp_page_buffer.size(); set_tmp_index++) {
                        if (mem.cnt_for_page_buffer_combine >= mem.page_size) {
                            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
                            boundary_vertices_set.clear();
                            page_set.emplace_back(set<vid_t>());
                            mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
                            mem.cnt_for_page_buffer_combine = 0;
                        }
                        boundary_vertices_set.insert(tmp_page_buffer[set_tmp_index].begin(), tmp_page_buffer[set_tmp_index].end());
                        mem.cnt_for_page_buffer_combine += mem.cnt_for_edges_in_page_buffer[set_tmp_index];
                    }
                    //LOG(INFO) << "5";
                    // place the redundant set into page buffer for next time the page buffer filled up
                    mem.page_buffer.emplace_back(set<vid_t>());
                    mem.page_buffer[mem.index_for_page_buffer++] = boundary_vertices_set;
                    boundary_vertices_set.clear();
                    mem.cnt_for_page_buffer = mem.cnt_for_page_buffer_combine;
                    mem.cnt_for_page_buffer_combine = 0;
                }
            } else {
                mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set; // place the page
                boundary_vertices_set.clear();
                page_set.emplace_back(set<vid_t>());
                mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
            }
            minni = UINT32_MAX;
            for (vid_t pos = 0; pos < num_vertices; pos++) {
                if (!part_degrees[pos]) {
                    continue;
                } else if (part_degrees[pos] < minni) {
                    minPosition = pos;
                    minni = part_degrees[pos];
                }
            }
            // LOG(INFO) << "minni: " << minni;
            cnt_for_visit_time = 0;
            int time = 0;
            for (vid_t i = 0; i < num_vertices; i++) { 
                color[i] = 0;
                discover[i] = 0;
                finish[i] = 0;
                predecessor[i] = -1;
            }
        }
    }
    mem.cnt_for_page_buffer = 0;
    // accumulate the related sets and sorted in descending order of vid, then flush
    for (vid_t v_i = 0; v_i < num_vertices; v_i++) { 
        for (uint32_t index_i = 0; index_i < mem.index_for_page_buffer; index_i++) { // travese sets
            if (*mem.page_buffer[index_i].begin() == v_i) {
                tmp_page_buffer.emplace_back(set<vid_t>());
                tmp_page_buffer[tmp_index_for_page_buffer++] = mem.page_buffer[index_i];
            }
        }
    }
    mem.index_for_page_buffer = 0;
    mem.page_buffer.clear();
    //merge all the set in a size of page for all page buffer
    for (uint32_t set_tmp_index = 0; set_tmp_index < tmp_page_buffer.size(); set_tmp_index++) {
        if (mem.cnt_for_page_buffer_combine >= mem.page_size) {
            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
            boundary_vertices_set.clear();
            page_set.emplace_back(set<vid_t>());
            mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
            mem.cnt_for_page_buffer_combine = 0;
        }
        boundary_vertices_set.insert(tmp_page_buffer[set_tmp_index].begin(), tmp_page_buffer[set_tmp_index].end());
        mem.cnt_for_page_buffer_combine += mem.cnt_for_edges_in_page_buffer[set_tmp_index];
    }
    // place the redundant set into last page
    mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
    boundary_vertices_set.clear();
    page_set.emplace_back(set<vid_t>());
    mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
}

void PreprocessingPartitioner::addEdge(vid_t s, vid_t d) {
    if (AdjList[s].find(d) != AdjList[s].end()) 
        return;
    AdjList[s].emplace(d);
    AdjList[d].emplace(s);
    part_degrees[s]++, part_degrees[d]++;
}

void PreprocessingPartitioner::prt_adjacencylist() {
    cout << "Adjacency list: " << endl;
    for (uint32_t i = 0; i < AdjList.size(); i++) {
        cout << i << "->";
        for (vid_t x : AdjList[i]) {
            cout << x << " ";
        }
        cout << endl;
    }
    cout << endl;
}

void PreprocessingPartitioner::DFSVisit(vid_t vertex, int &time) {   // 一旦有vertex被發現而且是白色, 便進入DFSVisit()
    //LOG(INFO) << "DFS Visit current time: " << total_time.get_time();
    if (cnt_for_visit_time == page_edge_cnt) {
        boundary_vertices_set.insert(vertex);
        return;
    }
    color[vertex] = 1;                         // 把vertex塗成灰色
    discover[vertex] = ++time;                 // 更新vertex的discover時間
    for (set<vid_t>::iterator itr = AdjList[vertex].begin();   // for loop參數太長
        itr != AdjList[vertex].end();) {                       // 分成兩段
        // LOG(INFO) << "DFS Visit start";
        // LOG(INFO) << "*itr: " << *itr;
        // LOG(INFO) << "vertex: " << vertex;
        if (color[*itr] == 0) {                // 若搜尋到白色的vertex
            //LOG(INFO) << "DFS Visit new v";
            predecessor[*itr] = vertex;        // 更新其predecessor
            page_set.at(mem.page_num).insert(vertex);                     // for total replication factor
            page_set.at(mem.page_num).insert(*itr);                       // for total replication factor
            part_degrees[vertex]--, part_degrees[*itr]--;
            vid_t next_vertex = *itr;
            AdjList[*itr].erase(vertex);       // delete the edges in both direction
            itr = AdjList[vertex].erase(itr);        // erase the edge in adj list, changed
            cnt_for_visit_time++;
            DFSVisit(next_vertex, time);              // 立刻以其作為新的搜尋起點, 進入新的DFSVisit()
            itr = AdjList[vertex].begin();
            if (cnt_for_visit_time == page_edge_cnt)
                return;
            //LOG(INFO) << "DFS Visit finish";
        } else {
            //LOG(INFO) << "DFS Visit else";
            part_degrees[vertex]--, part_degrees[*itr]--;
            AdjList[*itr].erase(vertex);       // delete the edges in both direction
            itr = AdjList[vertex].erase(itr);        // erase the edge in adj list, changed
            cnt_for_visit_time++;
            if (cnt_for_visit_time == page_edge_cnt) {
                boundary_vertices_set.insert(vertex);
                return;
            }
        }
    }
    color[vertex] = 2;                         // 當vertex已經搜尋過所有與之相連的vertex後, 將其塗黑
    finish[vertex] = ++time;                   // 並更新finish時間
    if (finish[vertex] - discover[vertex] == 1) {
        boundary_vertices_set.insert(vertex);
    }
}

size_t PreprocessingPartitioner::count_mirrors() {
    size_t result = 0;
    vector<set<vid_t>> partition_set(p, set<vid_t>());
    rep (i, p) 
        rep (j, occupied[i]) 
            partition_set[i].insert(page_set[partition[i][j]].begin(), page_set[partition[i][j]].end());
    rep (i, p) {
        LOG(INFO) << "number of vertices in partition" << i << ": " << partition_set[i].size();
        result += partition_set[i].size();
    }
    return result;
}

void PreprocessingPartitioner::split()
{   
    batch_write("polon");
    resize_mapping();
    batch_read();

    node_fout.close();
    edge_fout.close();
    partition_fout.close();
    total_time.stop();
    
    LOG(INFO) << "total partition time: " << total_time.get_time();
    LOG(INFO) << "expected edges in each partition: " << num_edges / p;
    LOG(INFO) << "expected pages in each partition: " << mem.page_num / p;
    rep (i, p)
        LOG(INFO) << "edges in partition " << i << ": " << occupied[i] * page_edge_cnt;
    rep (i, p)
        LOG(INFO) << "pages in partition " << i << ": " << occupied[i];
    size_t max_occupied = *std::max_element(occupied.begin(), occupied.end());
    LOG(INFO) << "balance: " << (double)max_occupied / ((double)num_edges / p);
    size_t total_mirrors = count_mirrors();
    LOG(INFO) << "total mirrors: " << total_mirrors;
    LOG(INFO) << "replication factor: " << (double)total_mirrors / num_vertices;
}