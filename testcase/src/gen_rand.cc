#include <iostream>
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MIN(x, y) ((x)<(y)?(x):(y))
#define MAX(x, y) ((x)>(y)?(x):(y))

#define MAX_VERTEX 2000

int graph[MAX_VERTEX][MAX_VERTEX]={};


bool check(int edge[][2], int idx){

    int i = edge[idx][0];
    int j = edge[idx][1];

    if(graph[i][j] == 1)
        return false;
    graph[i][j] = graph[j][i] = 1;
    return true;
}

bool check_end(int vex[], int sz){
    for(int i=0;i<sz;++i)
        if(vex[i] != 0)
            return false;
    return true;
}

void GenerateRandGraphs(int NOE, int NOV, std::mt19937 &gen, char* out)
{
    std::uniform_int_distribution<int> genv(0, NOV-1), genw(0, 100);

    int vex[NOV];

    bool conn_all=false;

    for(int i=0;i<NOV;++i)
        vex[i] = i;
    
    int idx=0, edge[NOE][2], count;
    

    std::cout << "generating..." << std::endl;

    while(idx < NOE)
    {
        edge[idx][0] = genv(gen);
        edge[idx][1] = genv(gen);

        if(edge[idx][0] == edge[idx][1])
            continue;
        else if(!conn_all && vex[ edge[idx][0] ] == vex[ edge[idx][1] ]){
            continue;
        }else{
            if(!check(edge, idx))
                continue;

            if(!conn_all){
                int mn = MIN(vex[edge[idx][0]], vex[edge[idx][1]]);
                int mx = MAX(vex[edge[idx][0]], vex[edge[idx][1]]);
            
                for(int n=0;n<NOV;++n){
                    if(vex[n] == mx)
                        vex[n] = mn;
                }

                conn_all = check_end(vex, NOV);
            }
        }
        idx++;
    }

    std::cout << "verifying...." << std::endl;

    for(int i=0;i<NOV;++i){
        if(vex[i] != 0)
            std::cout << i << ": " << vex[i] << std::endl;
    }

    //std::cout << "The generated random random graph is: " << std::endl;
    for(int i = 0; i < NOV; i++)
    {
        count = 0;
        //std::cout << "\t" << i << "-> { ";
        for(int j = 0; j < NOE; j++)
        {
            if(edge[j][0] == i)
            {
                //std::cout << edge[j][1] << "   ";
                count++;
            }
            else if(edge[j][1] == i)
            {
                //std::cout << edge[j][0] << "   ";
                count++;
            }
            else if(j == NOE-1 && count == 0){
                std::cout << "Isolated Vertex!" << std::endl;
                exit(-1);
            }
        }
        //std::cout << " }" << std::endl;
    }

    std::ofstream fout(out);

    std::stringstream ss;

    ss << NOV << ' ' << NOE << '\n';


    for(int i=0;i<NOE;++i){
        ss << edge[i][0] << ' ' << edge[i][1] << ' ' << genw(gen) << '\n';
    }

    fout << ss.rdbuf();
    fout.close();

    std::cout << "generated!" << std::endl;
}
 
int main(int argc, char **argv){
    
    // V, E, file
    assert(argc == 4);

    std::random_device dev;
    std::mt19937 gen(dev());

    int n, i, e, v;
 
    std::cout << "Random graph generator: ";
 
    v = atoi(argv[1]);

    std::cout << "The graph has " << v << " vertexes." << std::endl;

    e = MIN(atoi(argv[2]), (v*(v-1))/2);
    std::cout << "The graph has " << e << " edges." << std::endl;

    GenerateRandGraphs(e, v, gen, argv[3]);

    return 0;
}




