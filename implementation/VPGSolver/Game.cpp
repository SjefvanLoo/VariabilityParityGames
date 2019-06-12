//
// Created by sjef on 5-6-19.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unordered_set>
#include "Game.h"

void Game::set_n_nodes(int nodes) {
    n_nodes = nodes;
    out_edges = new std::vector<std::tuple<int,int>>[n_nodes];
    in_edges = new std::vector<std::tuple<int,int>>[n_nodes];
    priority.resize(n_nodes);
    owner.resize(n_nodes);
    declared.resize(n_nodes);
    for(int i = 0;i<n_nodes;i++)
        declared[i] = false;
}


Game::Game() {

}


void Game::parseGameFromFile(const string &filename, const char* specificconf) {
    int c = 0;
    std::ifstream infile(filename);

    while (infile.good())
    {
        char s[PARSER_LINE_SIZE];
        infile.getline(s, PARSER_LINE_SIZE,';');
        if(c == 0){
            // create bigC
            cout << "Found confs: " << s << '\n';
            parseConfs(s);
            if(strlen(specificconf) > 0)
                parseConfSet(specificconf,0,&bigC);
            c++;
        } else if(c == 1)
        {
            cout << "Found game: " << s << '\n';
            // init with parity
            parseInitialiser(s);
            c++;
        } else {
            // add vertex
            if(strlen(s) > 0)
                parseVertex(s);
        }
    }
    if (!infile.eof()) {
        throw std::string("could not open file");
    }
}

void Game::parseGameFromFile(const string& filename) {
    parseGameFromFile(filename, "");
}

void Game::parseConfs(char * line) {
    while(*line == '\n' || *line == '\t' ||*line == ' ')
        line++;
    if(strncmp(line, "confs ",6) != 0) throw std::string("Expected confs");
    char conf[PARSER_LINE_SIZE];
    int i = 6;
    char c;
    do{
        c = line[i++];
    } while(c != '\0' && c != '+');
    bm_n_vars = i - 7;
    bm_vars.resize(bm_n_vars);
#ifdef subsetbdd
    bdd_init(100000,100000);
    bdd_setvarnum(bm_n_vars);
    for(int i = 0;i<bm_n_vars;i++) {
        bm_vars[i] = bdd_ithvar(i);
    }
#endif
#ifdef subsetexplicit
    for(int i =0;i<1<<bm_n_vars;i++)
        fullset.items.insert(i);
    for(int i = 0;i<bm_n_vars;i++) {
        bm_vars[i] = SubsetExplicit((1 << (bm_n_vars))-1, bm_n_vars-i-1);
    }
#endif
    parseConfSet(line, 6, &bigC);
}

void Game::parseInitialiser(char *line) {
    while(*line == '\n' || *line == '\t' ||*line == ' ')
        line++;
    if(strncmp(line, "parity ",7) != 0) throw std::string("Expected parity");
    int parity = atoi(line + 7);
    set_n_nodes(parity);
}


int Game::parseConfSet(const char *line, int i, Subset *result) {
    *result = emptyset;
    Subset entry = fullset;
    int var = 0;
    char c;
    do
    {
        c = line[i++];
        if(c == '0'){
            if(var > bm_n_vars) throw std::string("Too many bits");
            entry -= bm_vars[var];
            var++;
        } else if(c == '1'){
            if(var > bm_n_vars) throw std::string("Too many bits");
            entry &= bm_vars[var];
            var++;
        } else if(c == '-'){
            if(var > bm_n_vars) throw std::string("Too many bits");
            var++;
        } else {
            *result |= entry;
            entry = fullset;
            var = 0;
        }
    } while(c =='0' || c == '1' || c == '-' || c == '+');
    return i;
}

void Game::dumpSet(Subset * dumpee, Subset t, char * p, int var) {
    if(var == bm_n_vars)
    {
        p[var]  = '\0';
        Subset result = *dumpee;
        result &= t;
        if(!(result == emptyset)){
            cout << p << ',';
        }
    } else {
        Subset t1 = t;
        t1 &= bm_vars[var];
        p[var] = '1';
        dumpSet(dumpee, t1, p , var + 1);
        p[var] = '0';
        Subset t2 = t;
        t2 -= bm_vars[var];
        dumpSet(dumpee, t2, p, var + 1);
    }
}

void Game::parseVertex(char *line) {
    while(*line == '\n' || *line == '\t' ||*line == ' ')
        line++;
    int index;
    int i;
    i = readUntil(line, ' ');
    index = atoi(line);
    if(declared[index]) throw std::string("Already declared vertex " + std::to_string(index));
    line += i + 1;
    i = readUntil(line, ' ');
    priority[index] = atoi(line);
    line += i + 1;
    i = readUntil(line, ' ');
    owner[index] = atoi(line);
    line += i + 1;


    cout << "\nVertex with index: " << index << " and prio: " << priority[index] << " and owner: " << owner[index] << "\n";
    while(*line != '\0')
    {
        if(*line == ',')
            line++;
        i = readUntil(line, '|');
        int target = atoi(line);
        line += i + 1;

        int guardindex = edge_guards.size();
        edge_guards.resize(guardindex + 1);
        i = parseConfSet(line, 0,&edge_guards[guardindex]);
        edge_guards[guardindex] &= bigC;
        int outindex = out_edges[index].size();
        out_edges[index].resize(outindex + 1);
        out_edges[index][outindex] = std::make_tuple(target, guardindex);

        int inindex = in_edges[target].size();
        in_edges[target].resize(inindex+1);
        in_edges[target][inindex] = std::make_tuple(index, guardindex);
//        cout<< "with edge to " << target << " allowing: ";
//        dumpSet(&edge_guards[guardindex], fullset, new char[bm_n_vars+1], 0);
        line += i-1;
    }
    declared[index] = true;
}

int Game::readUntil(const char * line, char delim){
    int i = 0;
    while(*(line + i) != delim)
        i++;
    return i;
}

void Game::printCV(unordered_set<int> *bigV, vector<Subset> *vc, Subset t, char * p, int var) {
    if(t == emptyset) return;
    if(var == bm_n_vars)
    {
        p[var]  = '\0';
        cout << "For product " << p << " the following vertices are in: ";
//        for(const int& vi : *bigV)
//        {
            int vi =0;
            Subset result = (*vc)[vi];
            result &= t;
            if(!((result) == emptyset)){
                cout << vi << ',';
            }
//        }
        cout <<"\n";
        fflush(stdout);

    } else {
        Subset t1 = t;
        t1 &= bm_vars[var];
        p[var] = '1';
        printCV(bigV,vc, t1, p , var + 1);
        p[var] = '0';
        Subset t2 = t;
        t2 -= bm_vars[var];
        printCV(bigV, vc, t2, p, var + 1);
    }
}


void Game::printCV(unordered_set<int> *bigV, vector<Subset> *vc) {
    printCV(bigV, vc, bigC, new char[bm_n_vars+1], 0);
}