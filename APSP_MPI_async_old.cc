#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mpi.h>

#define MAX(x, y) ((x)>(y)?(x):(y))
#define MIN(x, y) ((x)<(y)?(x):(y))


#define INF 999999999


#define _DEBUG_

int world_size;
int world_rank;
int graph_rank;

int vert;
int edge;

MPI_Comm graph_comm;


struct Map{
    int *full;
    int *data;
    int **ptr;
    int *index;
    int *edge;
    int *neighbor;
    int vt;
    int eg;
    Map(){}
    ~Map(){
        delete [] full;
        delete [] ptr;
        delete [] index;
        delete [] edge;
    }

    inline void init(int v){
        vt = v; eg = 0;
        full = new int[v*v];
        std::fill(full, full+v*v, INF);
        data = full + world_rank*v;
        data[world_rank]=0;

        ptr = new int*[v];
        for(int i=0;i<v;++i){
            ptr[i] = &full[i*v]; 
        }
        
        index = new int[vt]();
        edge = new int[vt*vt]();
    }

    inline void print_map(){
        std::stringstream ss;
    
        ss << "Rank " << world_rank << ": \n";
        for(int i=0;i<vt;++i){
            for(int j=0;j<vt;++j){
                ss << ptr[i][j] << " "; 
            }
            ss << "\n";
        }

        std::cout << ss.rdbuf();
    }

    inline void calc_list(){
        for(int i=0;i<vt;++i){
            if(i>0)index[i] = index[i-1];
            for(int j=0;j<vt;++j){
                if(i!=j && ptr[i][j] != INF){
                    edge[index[i]]=j;
                    ++eg;
                    ++index[i];
                }
            }
        }
    }

    inline int &operator[](const int &index){
        return data[index];
    }
};

Map map;

int done=0;

void dump_from_file(char *file){
    std::ifstream fin(file);
    std::stringstream ss;

    ss << fin.rdbuf();

    ss >> vert >> edge;

    map.init(vert);

    int i, j, w;
    for(int e=0;e<edge;++e){
        ss >> i >> j >> w;
        map.ptr[i][j] = map.ptr[j][i] = w;
    }
}

void create_graph(){

    map.calc_list();

#ifdef _DEBUG_
    printf("Rank %d index: ", world_rank);
    for(int i=0;i<map.vt;++i){
        printf("%d, ", map.index[i]);
    }
    printf("\nRank %d edge: ", world_rank);
    for(int i=0;i<map.eg;++i){
        printf("%d, ", map.edge[i]);
    }
    printf("\n");
#endif


    MPI_Graph_create(MPI_COMM_WORLD, world_size, map.index, map.edge, true, &graph_comm);

    int topo_t;
    MPI_Topo_test(graph_comm, &topo_t);

    if(topo_t == MPI_GRAPH){
        printf("Rank %d: Graph created!!\n", world_rank);
        MPI_Comm_rank(graph_comm, &graph_rank);
        printf("Rank %d: new rank is %d\n", world_rank, graph_rank);
        MPI_Graph_neighbors(graph_comm, graph_rank, map.vt, map.neighbor);
    }
}

void floyd_graph(){
    int not_done = 1;
    while(not_done){
        not_done = 0;
        
    }
}

int main(int argc, char **argv){

    assert(argc == 4);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);    

    dump_from_file(argv[1]);

    create_graph();

    floyd_graph();
    

    MPI_Finalize();
    return 0;
}
