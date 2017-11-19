#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iterator>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>

#define MAX(x, y) ((x)>(y)?(x):(y))
#define MIN(x, y) ((x)<(y)?(x):(y))


#define INF 999999999

//#define _DEBUG_
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


int world_size;
int world_rank;

int vert;
int edge;


MPI_Comm COMM_GRAPH;
int graph_size;
int graph_rank;

const int NOTHING = 0;

enum tags { invt=1, rej, join, upd, nupd, term, stop };
enum term_flag { none, fwd, back };
//=v==v==v==v==v=   MAP   =v==v==v==v==v=//
struct Map{
    //allocate
    int *data;
    int *buf;
    int *node_update;

    //map info
    int parent;
    std::vector<int> neig;
    std::vector<int> chds;
    std::vector<int> term;
    std::vector<MPI_Request> sendreq;
    std::vector<MPI_Request> recvreq;

    int vt;
    int nb;

    //fags
    int not_done;
    int has_update;
    int term_flag;

    Map();
    ~Map(){};
    inline void init(int vt);
    inline void calc();
    inline int update(const int id);
    inline void mark(const int id, const int mk);
    inline void mark_all(const int mk);
    inline void mark_term(const int id, const int mk);
    inline int check_all_neig_no_update();
    inline int check_all_child_term();
    inline int &operator[](const int &index);

    inline void wait_send(const int id, MPI_Status *st);
    inline void wait_recv(const int id, MPI_Status *st);
    inline void isend(int id, int tag);
    inline void irecv(int id, int tag);
    inline void recv(int id, int tag, MPI_Status *st);
    inline void isend_tag(int id, int tag);
    inline void irecv_tag(int id, int tag);
}

Map::Map(){
    parent=-1;
    not_done=1;
    has_update=1;
    back_term=0;
}

Map::~Map(){
    delete[] data;
    delete[] buf;
    delete[] node_update;
}

inline void Map::init(int vt){
    this->vt = vt;
    data = new int[vt];
    buf = new int[vt];
    node_update = new int[vt];

    neig.reserve(vt);
    chds.reserve(vt);
    term.reserve(vt);
    sendreq.resize(vt);
    recvreq.resize(vt);

    std::fill(data, data+v, INF);
    data[world_rank] = 0;
}

inline void Map::calc(){
    for(int i=0;i<vt;++i){
        if(i != world_rank && data[i] != INF)
            neig.push_back(i);
    }
    nb = neig.size();
    
    std::fill(node_update, node_update + vt, 1);

#ifdef _DEBUG_
    printf("Rank %d neig: ", world_rank);
    for(int i=0;i<nb;++i){
        printf("%d, ", neig[i]);
    }
    printf("\n");
#endif

}

inline int Map::update(const int id){
    int up = 0;
    for(int i=0;i<vt;++i){
        if(data[i] > data[id] + buf[i]){
            data[i] = data[id] + buf[i];
            up = 1;
        }
    }
    return up;
}

inline void Map::mark(const int id, const int mk){
    node_update[id] = mk;
}

inline void Map::mark_all(const int mk){
    for(int i=0;i<nb;++i){

        node_update[neig[i]] = mk;
    }
}

inline void Map::mark_term(const int id, const int mk){
    for(int i=0;i<map.chds.size();++i){
        if(id == map.chds[i])
            map.term[i] = mk;
    }
}

inline int Map::check_all_neig_no_update(){
    int up=0;
    for(int i=0;i<nb;++i){
        up |= node_update[neig[i]];
    }
    return !up;
}

inline int Map::check_all_child_term(){
    int tm=0;
    for(int i=0;i<map.chds.size();++i){
        tm |= map.term[i];
    }
    return tm;
}

inline int& Map::operator[](const int &index){
    return data[index];
}

inline void Map::wait_send(const int id, MPI_Status *st=NULL){
    if(st==NULL)
        MPI_Wait(&sendreq[id], MPI_STATUS_IGNORE);
    else
        MPI_Wait(&sendreq[id], st);
}

inline void Map::wait_recv(const int id, MPI_Status *st=NULL){
    if(st==NULL)
        MPI_Wait(&recvreq[id], MPI_STATUS_IGNORE);
    else
        MPI_Wait(&recvreq[id], st);
}

inline void Map::isend(int id, int tag){
    MPI_Isend(data, vt, MPI_INT, id, tag, COMM_GRAPH, &sendreq[id]);
}

inline void Map::irecv(int id, int tag){
    MPI_Irecv(buf, vt, MPI_INT, id, tag, COMM_GRAPH, &recvreq[id]);
}

inline void Map::recv(int id, int tag, MPI_Status *st=NULL){
    if(st == NULL)
        MPI_Recv(buf, vt, MPI_INT, id, tag, COMM_GRAPH, MPI_STATIS_IGNORE);
    else
        MPI_Recv(buf, vt, MPI_INT, id, tag, COMM_GRAPH, st);
}

inline void Map::isend_tag(int id, int tag){
    MPI_Isend(&NOTHING, 1, MPI_INT, id, tag, COMM_GRAPH, &sendreq[id]);    
}

inline void Map::irecv_tag(int id, int tag){
    int i;
    MPI_Irecv(&i, 1, MPI_INT, id, tag, COMM_GRAPH, &recvreq[id]);
}

//=^==^==^==^==^=   MAP   =^==^==^==^==^=//

Map map;

//=v==v==v==v==   File IO   ==v==v==v==v=//

inline void dump_from_file(const char *file){

    std::ifstream fin(file);
    std::stringstream ss;

    TIC;{
    ss << fin.rdbuf();

    }TOC_P(IO);

    ss >> vert >> edge;

    map.init(vert);

    int i, j, w;
    for(int e=0;e<edge;++e){
        ss >> i >> j >> w;
        if(i==world_rank)map.data[j]=w;
        else if(j==world_rank)map.data[i]=w;
    }
}


inline void dump_to_file(const char *file){
    std::stringstream ss;

    std::ostream_iterator<int> out(ss, ' ');
    std::copy(map.data, map.data+map.vt, out);
    ss << '\n';

    std::string str = ss.str();
    int *len = new int[map.vt]();
    len[world_rank] = str.size();

    MPI_File fout;
    MPI_File_open(MPI_COMM_WORLD, file, MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fout);


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

    delete [] len;
}

//=^==^==^==^==   File IO   ==^==^==^==^=//

//=v==v==v==v==v=  Utils  =v==v==v==v==v=//

inline void discard_tag(const MPI_Status &status){
    int i;
    MPI_Recv(&i, 1, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, COMM_GRAPH, MPI_STATUS_IGNORE);
}


inline void send_update_to_all(){
    for(int i=0;i<map.nb;++i){
        map.isend(map.neig[i], tags.upd);
    }
}

inline void send_tag_to_all(int tag){
    for(int i=0;i<map.nb;++i){
        map.isend_tag(map.neig[i], tag);
    }
}

inline void send_tag_to_child(int tag){
    for(int i=0;i<map.chds.size();++i){
        map.isend_tag(map.chds[i], tag);
    }
}

inline void send_term_to_parent(){
    map.isend_tag(map.parent, tags.term);
}


//=^==^==^==^==^=  Utils  =^==^==^==^==^=//

//=v==v==v==v== Initialize  ==v==v==v==v=//


inline void create_graph(){
    MPI_Dist_graph_create(MPI_COMM_WORLD, 1, &world_rank, &map.nb, map.neig.data(),
                                MPI_UNWEIGHTED, MPI_INFO_NULL, false, &COMM_GRAPH);
    MPI_Comm_rank(COMM_GRAPH, &graph_rank);
    assert(graph_rank == world_rank);
}

//only rank 0
inline void construct_tree_root(){
    map.parent = 0;

    for(int i=0;i<map.nb;++i){
        send_tag_to_all(tags.invt);
    }

#ifdef _DEBUG_
    printf("Rank %d: tree root created\n", graph_rank);
#endif
}


//=^==^==^==^== Initialize  ==^==^==^==^=//

//=v==v==v==v==v=  Tasks  =v==v==v==v==v=//


inline void do_construct_tree(const MPI_Status &status){
    //if get invitation
    if(status.MPI_TAG == tags.invt){

#ifdef _DEBUG_
        printf("Rank %d: get invitation from rank %d\n", graph_rank, status.MPI_SOURCE);
#endif
        
        discard_tag(status);

        //check if already joined to another party
        if(map.parent != -1){

#ifdef _DEBUG_
            printf("Rank %d: reject %d / parent=%d\n", graph_rank, status.MPI_SOURCE, map.parent);
#endif
            //send reject
            map.isend_tag(status.MPI_SOURCE, tags.rej);

        }else{

            printf("Rank %d: join %d\n", graph_rank, status.MPI_SOURCE);

            //send join request
            map.parent = status.MPI_SOURCE;
            map.isend_tag(status.MPI_SOURCE, tags.join);

            //send invitation to all neighbors
            for(int i=0;i<map.nb;++i){
                if(map.neig[i] != map.parent)
                    map.isend_tag(map.neig[i], tags.invt);
            }
        }

    }else if(status.MPI_TAG == tags.join){
        //get request
        discard_tag(status);
        map.chds.push_back(status.MPI_SOURCE);
        map.term.push_back(0);
    }else if(status.MPI_TAG == tags.rej){
        //get reject
        discard_tag(status);
    }
}

inline void do_update(const MPI_Status &status){
    map.recv(status.MPI_SOURCE, status.MPI_TAG);

    int has_update = map.update(status.MPI_SOURCE);
    if(!has_update){
        map.isend_tag(status.MPI_SOURCE, tag.nupd);
        map.mark(status.MPI_SOURCE, 0);
    }

    map.has_update |= has_update;
}

inline void do_no_update(const MPI_Status &status){
    discard_tag(status);
    map.mark(status.MPI_SOURCE, 0);
}

inline void do_terminate(const MPI_Status &status){
    discard_tag(status);
    if(status.MPI_SOURCE == map.parent){
        map.term_flag = term_flag.fwd;
    }else{
        map.mark_term(status.MPI_SOURCE, 1);
        if(map.check_all_child_term()){
            if(graph_rank == 0){
                send_tag_to_child(tags.stop);
                map.not_done = 0;
            }
            else
                map.term_flag = term_flag.back;
        }
    }
}

inline void listen_(){

    MPI_Status status;
    int flag=0;
    while(map.not_done){
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, COMM_GRAPH, &flag, &status);
        if(flag){
            switch(status.MPI_TAG){
                case tags.invt:
                case tags.join:
                case tags.rej:
                    do_construct_tree(status);
                    break;
                case tags.upd:
                    do_update(status);
                    break;
                case tags.nupd:
                    do_no_update(status);
                    break;
                case tags.term:
                    do_terminate(status):
                    break;
                case tags.done:
                    map.not_done = 0;
                    break;
                default:
#ifdef _DEBUG_
                    printf("Rank %d: Unrecognized Tags Received: [%d]\n", graph_rank, status.MPI_TAG);
#endif
                    break;
            }
        }else{
            if(map.has_update){
                send_update_to_all();
                map.mark_all(1);
                map.has_update = 0;
            }else if(map.term_flag == term_flag.fwd && map.check_all_neig_no_update()){
                send_tag_to_child(tags.term);
            }else if(map.term_flag == term_flag.back && map.check_all_neig_no_update()){
                send_term_to_parent();
                map.term_flag = term_flag.none;
            }
        }
    }
}

//=^==^==^==^==^=  Tasks  =^==^==^==^==^=//


int main(int argc, char **argv){

    assert(argc == 4);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    
    TIME(ST);
    dump_from_file(argv[1]);

#ifdef _DEBUG_
    printf("Rank %d: file read\n", world_rank);
#endif

    map.calc();

    create_graph();

    if(graph_rank ==0)
        construct_tree_root();

    listen_();
    
    dump_to_file(argv[2]);


#ifdef _MEASURE_TIME
    TIME(ED);
    EXE = ED - ST;
    //rank, EXE, CALC, IO, COMM, PROC
    printf("%d, %lf, %lf, %lf, %lf, %lf\n", world_rank, EXE, CALC, IO, COMM, EXE-CALC-IO-COMM);
#endif

    MPI_Finalize();
    return 0;
}







