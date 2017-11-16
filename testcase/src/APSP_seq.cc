#include <iostream>
#include <pthread.h>
#include <algorithm>
#include <assert.h>
#include <fstream>

#define UNK 999999999
#define MIN(x, y) ((x)<(y)?(x):(y))

int main(int argc, char **argv){

    // check for input argument count
    assert(argc==4);

    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);

    int v, e;
    fin >> v >> e;
    
    int *ln = new int[v*v];
    std::fill(ln, ln+v*v, UNK);

    int **map = new int*[v];

    for(int i=0;i<v;i++){
        map[i] = &ln[i*v];
        map[i][i] = 0;
    }

    while(e--){
        int i, j, w;
        fin >> i >> j >> w;
        map[i][j] = map[j][i] = w;
    }

    for(int k=0;k<v;k++)
        for(int i=0;i<v;i++)
            for(int j=0;j<v;j++)
                map[i][j] = MIN(map[i][k]+map[k][j], map[i][j]);
    

    for(int i=0;i<v;i++){
        for(int j=0;j<v;j++){
            fout << map[i][j] << " ";
        }
        fout << std::endl;
    }

    delete [] map;
    delete [] ln;

    fin.close();
    fout.close();

    return 0;
}
