#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include "util.hpp"
//#define page_size 4096;

using namespace std;

class Memory_Pre { 
private: 
    size_t channel = 32; 
    size_t dies = 4; 
    // 1.Request_Arrival_Time 2.Device_Number 3.Starting_Logical_Sector_Address 4.Request_Size_In_Sectors 5.Type_of_Requests[0 for write, 1 for read]
    size_t Request_Arrival_Time = -10; 
    size_t Device_Number = 0; 
    size_t Starting_Logical_Sector_Address = -8; 
    size_t Request_Size_In_Sectors = 8; 
    long pos; 
    //vector<vector<vid_t>> LPN_Boundary_vertices_set_map; 

    string basefilename = "/home/polon/Desktop/Master-Research/Output_trace_pre_yt.txt"; 
    ofstream trace_fout; 

    // void Update_batch_buffer(vid_t); 

public: 
    uint32_t page_size = 4096; 
    uint32_t cache_buffer = channel * dies; 
    uint32_t trace_write_cnt = 512; 
    uint32_t page_num = 0;
    uint32_t index_for_page_buffer = 0; // index of set number
    uint32_t page_buffer_size = cache_buffer * trace_write_cnt;
    uint32_t cnt_for_page_buffer = 0; //counting the edges togather for containing the page buffer size
    uint32_t tmp_cnt_for_page_buffer = 0;
    uint32_t cnt_for_page_buffer_combine = 0;
    size_t read_times = 0;
    size_t write_times = 0;

    vector<vector<vid_t>> vid_for_page_range;
    vector<set<uint32_t>> related_pages_map;
    vector<set<uint32_t>> vertices_LPN_map;
    vector<set<vid_t>> LPN_Boundary_vertices_set_map;
    vector<set<vid_t>> page_buffer;
    vector<uint32_t> cnt_for_edges_in_page_buffer; //counting the edges separate from different sets in page buffer

    void Set_trace_files();
    void Trace_R();
    void Trace_W();
};