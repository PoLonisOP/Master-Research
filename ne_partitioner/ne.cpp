#include "ne.hpp"

//固定随机数
NePartitioner::NePartitioner(std::string basefilename, std::string method, int pnum)
        : basefilename(basefilename), gen(985) {
    p = pnum;
    set_write_files(basefilename,method,pnum);

    total_time.start();

    std::ifstream fin(binedgelist_name(basefilename),
                      std::ios::binary | std::ios::ate);
    // tellp 用于返回写入位置，
    // tellg 则用于返回读取位置也代表着输入流的大小
    auto filesize = fin.tellg();
//  LOG(INFO) << "file size: " << filesize;
    fin.seekg(0, std::ios::beg);

    //最大的下标+1
    fin.read((char *)&num_vertices, sizeof(num_vertices));
    fin.read((char *)&num_edges, sizeof(num_edges));

    CHECK_EQ(sizeof(vid_t) + sizeof(size_t) + num_edges * sizeof(edge_t),
             filesize);

    average_degree = (double)num_edges * 2 / num_vertices;
    assigned_edges = 0;
    capacity = (double)num_edges * BALANCE_RATIO / p + 1;
    occupied.assign(p, 0);
    adj_out.resize(num_vertices);
    adj_in.resize(num_vertices);
    is_cores.assign(p, dense_bitset(num_vertices));
    is_boundarys.assign(p, dense_bitset(num_vertices));
    true_vids.resize(num_vertices);
    is_mirrors.assign(num_vertices,dense_bitset(p));
    master.assign(num_vertices, -1);
    dis.param(std::uniform_int_distribution<vid_t>::param_type(0, num_vertices - 1));

    edges.resize(num_edges);
    fin.read((char *) &edges[0], sizeof(edge_t) * num_edges);
    adj_out.build(edges, mem);
    adj_in.build_reverse(edges, mem);

    degrees.resize(num_vertices);
    std::ifstream degree_file(degree_name(basefilename), std::ios::binary);
    degree_file.read((char *)&degrees[0], num_vertices * sizeof(vid_t));
    degree_file.close();

    part_degrees.assign(num_vertices,vector<vid_t>(p));
    balance_vertex_distribute.resize(num_vertices);
    fin.close();
}

//最后一个子图就是剩下边组合而成
void NePartitioner::assign_remaining() {
    auto &is_boundary = is_boundarys[p - 1], &is_core = is_cores[p - 1];
    repv(u, num_vertices) for (auto &i : adj_out[u]) if (edges[i.v].valid()) {
            assign_edge(p - 1, u, edges[i.v].second);
            is_boundary.set_bit_unsync(u);
            is_boundary.set_bit_unsync(edges[i.v].second);
        }
    repv(i, num_vertices) {
        if (is_boundary.get(i)) {
            is_core.set_bit_unsync(i);
            //在其他子图中是核心集的不予理睬，不能设置为本子图的核心集
            rep(j, p - 1) if (is_cores[j].get(i)) {
                is_core.set_unsync(i, false);
                break;
            }
        }
    }
}

size_t NePartitioner::count_mirrors() {
    size_t result = 0;
    rep(i, p) result += is_boundarys[i].popcount();
    return result;
}

void NePartitioner::split() {
    min_heap.reserve(num_vertices);

    LOG(INFO) << "partitioning...";
    for (bucket = 0; bucket < p - 1; bucket++) {
        // std::cerr << bucket << ", ";
        // Ek<delta
        while (occupied[bucket] < capacity) {
            vid_t d, vid;
            //初始
            if (!min_heap.get_min(d, vid)) {
                //从v-c中随机选择
                if (!get_free_vertex(vid)) {
                //LOG(INFO) << "partition " << bucket << " stop: no free vertices";
                    break;
                }
                d = adj_out[vid].size() + adj_in[vid].size();
            } else {
                //应该是通过建立s-c的堆选择最小的度结点 度算的是除开s的边
                min_heap.remove(vid);
                /* CHECK_EQ(d, adj_out[vid].size() + adj_in[vid].size()); */
            }

            occupy_vertex(vid, d);
        }
        min_heap.clear();
        rep(direction, 2) repv(vid, num_vertices) {
            adjlist_t &neighbors = direction ? adj_out[vid] : adj_in[vid];
            for (size_t i = 0; i < neighbors.size();) {
                if (edges[neighbors[i].v].valid()) {
                    i++;
                } else {
                    std::swap(neighbors[i], neighbors.back());
                    neighbors.pop_back();
                }
            }
        }
        std::cerr << "For the " << bucket << " partition complete!" << std::endl;
    }
    bucket = p - 1;
    assign_remaining();
    std::cerr << "For the " << bucket << " partition complete!" << std::endl;

    CHECK_EQ(assigned_edges, num_edges);

    vector<vid_t> buckets(p);
    double capacity = (double)true_vids.popcount() * 1.05 / p + 1;
    rep(i, num_vertices){
        double max_score=0.0;
        vid_t which_p;
        bool unique=false;
        if(is_mirrors[i].popcount()==1){
            unique=true;
        }
        repv(j, p){
            if (is_mirrors[i].get(j)) {
//                double score=((i%p==j)?1:0)+(part_degrees[i][j]/(degrees[i]+1))+(buckets[j]< capacity?1:0);
                double score=(part_degrees[i][j]/(degrees[i]+1))+(buckets[j]< capacity?1:0);
                if (unique){
                    which_p=j;
                }else if (max_score<score){
                    max_score=score;
                    which_p=j;
                }
            }
        }
        ++buckets[which_p];
        save_vertex(i,which_p);
        balance_vertex_distribute[i]=which_p;
    }
    node_fout.close();

    size_t total_mirrors = count_mirrors();

    ifstream fin(binedgelist_name(basefilename), std::ios::binary | std::ios::ate);
    fin.seekg(sizeof(num_vertices) + sizeof(num_edges), std::ios::beg);
    edges.resize(num_edges);
    fin.read((char *)&edges[0], sizeof(edge_t) * num_edges);
    for (auto &e : edges)
    {
        vid_t sp=balance_vertex_distribute[e.first],tp=balance_vertex_distribute[e.second];
        save_edge(e.first,e.second,sp);
        save_edge(e.second,e.first,tp);
    }
    edge_fout.close();

    // edge_fout.close();
    total_time.stop();
    // repv(j, p)
    //     LOG(INFO) << "each partition node count: " << buckets[j];
    LOG(INFO) << "expected edges in each partition: " << num_edges / p;
    size_t max_occupied = *std::max_element(occupied.begin(), occupied.end());
    LOG(INFO) << "balance: " << (double)max_occupied / ((double)num_edges / p);
    rep (i, p)
        DLOG(INFO) << "edges in partition " << i << ": " << occupied[i];
    LOG(INFO) << "total mirrors: " << total_mirrors;
    LOG(INFO) << "replication factor: " << (double)total_mirrors / true_vids.popcount();
    // changed
    LOG(INFO) << "Read times in trace: " << mem.read_times;
    LOG(INFO) << "Write times in trace: " << mem.write_times;
    // LOG(INFO) << "Read times in trace: " << mem.read_times_2;
    // LOG(INFO) << "Write times in trace: " << mem.write_times_2;
    LOG(INFO) << "page_num: " << mem.page_num_cnt;

    LOG(INFO) << "total partition time: " << total_time.get_time();
}

