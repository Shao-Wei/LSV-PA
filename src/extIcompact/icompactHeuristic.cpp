#include "icompactHeuristic.h"
#include "icompact.h"
#include <iostream>
#include <algorithm> // sort
#include <map>
#include <vector>
#include <string.h> 

static bool PatternLessThan(Pattern* a, Pattern* b)
{
    for(int i=0; i<a->getnPi(); i++)
    {
        if(a->getpatInIdx(i) > b->getpatInIdx(i))
            return 1;
        if(a->getpatInIdx(i) < b->getpatInIdx(i))
            return 0;
    }
    return 0;
}

void ICompactHeuristicMgr::readFile(char *fileName)
{
    FILE * fRead = fopen(fileName, "r");
    char buff[10000];
    char *t;
    char * unused __attribute__((unused));

    unused = fgets(buff, 10000, fRead); // .i n
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    nPi = atoi(t);
    unused = fgets(buff, 10000, fRead); // .o m
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    nPo = atoi(t);
    unused = fgets(buff, 10000, fRead); // .type fr
    while(fgets(buff, 10000, fRead))
    {
        char* one_line_i = new char[nPi+1];
        one_line_i[nPi] = '\0';
        t = strtok(buff, " \n");
        for(size_t k=0; k<nPi; k++)
            one_line_i[k] = t[k];

        char* one_line_o = new char[nPo+1];
        one_line_o[nPo] = '\0';
        t = strtok(NULL, " \n");
        for(size_t k=0; k<nPo; k++)
            one_line_o[k] = t[k];
        pushbackvecPat(nPi, one_line_i, nPo, one_line_o);
    }

    fclose(fRead);
}

void ICompactHeuristicMgr::lockEntry(double ratio)
{
    Pattern* p;
    int patSize = vecPat.size();
    double upper = (1-ratio)*patSize;
    double lower = ratio*patSize;
    int count[nPi][nPo] = {};

    for(int i=0; i<patSize; i++)
    {
        p = vecPat[i];
        for(int j=0; j<nPi; j++)
            for(int k=0; k<nPo; k++)
                if(p->getpatInIdx(j) == p->getpatOutIdx(k) || p->getpatOutIdx(k) == '-')
                    count[j][k]++;
    }

    locked.clear();
    for(int i=0; i<nPi; i++)
        locked.push_back(0);
    for(int i=0; i<nPi; i++)
        for(int j=0; j<nPo; j++)
            if(count[i][j] > upper || count[i][j] < lower)
            {
                locked[i] = 1;
                break;
            }
    // printf("LOCKED\n");
    // for(int i=0; i<nPi; i++)
    //     printf("%s", locked[i]?"1":"0");
    // printf("\n");
}

void ICompactHeuristicMgr::maskOne(int test)
{
    for(int i=0, n=vecPat.size(); i<n; i++)
        vecPat[i]->maskOne(test, '0');      
}

void ICompactHeuristicMgr::undoOne()
{
    for(int i=0, n=vecPat.size(); i<n; i++)
        vecPat[i]->undoOne();
}

bool ICompactHeuristicMgr::findConflict(bool* litPo)
{    
    sort(vecPat.begin(), vecPat.end(), PatternLessThan);
    // for(int i=0; i<vecPat.size(); i++)
    //     printf("%s\n", vecPat[i]->getpatIn());

    char* curr_i, *curr_o, *prev_i, *prev_o;
    prev_i = vecPat[0]->getpatIn();
    prev_o = vecPat[0]->getpatOut();
    for(int i=1, n=vecPat.size(); i<n; i++)
    {
        curr_i = vecPat[i]->getpatIn();
        curr_o = vecPat[i]->getpatOut();
        if(strcmp(curr_i, prev_i) == 0)
        {
            // check output
            for(int j=0; j<nPo; j++)
                if(litPo[j] && (curr_o[j] != prev_o[j]))
                    return 1;
        }
        else
        {
            prev_i = curr_i;
            prev_o = prev_o;
        }  
    }
    return 0;
}

