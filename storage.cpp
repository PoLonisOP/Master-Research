#include"storage.hpp"

void Storage::StorePageAdjMethod(vid_t src, vid_t dst) {
    flag_in_memory = 0;
    if (ADJ_lru_pointer > 128)
        for (size_t index: ADJ_lru_for_working_memory)
            index -= 128;

    // check the page is in the working memory or not
    for (auto it = ADJ_working_memory.begin(); it < ADJ_working_memory.end(); it++) {
        // cout << "it->first: " << it->first << endl;
        if (it->first == src) {
            memory_index = it - ADJ_working_memory.begin();
            flag_in_memory = 1;
        }
    }
    
    if (!flag_in_memory) {
        // check if the map whether the page is exsited or not
        if (src_page_num_map.find(src) != src_page_num_map.end()) {
            for (int j = 0; j < src_page_num_map[src].size(); j++) {
                // index of min element
                min = min_element(ADJ_lru_for_working_memory.begin(), ADJ_lru_for_working_memory.end()) - ADJ_lru_for_working_memory.begin();
                ADJ_working_memory[min] = std::make_pair(src, src_page_num_map[src].at(j));
                ADJ_lru_for_working_memory[min] = ++ADJ_lru_pointer;
                Trace_R();
                Trace_W();
            }
            // check if the page is full
            if (page_for_adj[src_page_num_map[src].back()].size() == CSR_trace_cnt) {
                page_for_adj.push_back(std::vector<vid_t>());
                page_for_adj[page_num_cnt].push_back(dst);
                src_page_num_map[src].push_back(page_num_cnt);
                min = min_element(ADJ_lru_for_working_memory.begin(), ADJ_lru_for_working_memory.end()) - ADJ_lru_for_working_memory.begin();
                ADJ_working_memory[min] = std::make_pair(src, page_num_cnt++);
                ADJ_lru_for_working_memory[min] = ++ADJ_lru_pointer;
                Trace_W();
            } else {
                // input the edge in the last page
                page_for_adj[src_page_num_map[src].back()].push_back(dst);
            }
        } else {
            // create new page, update the map
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            src_page_num_map[src].push_back(page_num_cnt);
            min = min_element(ADJ_lru_for_working_memory.begin(), ADJ_lru_for_working_memory.end()) - ADJ_lru_for_working_memory.begin();
            ADJ_working_memory[min] = std::make_pair(src, page_num_cnt++);
            ADJ_lru_for_working_memory[min] = ++ADJ_lru_pointer;
            Trace_W();
        }
    } else {
        // std::cout << "Input the edge" << std::endl;
        // std::cout << src_page_num_map[src].size() << std::endl;
        if (src_page_num_map[src].size() == 0) {
            //create new page, update the map
            //std::cout << "huzzah" << std::endl;
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            src_page_num_map[src].push_back(page_num_cnt);
            min = min_element(ADJ_lru_for_working_memory.begin(), ADJ_lru_for_working_memory.end()) - ADJ_lru_for_working_memory.begin();
            ADJ_working_memory[min] = std::make_pair(src, page_num_cnt++);
            ADJ_lru_for_working_memory[min] = ++ADJ_lru_pointer;
            Trace_W();
        } else if (page_for_adj[src_page_num_map[src].back()].size() == CSR_trace_cnt) {
            //std::cout << "Poggg" << std::endl;
            //create new page, update the map
            page_for_adj.push_back(std::vector<vid_t>());
            page_for_adj[page_num_cnt].push_back(dst);
            src_page_num_map[src].push_back(page_num_cnt);
            min = min_element(ADJ_lru_for_working_memory.begin(), ADJ_lru_for_working_memory.end()) - ADJ_lru_for_working_memory.begin();
            ADJ_working_memory[min] = std::make_pair(src, page_num_cnt++);
            ADJ_lru_for_working_memory[min] = ++ADJ_lru_pointer;
            Trace_W();
        } else {
            // input the edge in the last page
            // std::cout << "Ayaya" << std::endl;
            page_for_adj[src_page_num_map[src].back()].push_back(dst);
            ADJ_lru_for_working_memory[memory_index] = ++ADJ_lru_pointer;
        }
    }
    // std::cout << "Working memory state: " << 
}

// changed
void Storage::StorePageCsrMethod(vid_t src, vid_t dst, size_t edge_num) {
    size_t edges_from_src = CSR_vertex_map[src + 1] - CSR_vertex_map[src];
    size_t pages_from_src = CSR_vertex_map[src + 1] / CSR_trace_cnt - CSR_vertex_map[src] / CSR_trace_cnt + 1;
    size_t src_init_page = CSR_vertex_map[src] / CSR_trace_cnt;
    size_t page_number = edge_num / CSR_trace_cnt;
    bool push_into_last_edge = 1;
    full = (edge_num % CSR_trace_cnt) ? false : true;
    flag_in_memory = 0;

    if (CSR_lru_pointer > 128)
        for (size_t index: CSR_lru_for_working_memory)
            index -= 128;

    // seperate the page replacement computing and CSR data update
    // CSR data update
    if (!CSR_edge_map.size())
        CSR_edge_map.emplace_back(dst);
    else
        for (vid_t i = CSR_vertex_map.at(src); i < CSR_vertex_map.at(src + 1); i++)
            if (dst < CSR_edge_map.at(i)) {
                CSR_edge_map.insert(CSR_edge_map.begin() + i, dst); 
                push_into_last_edge = 0;
            }
    if (push_into_last_edge)
        CSR_edge_map.emplace_back(dst);
    for (vid_t i = src + 1; i < CSR_vertex_map.size(); i++)
        CSR_vertex_map[i]++;
    
    // page replacement computing
    if (!full) 
        page_number++;
    for (size_t pr = src_init_page; pr < page_number; pr++) {
        for (auto it = 0; it < CSR_working_memory.size(); it++) {
            // check if the first page of src is in the working memory or not
            if (CSR_working_memory[it] == pr) {
                CSR_lru_for_working_memory[it] = ++CSR_lru_pointer;
                flag_in_memory = 1;
            }
        }
        if (!flag_in_memory) {
            min = min_element(CSR_lru_for_working_memory.begin(), CSR_lru_for_working_memory.end()) - CSR_lru_for_working_memory.begin();
            CSR_working_memory[min] = pr;
            CSR_lru_for_working_memory[min] = ++CSR_lru_pointer;
            Trace_W2();
            Trace_R2();
        }
    }
    if (edge_num % CSR_trace_cnt == 1) {
        min = min_element(CSR_lru_for_working_memory.begin(), CSR_lru_for_working_memory.end()) - CSR_lru_for_working_memory.begin();
        CSR_working_memory[min] = page_number + 1;
        CSR_lru_for_working_memory[min] = ++CSR_lru_pointer;
        Trace_W2();
    }
}

void Storage::prt_CSR() {
    // LOG(INFO) << "CSR_edge_map: ";
    // for (auto itr = CSR_edge_map.begin(); itr != CSR_edge_map.end(); itr++) 
    //     cout << *itr << " ";
    // cout << endl;
    LOG(INFO) << "CSR_vertex_map: ";
    for (auto itr = CSR_vertex_map.begin(); itr != CSR_vertex_map.end(); itr++) 
        cout << *itr << " ";
    cout << endl;
}