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
    //size_t page_num = 0; 
    size_t chennel = 32; 
    size_t dies = 4; 
    // 1.Request_Arrival_Time 2.Device_Number 3.Starting_Logical_Sector_Address 4.Request_Size_In_Sectors 5.Type_of_Requests[0 for write, 1 for read]
    size_t Request_Arrival_Time = -10; 
    size_t Device_Number = 0; 
    size_t Starting_Logical_Sector_Address = -8; 
    size_t Request_Size_In_Sectors = 8; 
    long pos; 

    string basefilename = "/home/polon/Desktop/GraphPartitioners-main/Output_trace.txt"; 
    ofstream trace_fout; 

    // void Update_batch_buffer(vid_t); 

public: 
    size_t page_size = 4096; 
    size_t cache_buffer = chennel * dies; 
    size_t trace_write_cnt = 512; 
    size_t trace_read_cnt = 512; 
    size_t page_num = 0;

    void Set_trace_files();
    void Trace_write();
    void Trace_read();
    void Trace_R();
    void Trace_W();
};