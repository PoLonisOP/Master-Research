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

class Memory { 
private: 
    size_t page_index = 0;
    size_t channel = 32; 
    size_t dies = 4; 
    // 1.Request_Arrival_Time 2.Device_Number 3.Starting_Logical_Sector_Address 4.Request_Size_In_Sectors 5.Type_of_Requests[0 for write, 1 for read]
    size_t Request_Arrival_Time = -10; 
    size_t Request_Arrival_Time_2 = -10;
    size_t Device_Number = 0; 
    size_t Starting_Logical_Sector_Address_2 = -8;
    size_t Starting_Logical_Sector_Address = -8; 
    size_t Request_Size_In_Sectors = 8; 
    long pos; 

    string basefilename = "/home/polon/Desktop/Master-Research/Output_trace_ne_yt_adj.txt"; 
    string basefilename2 = "/home/polon/Desktop/Master-Research/Output_trace_ne_yt_csr.txt"; 
    ofstream trace_fout;
    ofstream trace_fout2;

    // void Update_batch_buffer(vid_t); 

public: 
    size_t page_size = 4096; 
    size_t cache_buffer = channel * dies; 
    size_t CSR_trace_cnt = 1024;
    size_t read_times = 0;
    size_t write_times = 0;
    size_t read_times_2 = 0;
    size_t write_times_2 = 0;

    void Set_trace_files();
    void Set_trace_files2();
    void Trace_R();
    void Trace_W();
    void Trace_R2();
    void Trace_W2();
};