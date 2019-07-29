//
// Created by sjef on 11-7-19.
//

#include "MBR.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <queue>
#include <chrono>
#include "FPIte.h"

vector<Subset> MBR::winningConf;
vector<VertexSet> MBR::winningVertices;
bool MBR::metric_output;
string MBR::metric_dir;
MBR::MBR(Game *game, Subset *conf,  VertexSet *P0, VertexSet *VP1, int feature) {
    this->game = game;
    this->conf = conf;
    this->P0 = P0;
    this->VP1 = VP1;
    this->feature = feature;
}

MBR::MBR(Game *game) {
    this->game = game;
    this->conf = new Subset;
    *this->conf = game->bigC;

    this->P0 =  new VertexSet();
    this->P0->resize(game->n_nodes);
    this->VP1 =  new VertexSet();
    this->VP1->resize(game->n_nodes);
    fill(this->VP1->begin(), this->VP1->end(), true);
    this->feature =  0;
}

void MBR::solve() {
    if(metric_output){
        if(this->measured == nullptr)
            this->measured = new bintree<vector<int>>();
        this->measured->value = new vector<int>(5);
        (*this->measured->value)[4] = confstring;

#ifdef subsetbdd
        (*this->measured->value)[0] = std::count_if(P0->begin(), P0->end(), [](bool b){return b;});
        (*this->measured->value)[1] = game->n_nodes - std::count_if(VP1->begin(), VP1->end(), [](bool b){return b;});
#endif
    }
    if(this->feature == game->bm_n_vars){
        int i = winningConf.size();
        winningConf.resize(i+1);
        winningVertices.resize(i+1);
        winningConf[i] = *conf;
        winningVertices[i].resize(game->n_nodes);
        FPIte fpite(game, P0, VP1, &(winningVertices[i]));
        fpite.solvelocal = solvelocal;

        if(metric_output)
        {
            auto start = std::chrono::high_resolution_clock::now();
            fpite.solve();
            auto end = std::chrono::high_resolution_clock::now();
            (*this->measured->value)[2] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            cout << "Assisted leaf " << (*this->measured->value)[0] + (*this->measured->value)[1] << " with time " << (*this->measured->value)[2] << endl;
        } else {
            fpite.solve();
        }
        return;
    }


    auto * subgame_out0 = new vector<std::tuple<int, int>>[game->n_nodes];
    auto * subgame_in0  = new vector<std::tuple<int, int>>[game->n_nodes];
    auto * subgame_out1 = new vector<std::tuple<int, int>>[game->n_nodes];
    auto * subgame_in1  = new vector<std::tuple<int, int>>[game->n_nodes];
    copyEdges(subgame_out0, subgame_in0);
    copyEdges(subgame_out1, subgame_in1);
    vector<std::tuple<int, int>> * orgin, * orgout;
    orgin = game->in_edges;
    orgout = game->out_edges;

    createPessimisticGames(subgame_out0, subgame_in0, subgame_out1, subgame_in1);

    VertexSet W0(game->n_nodes);
    auto * P0b = new VertexSet;
    auto * VP1b = new VertexSet;
    game->in_edges = subgame_in0;
    game->out_edges = subgame_out0;


    auto *fpite0 = new FPIte(game, P0, VP1, &W0);
    if (metric_output) {
        auto start = std::chrono::high_resolution_clock::now();
        fpite0->solve();
        auto end = std::chrono::high_resolution_clock::now();
        (*this->measured->value)[2] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//            if(std::count_if(W0.begin(), W0.end(), [](bool b){return b;}) - std::count_if(P0->begin(), P0->end(), [](bool b){return b;}) < 3000)
//        if(this->feature != 0)
//            (*this->measured->value)[2] = 0;
            if(metric_dir.length() > 0){
                ofstream f;
                f.open(metric_dir + '/' + to_string(confstring) + "_0.pg");
                game->writePG(&f);
                f.close();
            }
        cout << "Assisted node " << (*this->measured->value)[0] + (*this->measured->value)[1] << " with time " << (*this->measured->value)[2] << endl;
    } else {
        fpite0->solve();
    }
    *P0 = W0;

    game->in_edges = subgame_in1;
    game->out_edges = subgame_out1;
    auto *fpite1 = new FPIte(game, P0, VP1, &W0);
    if (metric_output) {
        auto start = std::chrono::high_resolution_clock::now();
        fpite1->solve();
        auto end = std::chrono::high_resolution_clock::now();
        (*this->measured->value)[3] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
//            if(std::count_if(VP1->begin(), VP1->end(), [](bool b){return b;}) - std::count_if(W0.begin(), W0.end(), [](bool b){return b;}) < 3000)
//        if(this->feature != 0)
//            (*this->measured->value)[3] = 0;
        if(metric_dir.length() > 0) {
            ofstream f;
            f.open(metric_dir + '/' + to_string(confstring) + "_1.pg");
            game->writePG(&f);
            f.close();
        }
        cout << "Assisted node " << (*this->measured->value)[0] + (*this->measured->value)[1] << " with time " << (*this->measured->value)[3] << endl;
    } else {
        fpite1->solve();
    }
    *VP1 = W0;

    delete fpite0;
    delete fpite1;

    game->in_edges = orgin;
    game->out_edges = orgout;
    delete[] subgame_in1;
    delete[] subgame_out1;

    bool done;
    if(this->solvelocal){
        //Local:
        done = (*P0)[game->reindexedNew[0]] || !(*VP1)[game->reindexedNew[0]];
    } else {
        //Global:
        done = *P0 == *VP1;
    }
    if (done) {
        int i = winningConf.size();
        winningConf.resize(i + 1);
        winningVertices.resize(i + 1);
        winningConf[i] = *conf;
        winningVertices[i] = *P0;
        return;
    }

    int confastring = confstring;
    int confbstring = confstring;
    auto * confa = new Subset;
    auto * confb = new Subset;
    do {
        *confa = *conf;
        parition(confa, confb);
        confastring *= 10;
        confastring += 1;
        confbstring *= 10;
        this->feature++;
        if(this->feature >  game->bm_n_vars)
            throw "Empty conf sets found in partitioning";
    } while (*confa == emptyset || *confb == emptyset);

    copyEdges(subgame_out0, subgame_in0);
    createSubGames(confa, subgame_out0, subgame_in0);
    createSubGames(confb, orgout, orgin);

    *P0b = *P0;
    *VP1b = *VP1;
    MBR ma(game, confa, P0, VP1, feature);
    MBR mb(game, confb, P0b, VP1b, feature);
    if(metric_output){
        this->measured->left = new bintree<vector<int>>();
        this->measured->right = new bintree<vector<int>>();
        ma.measured = this->measured->left;
        mb.measured = this->measured->right;
        ma.confstring = confastring;
        mb.confstring = confbstring;
    }
    ma.solvelocal = solvelocal;
    mb.solvelocal = solvelocal;
    game->out_edges = subgame_out0;
    game->in_edges = subgame_in0;
    ma.solve();
    delete[] subgame_out0;
    delete[] subgame_in0;
    game->out_edges = orgout;
    game->in_edges = orgin;
    mb.solve();

    delete P0b;
    delete VP1b;
}

//void MBR::createPessimisticGames(vector<bool> *pessimisticedges0, vector<bool> *pessimisticedges1) {
//    vector<bool> * pess;
//    vector<bool> * opt;
//    for(int i = 0;i<edgeenabled->size();i++){
//        if(!(*edgeenabled)[i])
//            continue;
//        if(game->owner[game->edge_origins[i]] == 0){
//            pess = pessimisticedges0;
//            opt = pessimisticedges1;
//        } else{
//            pess = pessimisticedges1;
//            opt = pessimisticedges0;
//        }
//        (*opt)[i] = true;
//        Subset g = *conf;
//        g -= game->edge_guards[i];
//        (*pess)[i] = g == emptyset;
//    }
//}

void MBR::parition(Subset *org, Subset *part) {
    *part = *org;
    *org &= game->bm_vars[feature];
    *part -= game->bm_vars[feature];
}
//
//void MBR::createSubGames(Subset *confP, vector<bool> *edgeenabledP) {
//    for(int i = 0;i<edgeenabled->size();i++) {
//        if (!(*edgeenabled)[i])
//            continue;
//        Subset g = *confP;
//        g &= game->edge_guards[i];
//        (*edgeenabledP)[i] = !(g == emptyset);
//    }
//}

void MBR::printMeasurements(ostream * output) {
    *output << "digraph timetree {" << endl;
    int nodes = printNode(output, this->measured,0);
    *output << '}' << endl;
    cout << "nr of nodes: " << nodes << endl;
    cout << "Solved " << ((nodes+1)*3/2-2) << " games" << endl;
}

int MBR::printNode(ostream *output, bintree<vector<int>> *node, int c) {
    if(node == nullptr)
        return -1;
    *output << 'n' << c++ << " [label=\"";
    *output << "V0: " << (*node->value)[0] << '/' << this->game->n_nodes << endl;
    *output << "V1: " << (*node->value)[1] << '/' << this->game->n_nodes << endl;
    *output << "Solve 1: " << (*node->value)[2] << "ns" << endl;
    *output << "Solve 2: " << (*node->value)[3] << "ns" << endl;
    *output << "Conf: " << (*node->value)[4];
    *output << "\"];" << endl;
    int l = printNode(output, node->left, c);
    if(l == -1) {
        int r = printNode(output, node->right, c);
        if(r != -1){
            *output << 'n' << (c-1) << " -> n" << c << ';' << endl;
            return r;
        }
        return c;
    } else {
        int r = printNode(output, node->right, l);
        *output << 'n' << (c-1) << " -> n" << c << ';' << endl;
        if(r != -1){
            *output << 'n' << (c-1) << " -> n" << l << ';' << endl;
            return r;
        }
        return l;
    }
}

void MBR::createPessimisticGames(vector<std::tuple<int, int>> *pessimistic_out0,
                                 vector<std::tuple<int, int>> *pessimistic_in0,
                                 vector<std::tuple<int, int>> *pessimistic_out1,
                                 vector<std::tuple<int, int>> *pessimistic_in1) {
    vector<bool> edgeenabled0(game->edge_guards.size()),edgeenabled1(game->edge_guards.size()), *pess, *opt;
    for(int v = 0;v<game->n_nodes;v++){
        for(const auto & e : game->out_edges[v]) {
            if (game->owner[v] == 0) {
                pess = &edgeenabled0;
                opt = &edgeenabled1;
            } else {
                pess = &edgeenabled1;
                opt = &edgeenabled0;
            }
            int gi = guard_index(e);
            (*opt)[gi] = true;
            Subset g = *conf;
            g -= game->edge_guards[gi];
            (*pess)[gi] = g == emptyset;
        }
    }
    for(int v = 0;v<game->n_nodes;v++) {
        removeDisabledEdge(pessimistic_out0 + v, &edgeenabled0);
        removeDisabledEdge(pessimistic_in0 + v, &edgeenabled0);
        removeDisabledEdge(pessimistic_out1 + v, &edgeenabled1);
        removeDisabledEdge(pessimistic_in1 + v, &edgeenabled1);
    }
}

void MBR::removeDisabledEdge(vector<std::tuple<int,int>> * edge, vector<bool> * edgeenabled){
    auto it = edge->begin();
    while(it != edge->end()){
        int guard_index = guard_index(*it);
        if(!(*edgeenabled)[guard_index]){
            it = edge->erase(it);
        } else {
            ++it;
        }
    }
}

void MBR::createSubGames(Subset *confP, vector<std::tuple<int, int>> *subgame_out,
                         vector<std::tuple<int, int>> *subgame_in) {
    vector<bool> edgeenabled(game->edge_guards.size());
    for(int v = 0;v<game->n_nodes;v++) {
        for (const auto & e : game->out_edges[v]) {
            Subset g = *confP;
            int gi = guard_index(e);
            g &= game->edge_guards[gi];
            edgeenabled[gi] = !(g == emptyset);
        }
    }
    for(int v = 0;v<game->n_nodes;v++) {
        removeDisabledEdge(subgame_out+v, &edgeenabled);
        removeDisabledEdge(subgame_in+v, &edgeenabled);
    }
}

void MBR::copyEdges(vector<std::tuple<int, int>> *edgeout, vector<std::tuple<int, int>> *edgein) {
    copy(game->in_edges, game->in_edges + game->n_nodes, edgein);
    copy(game->out_edges, game->out_edges + game->n_nodes, edgeout);
}

