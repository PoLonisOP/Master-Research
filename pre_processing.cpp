#include "pre_processing.hpp"

PreprocessingPartitioner::PreprocessingPartitioner(string basefilename, string method, uint32_t pnum, int memsize)
{
    //changed
    page_edge_cnt = mem.trace_write_cnt;
    p = pnum;
    set_write_files(basefilename, method, pnum);
    mem.Set_trace_files();
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

    // LOG(INFO) << "constructor current time: " << total_time.get_time();
    num_batches = (filesize / ((std::size_t)memsize * mem.cache_buffer)) + 1;
    num_edges_per_batch = (num_edges / num_batches) + 1;
    page_set.assign(1, set<vid_t>());
    mem.LPN_Boundary_vertices_set_map.assign(1, set<vid_t>());
    mem.vertices_LPN_map.assign(num_vertices, set<uint32_t>());

    LOG(INFO) << "num_vertices: " << num_vertices;
    LOG(INFO) << "num_edges: " << num_edges;
}

void PreprocessingPartitioner::resize_mapping() { // page number -> related pages for partitioning
    // LOG(INFO) << "resize mapping time: " << total_time.get_time();
    // LOG(INFO) << "mem.page_num: " << mem.page_num;
    // LOG(INFO) << "mem.LPN_Boundary_vertices_set_map.size(): " << mem.LPN_Boundary_vertices_set_map.size();
    mem.related_pages_map.resize(mem.page_num, set<uint32_t>());
    for (uint32_t page_i = 0; page_i < mem.page_num; page_i++) {
        for (set<vid_t>::iterator related_v = mem.LPN_Boundary_vertices_set_map.at(page_i).begin(); 
            related_v != mem.LPN_Boundary_vertices_set_map.at(page_i).end(); related_v++) {
            // LOG(INFO) << "*related_v: " << *related_v;
            mem.related_pages_map.at(page_i).insert(mem.vertices_LPN_map.at(*related_v).begin(), mem.vertices_LPN_map.at(*related_v).end());
        }
        // LOG(INFO) << "page_i: " << page_i;
        mem.related_pages_map.at(page_i).erase(page_i);
    }
    // LOG(INFO) << "finished";
    mem.vertices_LPN_map.clear();
    mem.LPN_Boundary_vertices_set_map.clear();
    // LOG(INFO) << "resize mapping finish time: " << total_time.get_time();
}

void PreprocessingPartitioner::BFS_partition(vid_t min_index, uint32_t pages, uint32_t part) {
    vector<uint32_t> color(num_vertices, 0);
    queue<uint32_t> que;
    uint32_t pages_for_forming_partition = 0;
    for (vid_t p_i = 0; p_i < mem.page_num; p_i++) {
        if (!color.at(min_index)) {
            color.at(min_index) = 1;
            que.push(min_index);
            partition_fout << min_index << " " << part << endl;
            partition[part].emplace_back(min_index);
            remaining_pages.erase(min_index);
            if (++pages_for_forming_partition == pages) {
                return;
            }
            while (!que.empty()) {
                uint32_t u = que.front();
                for (set<uint32_t>::iterator it = mem.related_pages_map[u].begin(); 
                    it != mem.related_pages_map[u].end(); it++) {
                    if (!color.at(*it)) {
                        color.at(*it) = 1;
                        que.push(*it);
                        partition_fout << *it << " " << part << endl;
                        partition[part].emplace_back(*it);
                        remaining_pages.erase(*it);
                        if (++pages_for_forming_partition == pages) {
                            return;
                        }
                    }
                }
                que.pop();
                color.at(u) = 2;
            }
        }
        min_index = p_i;
    }
}

void PreprocessingPartitioner::batch_read() {
    // LOG(INFO) << "0";
    uint32_t most_pages_per_partition = (mem.page_num / p) + 1;
    uint32_t remainder = mem.page_num % p;
    for (uint32_t i = 0; i < p; i++) {
        // LOG(INFO) << "remainder: " << remainder;
        if (remainder) {
            occupied.emplace_back(most_pages_per_partition);
            // LOG(INFO) << "occupied[i]: " << occupied[i];
            remainder--;
        } else {
            occupied.emplace_back(most_pages_per_partition - 1);
            // LOG(INFO) << "occupied[i]: " << occupied[i];
        }
    }
    // LOG(INFO) << "1";
    uint32_t none_related_pages = 0;
    int min_related_page_size = INT32_MAX;
    int min_page_num = -1;
    partition.assign(p, vector<uint32_t>());
    for (uint32_t i = 0; i < mem.page_num; i++) { // the first time initialize
        remaining_pages.insert(i);
        if (!mem.related_pages_map[i].size()) {
            partition_fout << i << " " << 0 << endl;
            partition[0].emplace_back(i);
            mem.Trace_R();
            remaining_pages.erase(i);
            none_related_pages++;
        } else if (mem.related_pages_map[i].size() < min_related_page_size) {
            min_related_page_size = mem.related_pages_map[i].size();
            min_page_num = i;
            // LOG(INFO) << "mem.related_pages_map[i].size(): " << mem.related_pages_map[i].size();
            // LOG(INFO) << "min_page_num: " << min_page_num;
        }
    }
    // LOG(INFO) << "3";
    for (uint32_t part = 0; part < p; part++) {
        // LOG(INFO) << "4";
        //do the partitioning
        BFS_partition(min_page_num, occupied[part] - none_related_pages, part);
        // insert the new relation of the pages in partition, simultaneously delete the relation 
        // between the pages that inside and outside of partition
        int iter = 0;
        partition[part].push_back(UINT32_MAX); // avoid iter overflow
        rep (i, mem.page_num) {
            if (partition[part][iter] == i) {
                mem.related_pages_map[i].clear();
                rep (j, occupied[part]) {
                    mem.related_pages_map[i].insert(partition[part][j]);
                }
                iter++;
            } else {
                rep (j, occupied[part]) {
                    mem.related_pages_map[i].erase(partition[part][j]);
                }
            }
        }
        partition[part].pop_back();
        // LOG(INFO) << "7";
        none_related_pages = 0;  // the rest of initialization
        min_related_page_size = INT32_MAX;
        // LOG(INFO) << "8";
        min_page_num = -1;
        // LOG(INFO) << "remaining_pages: " << remaining_pages.size();
        for (set<vid_t>::iterator re = remaining_pages.begin(); re != remaining_pages.end(); re++) {
            // LOG(INFO) << "iterator: ";
            if (!mem.related_pages_map[*re].size()) {
                // LOG(INFO) << "1";
                int next_part = part + 1;
                if (part == p - 1)
                    next_part = part;
                partition_fout << *re << " " << next_part << endl;
                partition[next_part].emplace_back(*re);
                mem.Trace_R();
                none_related_pages++;
                // LOG(INFO) << "2";
            } else if (mem.related_pages_map[*re].size() < min_related_page_size) {
                // LOG(INFO) << "3";
                min_related_page_size = mem.related_pages_map[*re].size();
                min_page_num = *re;
                // LOG(INFO) << "mem.related_pages_map[*re].size(): " << mem.related_pages_map[*re].size();
                // LOG(INFO) << "min_page_num: " << min_page_num;
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
            // LOG(INFO) << "OWARI";
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
                        if (mem.cnt_for_page_buffer_combine >= page_edge_cnt) {
                            for (set<uint32_t>::iterator v_id = boundary_vertices_set.begin(); v_id != boundary_vertices_set.end(); v_id++) {
                                mem.vertices_LPN_map.at(*v_id).emplace(mem.page_num);
                            }
                            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
                            mem.Trace_W();
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
                for (set<uint32_t>::iterator v_id = boundary_vertices_set.begin(); v_id != boundary_vertices_set.end(); v_id++) {
                    mem.vertices_LPN_map.at(*v_id).emplace(mem.page_num);
                }
                mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set; // place the page
                mem.Trace_W();
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
        if (mem.cnt_for_page_buffer_combine >= page_edge_cnt) {
            for (set<uint32_t>::iterator v_id = boundary_vertices_set.begin(); v_id != boundary_vertices_set.end(); v_id++) {
                mem.vertices_LPN_map.at(*v_id).emplace(mem.page_num);
            }
            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
            mem.Trace_W();
            boundary_vertices_set.clear();
            page_set.emplace_back(set<vid_t>());
            mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
            mem.cnt_for_page_buffer_combine = 0;
        }
        boundary_vertices_set.insert(tmp_page_buffer[set_tmp_index].begin(), tmp_page_buffer[set_tmp_index].end());
        mem.cnt_for_page_buffer_combine += mem.cnt_for_edges_in_page_buffer[set_tmp_index];
    }
    // place the redundant set into last page
    for (set<uint32_t>::iterator v_id = boundary_vertices_set.begin(); v_id != boundary_vertices_set.end(); v_id++) {
        mem.vertices_LPN_map.at(*v_id).emplace(mem.page_num);
    }
    mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
    mem.Trace_W();
    boundary_vertices_set.clear();
    page_set.emplace_back(set<vid_t>());
    mem.LPN_Boundary_vertices_set_map.emplace_back(set<vid_t>());
}

void PreprocessingPartitioner::addEdge(vid_t s, vid_t d) {
    if (AdjList[s].find(d) != AdjList[s].end()) {
        cycle_edges++;
        return;
    }
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
            AdjList[vertex].erase(itr);        // erase the edge in adj list, changed
            itr = AdjList[vertex].begin();
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
    // LOG(INFO) << "count mirrors";
    size_t result = 0;
    vector<set<vid_t>> partition_set(p, set<vid_t>());
    // LOG(INFO) << "1";
    for (uint32_t i = 0; i < p; i++) {
        // LOG(INFO) << "occupied[i]: " << occupied[i];
        rep (j, occupied[i]) {
            // LOG(INFO) << "1.1";
            // LOG(INFO) << "partition[" << i << "][" << j << "]: " << partition[i][j];
            partition_set[i].insert(page_set[partition[i][j]].begin(), page_set[partition[i][j]].end());
        }
    }
    // LOG(INFO) << "2";
    for (uint32_t i = 0; i < p; i++) {
        LOG(INFO) << "number of vertices in partition " << i << ": " << partition_set[i].size();
        result += partition_set[i].size();
    }
    // LOG(INFO) << "3";
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
    
    LOG(INFO) << "cycle edges: " << cycle_edges;
    LOG(INFO) << "total partition time: " << total_time.get_time();
    LOG(INFO) << "expected edges in each partition: " << (double)num_edges / p;
    LOG(INFO) << "expected pages in each partition: " << (double)mem.page_num / p;
    rep (i, p)
        LOG(INFO) << "edges in partition " << i << ": " << occupied[i] * page_edge_cnt;
    rep (i, p)
        LOG(INFO) << "pages in partition " << i << ": " << occupied[i];
    size_t max_occupied = *std::max_element(occupied.begin(), occupied.end());
    LOG(INFO) << "balance: " << ((double)max_occupied * (double)page_edge_cnt / (double)num_edges * p);
    size_t total_mirrors = count_mirrors();
    LOG(INFO) << "total mirrors: " << total_mirrors;
    LOG(INFO) << "replication factor: " << (double)total_mirrors / num_vertices;
    LOG(INFO) << "Read times in trace: " << mem.read_times;
    LOG(INFO) << "Write times in trace: " << mem.write_times;
}