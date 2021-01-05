#include "icompactHeuristic.h"
#include "icompact.h"
#include <iostream>
#include <algorithm> // sort
#include <map>
#include <vector>
#include <string.h> 

static bool PatternLessThan(Pattern* a, Pattern* b)
{
    for(int i=0; i<a->_nPi; i++)
    {
        if(a->_patIn[i] > b->_patIn[i])
            return 1;
        if(a->_patIn[i] < b->_patIn[i])
            return 0;
    }
    return 0;
}

ICompactHeuristicMgr::ICompactHeuristicMgr(char *fileName)
{
    FILE *fRead = fopen(fileName, "r");
    char buff[10000];
    char *t;
    char *unused __attribute__((unused));

    unused = fgets(buff, 10000, fRead); // .i n
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    _nPi = atoi(t);
    unused = fgets(buff, 10000, fRead); // .o m
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    _nPo = atoi(t);
    unused = fgets(buff, 10000, fRead); // .type fr
    while(fgets(buff, 10000, fRead))
    {
        char *one_line_i = new char[_nPi+1]; // delete by class Pattern destructor
        one_line_i[_nPi] = '\0';
        t = strtok(buff, " \n");
        for(size_t k=0; k<_nPi; k++)
            one_line_i[k] = t[k];

        char *one_line_o = new char[_nPo+1]; // delete by class Pattern destructor
        one_line_o[_nPo] = '\0';
        t = strtok(NULL, " \n");
        for(size_t k=0; k<_nPo; k++)
            one_line_o[k] = t[k];
        pushbackvecPat(_nPi, one_line_i, _nPo, one_line_o);
    }

    _locked = new bool[_nPi](); // init to zero
    _litPo = new bool[_nPo];
    for(int i=0; i<_nPo; i++) 
        _litPo[i] = 1;
    _minMask = new bool[_nPi];
    for(size_t i=0; i<_nPi; i++)
        _varOrder.push_back(i);

    // clean up
    fclose(fRead);
}

ICompactHeuristicMgr::~ICompactHeuristicMgr()
{
        delete [] _locked;
        delete [] _litPo;
        delete [] _minMask;
}

void ICompactHeuristicMgr::lockEntry(double ratio)
{
    if(ratio < 0.01) { return; }
    Pattern* p;
    int patSize = _vecPat.size();
    double upper = (1-ratio)*patSize;
    double lower = ratio*patSize;
    int count[_nPi][_nPo] = {};

    for(int i=0; i<patSize; i++)
    {
        p = _vecPat[i];
        for(int j=0; j<_nPi; j++)
            for(int k=0; k<_nPo; k++)
                if(p->_patIn[j] == p->_patOut[k] || p->_patOut[k] == '-')
                    count[j][k]++;
    }

    for(int i=0; i<_nPi; i++)
        for(int j=0; j<_nPo; j++)
            if(count[i][j] > upper || count[i][j] < lower)
            {
                _locked[i] = 1;
                break;
            }
}

bool * ICompactHeuristicMgr::compact_drop(int iterNum)
{
    size_t maskCount, minMaskCount = _nPi;
    char mask[_nPi], minMask[_nPi];

    // solve
    for(int iter=0; iter<iterNum; iter++)
    {
        printf("Solve heuristic - iter %i\n", iter);
        varOrder_randomSuffle();

        for(size_t i=0; i<_nPi; i++)
            mask[i] = 1;
        for(size_t i=0; i<_nPi; i++)
        {
            size_t test = _varOrder[i];
            if(isLocked(test)) { continue; }
            mask[test] = 0; // drop var
            maskOne(test);
            if(findConflict(_litPo))
            {
                mask[test] = 1; // preserve var
                undoOne();
            }        
        }
        maskCount = _nPi;
        for(size_t i=0; i<_nPi; i++)
            if(mask[i] == 0)
                maskCount--; 
        // update
        if(maskCount <= minMaskCount)
        {
            minMaskCount = maskCount;
            for(int i=0; i<_nPi; i++)
                minMask[i] = mask[i];
        }
    }

    for(size_t i=0; i<_nPi; i++)
        _minMask[i] = minMask[i];

    return _minMask;
}

void ICompactHeuristicMgr::varOrder_randomSuffle()
{
    random_shuffle(_varOrder.begin(), _varOrder.end());
    random_shuffle(_varOrder.begin(), _varOrder.end()); 
}

void ICompactHeuristicMgr::maskOne(int test)
{
    for(int i=0, n=_vecPat.size(); i<n; i++)
        _vecPat[i]->maskOne(test);      
}

void ICompactHeuristicMgr::undoOne()
{
    for(int i=0, n=_vecPat.size(); i<n; i++)
        _vecPat[i]->undoOne();
}

bool ICompactHeuristicMgr::findConflict(bool* litPo)
{    
    sort(_vecPat.begin(), _vecPat.end(), PatternLessThan);
    // for(int i=0; i<vecPat.size(); i++)
    //     printf("%s\n", vecPat[i]->getpatIn());

    char* curr_i, *curr_o, *prev_i, *prev_o;
    prev_i = _vecPat[0]->_patIn;
    prev_o = _vecPat[0]->_patOut;
    for(int i=1, n=_vecPat.size(); i<n; i++)
    {
        curr_i = _vecPat[i]->_patIn;
        curr_o = _vecPat[i]->_patOut;
        if(strcmp(curr_i, prev_i) == 0)
        {
            // check output
            for(int j=0; j<_nPo; j++)
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

