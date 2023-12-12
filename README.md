# GraLoc
Github: https://github.com/PoLonisOP/Master-Research/tree/preprocessingPartition
## main 
之前使用 ..SSD 不同的儲存方式 (ex. CSR, ADJ, sequential) 跟 pagerank 演算法結合, 直接 vscode 跑任何一個 `.exe` 就可以運行
## preprocessingPartition 
包含 ne, fennel, ldg, dbh, hdrf and my method(pre)不同的 partitioning 方法, 其中我的方法跟 ne 有做 SSD 儲存方式比較 (ex. ADJ, CSR)  
Linux上 cmake 後執行 `./GraphPartitioning` 運行  
  
some args in the `main.cpp`:  
```c++
int pnum = 30;
    int memsize = 4096;
    string method = "ne";
    double lambda = 1.1;
    double balance_ratio = 1.05;
```
pnum is the partitioned chunks, memsize is the memory size for edge streaming methods, method is the choosed partitioner, lambda is for HDRF, balance_ratio is the edge counts deviating from the average, edgename is the absolute path for graph datasets.  
