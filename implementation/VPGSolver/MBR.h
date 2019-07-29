//
// Created by sjef on 11-7-19.
//

#ifndef VPGSOLVER_MBR_H
#define VPGSOLVER_MBR_H


#include "Game.h"
#include "conf.h"
#include "Measurements/bintree.h"
#include <vector>
#include <iostream>
using namespace std;

class MBR {
public:
    static vector<Subset> winningConf;
    static vector<VertexSet> winningVertices;
    Game * game;
    Subset * conf;
    VertexSet * P0;
    VertexSet * VP1;
    int feature;

    bintree<vector<int>> *measured = nullptr;
    bool solvelocal = false;
    int confstring = 1;

    static bool metric_output;
    static string metric_dir;
    MBR(Game * game);
    MBR(Game * game, Subset * conf, VertexSet * P0, VertexSet * P1, int feature);

    void solve();
    void parition(Subset * org, Subset * part);
    void createSubGames(Subset * confP, vector<std::tuple<int,int>> * subgame_out, vector<std::tuple<int,int>> * subgame_in);
    void createPessimisticGames(vector<std::tuple<int,int>> * pessimistic_out0, vector<std::tuple<int,int>> *pessimistic_in0, vector<std::tuple<int,int>> *pessimistic_out1, vector<std::tuple<int,int>> *pessimistic_in1);
    void copyEdges(vector<std::tuple<int,int>> * edgeout, vector<std::tuple<int,int>> * edgein);
    void printMeasurements(ostream * output);
    int printNode(ostream * output,bintree<vector<int>> * node, int c);

    void removeDisabledEdge(vector<tuple<int, int>> *edge, vector<bool> *edgeenabled);
};


#endif //VPGSOLVER_MBR_H
