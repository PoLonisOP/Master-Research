﻿# GraLoc
GraLoc, a graph partitioning solution designed to be NAND flash memory-friendly by considering graph locality during data placement. The proposed solution aims to maximize graph locality within a single page, effectively minimizing read amplification during the graph partitioning process. Our experiments demonstrate a significant improvement in storage performance during graph partitioning. 
## storage_PR branch
之前使用 SSD 不同的儲存方式 (ex. CSR, ADJ, sequential) 跟 pagerank 演算法結合, 直接 vscode 跑任何一個 `.exe` 就可以運行
## preprocessingPartition branch
包含 ne, fennel, ldg, dbh, hdrf and my method(pre) 不同的 partitioning 方法, 其中我的方法跟 ne 有做 SSD 儲存方式比較 (ex. ADJ, CSR)  
Linux 上 cmake 後執行 `./GraphPartitioning` 運行  
```c++
mkdir build
cd build
cmake ..
make -j8
./GraphPartitioning 
```
  
### Execution environment 
Install cmake <https://cmake.org/download/>, glog <https://github.com/google/glog>, gflags <https://github.com/gflags/gflags/blob/master/INSTALL.md> and MQSim in Linux, my Ubuntu version is 20.04. 
  
### Baseline operation 
We compare the page replacements with `ne` method with two state-of-art storage format (CSR and Adj), based on <https://github.com/zongshenmu/GraphPartitioners>.  
If you want to comment out one of the storage format in `graph.cpp` , please make sure the format marked in comments is active. Btw, the default is both active. 
* `main.cpp` input:    
  ```c++
  int pnum = 30;
      int memsize = 4096;
      string method = "ne";
      double lambda = 1.1;
      double balance_ratio = 1.05;
    string edgename = "/home/polon/Desktop/Master-Research/Datasets/com-youtube.ungraph.txt";
  ```
  pnum is the partitioned chunks, memsize is the memory size for edge streaming methods, method is the choosed partitioner, lambda is for HDRF, balance_ratio is the edge counts deviating from the average, edgename is the absolute path for graph datasets. All of our datasets is cited from <https://snap.stanford.edu/data/index.html>.  
* `memory.hpp` input:
  ```c++
    size_t channel = 32; 
    size_t dies = 4; 
    // 1.Request_Arrival_Time 2.Device_Number 3.Starting_Logical_Sector_Address 4.Request_Size_In_Sectors 5.Type_of_Requests[0 for write, 1 for read]
    size_t Request_Arrival_Time = -10; 
    size_t Request_Arrival_Time_2 = -10;
    size_t Device_Number = 0; 
    size_t Starting_Logical_Sector_Address_2 = -8;
    size_t Starting_Logical_Sector_Address = -8; 
    size_t Request_Size_In_Sectors = 8;
  ```
  In the private, channel is the amount of channels, and so as die. The rest of arguments are MQSim input trace's arguments, you can see in the <https://github.com/CMU-SAFARI/MQSim>. 
  ```c++
    size_t page_size = 4096; 
    size_t CSR_trace_cnt = 1024;
  ```
  In the public, page_size is the size of page, CSR_trace_cnt is the amount of edges in the CSR format storage. 
* `ne.cpp` output:
  ```c++
    repv(j, p)
        LOG(INFO) << "each partition node count: " << buckets[j];
    LOG(INFO) << "expected edges in each partition: " << num_edges / p;
    size_t max_occupied = *std::max_element(occupied.begin(), occupied.end());
    LOG(INFO) << "balance: " << (double)max_occupied / ((double)num_edges / p);
    rep (i, p)
        DLOG(INFO) << "edges in partition " << i << ": " << occupied[i];
    LOG(INFO) << "total mirrors: " << total_mirrors;
    LOG(INFO) << "replication factor: " << (double)total_mirrors / true_vids.popcount();
    // changed
    LOG(INFO) << "Adj read times in trace: " << mem.read_times;
    LOG(INFO) << "Adj write times in trace: " << mem.write_times;
    LOG(INFO) << "Csr read times in trace: " << mem.read_times_2;
    LOG(INFO) << "Csr write times in trace: " << mem.write_times_2;
    LOG(INFO) << "page number: " << mem.page_num_cnt;

    LOG(INFO) << "total partition time: " << total_time.get_time();
  ```
* `memory.hpp` output:
  ```c++
    string basefilename = "/home/polon/Desktop/Master-Research/Output_trace_ne_yt_adj.txt"; 
    string basefilename2 = "/home/polon/Desktop/Master-Research/Output_trace_ne_yt_csr.txt";
  ```
  basefilename is the output trace of Adj list format, and basefilename2 is for CSR format. The Output_trace can be put into MQSim's `workload.xml` directly.  
### GraLoc operation
Choose our method with `pre` in the `main.cpp` line `24`.  
* `memory_for_pre.hpp` input:  
  same as `memory.hpp`.  
* `pre_processing.cpp` output:  
  ```c++
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
    LOG(INFO) << "Amount of boundary vertices sets: " << cnt_for_bvs;
  ```
### MQSim setup
Set the flash memory arguments in `ssdconfig.xml`.  
<table>
    <tr>
        <td align="center" colspan=4 >MQSim Settings</td>
    </tr>
    <tr>
        <td>Cell types</td>
        <td>MLC</td>
        <td>Capacity</td>
        <td>8GB</td>
    </tr>
    <tr>
        <td>Block size</td>
        <td>1MB</td>
        <td>Page size</td>
        <td>1 ns/200 fJ</td>
    </tr>
    <tr>
        <td>Program</td>
        <td>750us</td>
        <td>Read</td>
        <td>35us</td>
    </tr>
</table>
