#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

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
int graph_rank;

int vert;
int edge;

MPI_Comm graph_comm;


struct Map{
    int *data;
    std::vector<int> neig;
    int vt;
    int eg;
    int nb;
    Map(){}
    ~Map(){
        delete [] data;
    }

    inline void init(int v){
        vt = v; eg = 0;
        data = new int[v];
        std::fill(data, data+v, INF);
        data[world_rank]=0;
        neig.reserve(vt);
    }

    inline void calc(){
        for(int i=0;i<vt; ++i){
            if(i != world_rank && data[i] != INF)
                neig.push_back(i);
        }

        nb = neig.size();

#ifdef _DEBUG_
        printf("Rank %d neig: ", world_rank);
        for(int i=0;i<nb;++i){
            printf("%d, ", neig[i]);
        }
        printf("\n");
#endif
    }

    inline int update(int id, int *buf){
        int up = 0;
        for(int i=0;i<vt;++i){
            if(data[i] > data[id] + buf[i]){
                data[i] = data[id] + buf[i];
                up = 1;
            }
        }
        return up;
    }

    inline int &operator[](const int &index){
        return data[index];
    }
};

Map map;

int done=0;

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

inline void floyd(){

    MPI_Request *send_req = new MPI_Request[map.nb];
    int *buf = new int[map.vt];

    int not_done = 1;
    while(not_done){
        not_done = 0;
        for(int i=0;i<map.nb;++i){
            MPI_Isend(map.data, map.vt, MPI_INT, map.neig[i], 1, MPI_COMM_WORLD, &send_req[i]);

#ifdef _DEBUG_
            printf("Rank %d: send to %d\n", world_rank, map.neig[i]);
#endif

        }
        for(int i=0;i<map.nb;++i){
            TIC;{

            MPI_Recv(buf, map.vt, MPI_INT, map.neig[i], 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
 
            }TOC_P(COMM);
            TIC;{

            not_done |= map.update( map.neig[i], buf );

            }TOC_P(CALC);
        }
        TIC;{
        MPI_Allreduce(MPI_IN_PLACE, &not_done, 1, MPI_INT, MPI_LOR, MPI_COMM_WORLD);
        }TOC_P(COMM);
    }

    delete [] send_req;
    delete [] buf;
}

inline void dump_to_file(const char *file){
    std::stringstream ss;
    for(int i=0;i<map.vt;++i){
        ss << map.data[i] << ' ';
    }
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

int main(int argc, char **argv){

    assert(argc == 4);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    
    TIME(ST);
    dump_from_file(argv[1]);


    map.calc();

    floyd();
    
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
