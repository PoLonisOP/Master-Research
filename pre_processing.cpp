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
    LOG(INFO) << "Constructor current time: " << total_time.get_time();
    num_batches = (filesize / ((std::size_t)memsize * mem.cache_buffer)) + 1;
    num_edges_per_batch = (num_edges / num_batches) + 1;
}

void PreprocessingPartitioner::resize_mapping() { // page number -> related pages for partitioning
    set<uint32_t> tmp_related_pages;
    for (uint32_t v_i = 0; v_i < num_vertices; v_i++) { 
        for (uint32_t page_i = 0; page_i < mem.page_num; page_i++) {
            if (*mem.LPN_Boundary_vertices_set_map[page_i].begin() == v_i) {
                tmp_related_pages.insert(page_i);
            }
        }
        for (set<uint32_t>::iterator it = tmp_related_pages.begin(); it != tmp_related_pages.end(); it++) {
            mem.related_pages_map[*it].insert(tmp_related_pages.begin(), tmp_related_pages.end());
        }
    }
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
    LOG(INFO) << "Batch write current time: " << total_time.get_time();

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
        AdjList.assign(num_vertices, list<vid_t>());
        page_set.assign(num_edges * mem.trace_read_cnt / mem.page_size, set<vid_t>());
        mem.LPN_Boundary_vertices_set_map.assign(num_edges * mem.trace_read_cnt / mem.page_size, set<vid_t>());
        LOG(INFO) << "current time: " << total_time.get_time();
        LOG(INFO) << "edges.size(): " << edges.size();
        //create partial adj list
        for(auto &e : edges) {
            addEdge(e.first, e.second);
            //LOG(INFO) << "e.first: " << e.first << ", e.second: " << e.second;
        }
        //prt_adjacencylist();
        if (opt_name == "polon") {
            batch_DFS();
        } else {
            LOG(ERROR) << "no valid opt function";
        }
        num_edges_left -= edges_per_batch;
    }
}

void PreprocessingPartitioner::batch_DFS() {
    color = new vid_t[num_vertices];           // 配置記憶體位置
    discover = new vid_t[num_vertices];
    finish = new vid_t[num_vertices];
    predecessor = new vid_t[num_vertices];

    LOG(INFO) << "Batch DFS current time: " << total_time.get_time();
    int time = 0;                          // 初始化
    for (vid_t i = 0; i < num_vertices; i++) { 
        color[i] = 0;
        discover[i] = 0;
        finish[i] = 0;
        predecessor[i] = -1;
    }

    vid_t minPosition = UINT32_MAX;
    for (vid_t pos = 0; pos < part_degrees.size(); pos++) {
        if(part_degrees[pos] < minPosition && part_degrees[pos]) {
            minPosition = pos;
        }
    }

    bool flag = 0;
    
    while (!flag) { // use flag to inspect all edges in AdjList is cleared
        int cnt_for_vectices_size = 0;
        for (int k = 0; k < AdjList.size(); k++) { // 檢查所有Adj list中的edges all clear
            if (!AdjList[k].empty())
                break;
            else
                cnt_for_vectices_size++;
        }
        if (cnt_for_vectices_size == AdjList.size())
            flag = 1;
        else {
            LOG(INFO) << "3";
            if (color[minPosition] == 0) {               // 若vertex不是白色, 則進行以該vertex作為起點之搜尋
                DFSVisit(minPosition, time);
            }
            LOG(INFO) << "page_num: " << mem.page_num;
            LOG(INFO) << "cnt_for_visit_time: " << cnt_for_visit_time;
            LOG(INFO) << "page_edge_cnt: " << page_edge_cnt;
            if (cnt_for_visit_time != page_edge_cnt) { // page_buffer is not full, stored into the page_buffer
                LOG(INFO) << "Store into page buffer: " << total_time.get_time();
                mem.page_buffer.assign(mem.index_for_page_buffer, set<vid_t>());
                if (cnt_for_visit_time + mem.cnt_for_page_buffer < mem.page_buffer_size) {
                    mem.page_buffer[mem.index_for_page_buffer++] = boundary_vertices_set;
                    boundary_vertices_set.clear();
                    mem.cnt_for_edges_in_page_buffer.push_back(cnt_for_visit_time);
                    mem.cnt_for_page_buffer += cnt_for_visit_time;
                } else { // flushed the page_buffer into pages, boundary set compare with each other
                    mem.cnt_for_page_buffer = 0;
                    for (vid_t v_i = 0; v_i < num_vertices; v_i++) { // accumulate the related sets and sorted in descending order of vid, then flush
                        for (uint32_t index_i = 0; index_i < mem.index_for_page_buffer; index_i++) { // travese sets
                            mem.sorted_page_buffer_check[index_i] = false;
                            if (*mem.page_buffer[index_i].begin() == v_i && !mem.sorted_page_buffer_check[index_i]) {
                                mem.sorted_page_buffer_check[index_i] = true;
                                mem.tmp_page_buffer.insert(mem.tmp_page_buffer.begin() + mem.tmp_index_for_page_buffer, set<vid_t>());
                                mem.tmp_page_buffer[mem.tmp_index_for_page_buffer++] = mem.page_buffer[index_i];
                            }
                        }
                    }
                    mem.page_buffer.clear();
                    //merge all the set in a size of page for all page buffer
                    for (uint32_t set_tmp_index = 0; set_tmp_index < mem.tmp_page_buffer.size(); set_tmp_index++) {
                        if (mem.cnt_for_page_buffer_combine >= mem.page_size) {
                            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set;
                            boundary_vertices_set.clear();
                        }
                        boundary_vertices_set.insert(mem.tmp_page_buffer[set_tmp_index].begin(), mem.tmp_page_buffer[set_tmp_index].end());
                        mem.cnt_for_page_buffer_combine += mem.cnt_for_edges_in_page_buffer[set_tmp_index];
                    }
                    // place the redundant set into page buffer for next time the page buffer filled up
                    mem.page_buffer.insert(mem.page_buffer.begin() + mem.index_for_page_buffer, set<vid_t>());
                    mem.page_buffer[mem.index_for_page_buffer++] = boundary_vertices_set;
                    boundary_vertices_set.clear();
                    mem.cnt_for_page_buffer = mem.cnt_for_page_buffer_combine; 
                }
            }

            minPosition = UINT32_MAX;
            for (vid_t pos = 0; pos < part_degrees.size(); pos++) {
                if(part_degrees[pos] < minPosition && part_degrees[pos]) {
                    minPosition = pos;
                }
            }
            LOG(INFO) << "4";
            mem.LPN_Boundary_vertices_set_map[mem.page_num++] = boundary_vertices_set; // place the page
            boundary_vertices_set.clear();
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
}

void PreprocessingPartitioner::addEdge(vid_t s, vid_t d) {
    AdjList[s].push_back(d);
    AdjList[d].push_back(s);
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
    LOG(INFO) << "DFS Visit current time: " << total_time.get_time();
    if (cnt_for_visit_time == page_edge_cnt) {
        boundary_vertices_set.insert(vertex);
        return;
    }
    color[vertex] = 1;                         // 把vertex塗成灰色
    discover[vertex] = ++time;                 // 更新vertex的discover時間
    bool flag = 0;
    for (list<vid_t>::iterator itr = AdjList[vertex].begin();   // for loop參數太長
        itr != AdjList[vertex].end(); itr++) {                       // 分成兩段
        if (color[*itr] == 0) {                // 若搜尋到白色的vertex
            predecessor[*itr] = vertex;        // 更新其predecessor
            page_set[mem.page_num].insert(vertex);                     // for total replication factor
            page_set[mem.page_num].insert(*itr);                       // for total replication factor
            cnt_for_visit_time++;
            DFSVisit(*itr, time);              // 立刻以其作為新的搜尋起點, 進入新的DFSVisit()
            part_degrees[vertex]--, part_degrees[*itr]--;
            AdjList[vertex].erase(itr--);        // erase the edge in adj list, changed
            if (cnt_for_visit_time == page_edge_cnt) {
                boundary_vertices_set.insert(vertex);
                return;
            }
        } else {
            part_degrees[vertex]--, part_degrees[*itr]--;
            AdjList[vertex].erase(itr--);        // erase the edge in adj list, changed
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