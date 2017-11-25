//cpp header
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <string>
#include <streambuf>

//c header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

//const
#define UNK 999999999
#define MAX_VERTEX 2000
#define PTHREAD_LIMIT 2000

//func
#define MIN(x, y) ((x)<(y)?(x):(y))
#define MAX(x, y) ((x)>(y)?(x):(y))


#define parallel_output


int vert;
int edge;


int num_threads;
pthread_t *threads;
int *ID;

int valid_size;


pthread_barrier_t barr;

///////////////////////////////////////////////
struct Map{
    int *data;
    int **ptr;
    std::stringstream *oss;
    
    Map(){}
    ~Map(){ 
        delete [] data;
        delete [] ptr;
        delete [] oss;
    }

    inline void init(const int &v){
        data = new int[v*v];
        ptr = new int*[v];
        oss = new std::stringstream[v];
        std::fill(data, data+v*v, UNK);

        for(int i=0;i<vert;++i){
            ptr[i] = &data[i*v];
            ptr[i][i] = 0;
        }

    }

    inline int* operator[](const size_t &index){
        return ptr[index];
    }
};
///////////////////////////////////////////////

Map map;


inline void dump_from_file(char *file){

    std::ifstream fin(file);
    std::stringstream ss;

    ss << fin.rdbuf();

    ss >> vert >> edge;

    map.init(vert);

    for (int e=0;e<edge;++e){
        int i,j,w;
        ss >> i >> j >> w;
        map[i][j] = map[j][i] = w;
    }
}



inline void dump_to_file(char *file){

    std::stringstream ss;

    int *iter=map.data;
    for(int i=0;i<vert;++i){
        for(int j=0;j<vert;++j){
            ss << *iter << ' ';
            ++iter;
        }
        ss << '\n';
    }
    

    std::ofstream fout(file);

    fout << ss.rdbuf();
    fout.close();
}

inline void parallel_dump_to_file(const int &id){
    for(int i=id;i<vert;i+=valid_size){
        for(int j=0;j<vert;++j){
            map.oss[i] << map[i][j] << ' ';
        }
        map.oss[i] << '\n';
    }
}


void *task(void* var){
    
    int id = * ((int*)var);
    /*
    int dv = vert / valid_size;
    int rm = vert % valid_size;
    int bn = (id < rm) ? 1:0;

    int sz = bn + dv;
    int st = dv * id + (bn ? id:rm);
    int ed = st + sz;
    */

    for(int k=0;k<vert;++k){
        for(int i=id;i<vert;i+=valid_size){
            for(int j=0;j<vert;++j){
                map[i][j] = MIN(map[i][k]+map[k][j], map[i][j]);
            }
        }

        pthread_barrier_wait(&barr);
    }
    
#ifdef parallel_output
    parallel_dump_to_file(id);
#endif

    return NULL;    
}

int main(int argc, char **argv){

    // check for argument count
    assert(argc == 5);

    num_threads = MIN(atoi(argv[3]), atoi(argv[4]));
    threads = new pthread_t[num_threads];
    ID = new int[num_threads];

    dump_from_file(argv[1]);
    valid_size = (vert < num_threads) ? vert:num_threads;

    pthread_barrier_init(&barr, NULL, valid_size);

    // parallel region
    {
        for(int i=0;i<valid_size;++i){
            ID[i] = i;
            pthread_create(&threads[i], NULL, task, (void*)&ID[i]);
        }

        for(int i=0;i<valid_size;++i){
            pthread_join(threads[i], NULL);
        }

    }

    pthread_barrier_destroy(&barr);

#ifdef parallel_output
    std::ofstream fout(argv[2]);
    for(int i=1;i<vert;++i){
        map.oss[0] << map.oss[i].rdbuf();
    }

    fout << map.oss[0].rdbuf();
    fout.close();
#else
    dump_to_file(argv[2]);
#endif

    delete [] threads;
    delete [] ID;

    return 0;
}
