#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <mpi.h>

#define MAX(x, y) ((x)>(y) ? (x):(y))
#define MIN(x, y) ((x)<(y) ? (x):(y))

#define INF 0x7FFFFFFF

#define _DEBUG_
#define _MEASURE_TIME

#ifdef _MEASURE_TIME
    double __temp_time=0;
    #define TIC     __temp_time = MPI_Wtime()
    #define TOC(X)  X = (MPI_Wtime() - __temp_time)
    #define TOC_P(X) X += (MPI_Wtime() - __temp_time)
    #define TIME(X) X = MPI_Wtime()

    double total_exetime=0;
    double total_calctime=0;
    double total_iotime=0;
    double total_commtime=0;
    double exe_st=0;
    double exe_ed=0;
    #define ST exe_st
    #define ED exe_ed
    #define EXE total_exetime
    #define CALC total_calctime
    #define IO total_calctime
    #define COMM total_commtime

#else
    #define TIC
    #define TOC(X)
    #define TOC_P(X)
    #define TIME(X)

    #define ST
    #define ED
    #define EXE
    #define CALC
    #define IO
    #define COMM
#endif


#ifdef _DEBUG_
    int __print_step = 0;

    void __pt_log(const char *f_, ...){
        std::stringstream ss;
        ss << "[Rank %d] Step %08d: " << f_ <<'\n';
        const char *format = ss.str().c_str();

        va_list va;
        va_start(va, f_);
            vprintf(format, va);
        va_end(va);
        __print_step++;
    }

    #define VA_ARGS(...) , ##__VA_ARGS__
    #define LOG(f_, ...) __pt_log((f_), world_rank, __print_step VA_ARGS(__VA_ARGS__))
#else
    #define LOG(F_, ...)
#endif

int world_size;
int world_rank;

int vert;
int edge;

MPI_Comm COMM_GRAPH;
int graph_size;
int graph_rank;

const int NOTHING = 0;

enum tree_tag_field { invite=1, reject, join };
enum task_field { none=0, t_handle, t_back, t_signal, updt, no_updt };

int *data;
//int *bufdata;
int *buf;
int parent = -1;
int neighbor_count;
int child_count;
int terminal_signal = none;
std::vector<int> neighbor_list;
std::vector<int> child_list;
std::vector<int> update_list;
std::vector<int> terminate_list;
std::vector<int> send_req;


inline void init(int v){
    data = new int[v];
    //bufdata = new int[v*v];
    buf = new int[v];
    neighbor_list.reserve(v);
    child_list.reserve(v);
    update_list.resize(v);
    terminate_list.resize(v);

    std::fill(data, data+v, INF);
    std::fill(update_list.begin(), update_list.end(), 1);
    std::fill(terminate_list.begin(), terminate_list.end(), 0);
}

inline void finalize(){
    delete[] data;
    //delete[] bufdata;
    delete[] buf;
}

inline int update(int id){
    int up = 0;
    for(int i=0;i<vert;++i){
        if(data[i] > data[id] + buf[i]){
            data[i] = data[id] + buf[i];
            up = 1;
        }
    }
    return up;
}

inline void dump_from_file(const char *file){
    std::ifstream fin(file);
    std::stringstream ss;

    TIC;{
    ss << fin.rdbuf();

    }TOC_P(IO);

    ss >> vert >> edge;

    init(vert);

    int i, j, w;
    for(int e=0;e<edge;++e){
        ss >> i >> j >> w;
        if(i==world_rank)data[j]=w;
        else if(j==world_rank)data[i]=w;
    }


    for(int i=0;i<vert;++i){
        if(data[i] != INF)
            neighbor_list.push_back(i);
    }
    neighbor_count = neighbor_list.size();
}

inline void dump_to_file(const char *file){
    std::stringstream ss;

    std::ostream_iterator<int> out(ss, " ");
    std::copy(map.data, map.data+map.vt, out);
    ss << '\n';

    std::string str = ss.str();
    int *len = new int[map.vt]();
    len[world_rank] = str.size();

    MPI_File fout;
    MPI_File_open(MPI_COMM_WORLD, file, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fout);


    LOG("writing file");


    TIC;{
    MPI_Allreduce(MPI_IN_PLACE, len, map.vt, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    }TOC_P(COMM);

    int offset=0;
    for(int i=0;i<world_rank;++i){
        offset += len[i];
    }

    TIC;{

    MPI_File_set_view(fout, offset, MPI_CHAR, MPI_CHAR, "native", MPI_INFO_NULL);
    MPI_File_write_all(fout, str.c_str(), len[world_rank], MPI_CHAR, MPI_STATUS_IGNORE);

    MPI_File_close(&fout);

    }TOC_P(IO);
    
    LOG("write file done");


    delete[] len;
}


inline void create_graph(){
    //MPI_Dist_graph_create(MPI_COMM_WORLD, 1, &world_rank, &neighbor_count, 
    //        neighbor_list.data(), MPI_UNWEIGHTED, MPI_INFO_NULL, false, &COMM_GRAPH);
    //MPI_Comm_rank(COMM_GRAPH, &graph_rank);
    COMM_GRAPH = MPI_COMM_WORLD;
    graph_rank = world_rank;
    assert(graph_rank == world_rank);
}

inline int check_all_no_update(){
    int up = 0;
    for(int i=0;i<neighbor_count;++i){
        up |= update_list[ neighbor_list[i] ];
    }
    return (up == 0) ? 1:0;
}

inline int check_all_terminate(){
    int ter = 0;
    for(int i=0;i<child_count;++i){
        ter |= terminate_list[ child_list[i] ];
    }
    return ter;
}


inline void isend_to_all_neighbor(void *buf, int count, MPI_Datatype type,
                int tag, MPI_Comm comm, MPI_Request *request, bool wait=false){
    for(int i=0;i<neighbor_count;++i){
        MPI_Isend(buf, count, type, neighbor_list[i], tag, comm, &request[i]);
    }

    if(wait)
        MPI_Waitall(neighbor_count, request, MPI_STATUSES_IGNORE);
}

inline void irecv_from_all_neighbor(void *buf, int count, MPI_Datatype type, 
                int tag, MPI_Comm comm, MPI_Request *request, bool wait=false){

    for(int i=0;i<neighbor_count;++i){
        MPI_Irecv(buf, count, type, neighbor_list[i], tag, comm, &request[i]);
    }

    if(wait)
        MPI_Waitall(neighbor_count, request, MPI_STATUSES_IGNORE);
}

inline void isend_to_all_neighbor_except(void *buf, int count, MPI_Datatype type,
                int tag, int except_node, MPI_Comm comm, MPI_Request *request, bool wait=false){
    
    for(int i=0,j=0;i<neighbor_count;++i){
        if(neighbor_list[i] != except_node){
            MPI_Isend(buf, count, type, neighbor_list[i], tag, comm, &request[j]);
            ++j;
        }
    }

    if(wait)
        MPI_Waitall(neighbor_count, request, MPI_STATUSES_IGNORE);
}

inline void isend_to_all_child(void *buf, int count, MPI_Datatype type,
                int tag, MPI_Comm comm, MPI_Request *request, bool wait=false){

    for(int i=0;i<child_count;++i){
        MPI_Isend(buf, count, type, child_list[i], tag, comm, &request[i]);
    }

    if(wait)
        MPI_Waitall(neighbor_count, request, MPI_STATUSES_IGNORE);
}

inline void create_spanning_tree(){
    send_req.resize(neighbor_count);

    if(graph_rank == 0){
        parent=0;
        terminal_signal = t_handle;
        isend_to_all_neighbor(data, vert, MPI_INT, invite, COMM_GRAPH, send_req.data(), true);
    }

    MPI_Status status;
    for(int i=0;i<neighbor_count;++i){
        MPI_Recv(buf, vert, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, COMM_GRAPH, &status);
        switch(status.MPI_TAG){
            case invite:
                //already has parent
                if(parent != -1){
                    LOG("Recv invite from %d, reject, already has parent %d", status.MPI_SOURCE, parent);
                    MPI_Isend(data, vert, MPI_INT, status.MPI_SOURCE, reject, COMM_GRAPH, &send_req.back());
                }else{
                    LOG("Recv invite from %d, join", status.MPI_SOURCE);
                    parent=status.MPI_SOURCE;
                    MPI_Isend(data, vert, MPI_INT, parent, join, COMM_GRAPH, &send_req.back());
                    isend_to_all_neighbor_except(data, vert, MPI_INT, invite, parent, COMM_GRAPH, send_req.data());
                }
                update_list[status.MPI_SOURCE] = update(status.MPI_SOURCE);
                break;
            case join:
                LOG("%d join me", status.MPI_SOURCE);
                //MPI_Recv(buf, vert, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, COMM_GRAPH, MPI_STATUS_IGNORE);
                child_list.push_back(status.MPI_SOURCE);
                update_list[status.MPI_SOURCE] = update(status.MPI_SOURCE);
                break;
            case reject:
                LOG("%d reject me", status.MPI_SOURCE);
                //MPI_Recv(buf, vert, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, COMM_GRAPH, MPI_STATUS_IGNORE);
                update_list[status.MPI_SOURCE] = update(status.MPI_SOURCE);
                break;
        }
    }

    std::sort(child_list.begin(), child_list.end());
    child_count = child_list.size();
    
    MPI_Waitall(neighbor_count, send_req.data(), MPI_STATUSES_IGNORE);

    #ifdef _DEBUG_
    std::stringstream ss;
    for(int i=0;i<child_count;++i){
        ss << child_list[i] << ", ";
    }
    LOG("my child list: %s", ss.str().c_str());
    #endif
}

inline void task(){

    MPI_status status;

    int not_done = 1;
    while(not_done){
        if(terminal_signal == t_handle){
            if(check_all_no_update()){
                if(child_count == 0){
                    LOG("leaf node, send t_back to parent");
                    terminal_signal = t_back;
                    MPI_Isend(&NOTHING, 1, MPI_INT, parent, t_back, COMM_GRAPH, &send_req[0]);
                }else{
                    LOG("send t_handle to all child");
                    terminal_signal = t_back;
                    isend_to_all_child(&NOTHING, 1, MPI_INT, t_handle, COMM_GRAPH, send_req.data(), false);
                }
            }
        }
        MPI_Recv(buf, vert, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, COMM_GRAPH, &status);

        switch(status.MPI_TAG){
            case t_handle:
                LOG("recv t_handle");
                terminal_signal = t_handle;
                break;
            case t_back:
                terminate_list[status.MPI_SOURCE] = 1;
                if(check_all_terminate()){
                    LOG("all child send back t_back!!");
                    if(graph_rank == 0){
                        LOG("send t_signal to all child");
                        isend_to_all_child(&NOTHING, 1, MPI_INT, t_signal, COMM_GRAPH, send_req.data(), false);
                        not_done=0;
                    }else{
                        LOG("send t_back to parent");
                        terminal_signal = t_back;
                        MPI_Isend(&NOTHING, 1, MPI_INT, parent, t_back, COMM_GRAPH, &send_req[0]);
                    }
                }
                break;
            case t_signal:
                LOG("send t_signal to all child");
                isend_to_all_child(&NOTHING, 1, MPI_INT, t_signal, COMM_GRAPH, send_req.data(), false);
                not_done=0;
                break;
            case updt:
                if(update(status.MPI_SOURCE)){
                    isend_to_all_neighbor(data, vert, MPI_INT, updt, COMM_GRAPH, send_req.data(), false);
                    update_list[status.MPI_SOURCE] = 1;
                }else{
                    MPI_Isend(data, vert, MPI_INT, status.MPI_SOURCE, no_updt, COMM_GRAPH, &send_req[status.MPI_SOURCE]);
                    update_list[status.MPI_SOURCE] = 0;
                }
                break;
            case no_updt:
                update_list[status.MPI_SOURCE] = 0;
                break;
        }
    }

    LOG("done wait for end");
    MPI_Waitall(neighbor_count, send_req.data(), MPI_STATUSES_IGNORE);
}

int main(int argc, char **argv){

    assert(argc == 4);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    TIME(ST);

    LOG("dump from file");

    //start here
    dump_from_file(argv[1]);

    LOG("create graph");
    create_graph();

    LOG("cerate spanning tree");
    create_spanning_tree();
    
    LOG("start task");
    task();

    LOG("dump to file");
    dump_to_file(argv[2]);

#ifdef _MEASURE_TIME
    TIME(ED);
    EXE = ED - ST;
    //rank, EXE, CALC, IO, COMM, PROC
    printf("%d, %lf, %lf, %lf, %lf, %lf\n", world_rank, EXE, CALC, IO, COMM, EXE-CALC-IO-COMM);
#endif

    finalize();
    MPI_Finalize();
    return 0;
}
