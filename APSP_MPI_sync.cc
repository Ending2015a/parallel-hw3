#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX(x, y) ((x)>(y)?(x):(y))
#define MIN(x, y) ((x)<(y)?(x):(y))


#define INF 999999999

int world_size;
int world_rank;

int vert;
int edge;

struct Map{
    int *data;
    Map(){}
    ~Map(){ delete [] data; }

    inline void init(int v){
        data = new int[v];
        std::fill(data, data+v, INF);
        data[world_rank]=0;
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
        if(j==world_rank)map.data[i] = w;
        else if(i==world_rank)map.data[j] = w;
    }
}

int main(int argc, char **argv){

    assert(argc == 4);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);    

    dump_from_file(argv[1]);




    MPI_Finalize();
    return 0;
}
