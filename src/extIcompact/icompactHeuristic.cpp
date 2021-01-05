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
    unused = fgets(buff, 10000, fRead); // .ilb
    unused = fgets(buff, 10000, fRead); // .ob
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
    _litPo = new bool[_nPo](); // init to zero
    _supportInfo = NULL;
    _minMask = new bool[_nPi];
    _eachMinMask = new bool*[_nPo];
    for(int i=0; i<_nPo; i++)
        _eachMinMask[i] = new bool[_nPi];    

    // clean up
    fclose(fRead);
}

ICompactHeuristicMgr::~ICompactHeuristicMgr()
{
        delete [] _locked;
        delete [] _litPo;
        delete [] _minMask;
        for(int i=0; i<_nPo; i++)
            delete [] _eachMinMask[i];
        delete [] _eachMinMask;
}

// if certain pi is consistent w/ certain po, it is locked 
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

// compact each po independently - scheme drop
bool ** ICompactHeuristicMgr::compact_drop_each(int iterNum)
{
    bool * minMask;
    for(int i=0; i<_nPo; i++)
    {
        // update _litPo
        for(int k=0; k<_nPo; k++)
            _litPo[k] = 0;
        _litPo[i] = 1;
        
        // start solving
        minMask = compact_drop(iterNum, 0);

        // update _eachMinMask
        for(int k=0; k<_nPi; k++)
            _eachMinMask[i][k] = minMask[k];
    }

    return _eachMinMask;
}

// compact - scheme drop. fAllPo = 1 considers all po at once. otherwise customized litPo setting is needed.
bool * ICompactHeuristicMgr::compact_drop(int iterNum, int fAllPo)
{
    size_t maskCount, minMaskCount = _nPi;
    int mask[_nPi], minMask[_nPi];
    int initMask[_nPi];
    if(fAllPo) // otherwise, current _litPo setting is used
    {
        for(int i=0; i<_nPo; i++)
            _litPo[i] = 1;
    }

    // set init mask, union of supports of cared po
    if(_supportInfo == NULL)
    {
        for(int i=0; i<_nPi; i++)
            initMask[i] = 1;
    }
    else
    {   
        for(int i=0; i<_nPi; i++)
            initMask[i] = 0;
        for(int i=0; i<_nPo; i++)
        {
            if(_litPo[i])
            {
                for(int k=0; k<_nPi; k++)
                    if(_supportInfo[i][k]) { initMask[k] = 1; }
            }
        }
    }
    for(int i=0; i<_nPi; i++)
        printf("%i", initMask[i]);
    printf("\n");
    for(int i=0; i<_nPi; i++)
    {
        if(!initMask[i])
            maskOne(i);
    }
    _varOrder.clear();
    for(int i=0; i<_nPi; i++)
        if(initMask[i]) { _varOrder.push_back(i); }

    // check validity of support info
    if(findConflict(_litPo, 1))
    {
        printf("Initial mask causing conflict between patterns.\n");
        return NULL;
    }

    // solve
    for(int iter=0; iter<iterNum; iter++)
    {
        // printf("Solve heuristic (drop) - iter %i\n", iter);
        varOrder_randomSuffle();

        for(size_t i=0; i<_nPi; i++)
            mask[i] = initMask[i];
        for(size_t i=0; i<_varOrder.size(); i++)
        {
            size_t test = _varOrder[i];
            if(isLocked(test)) { continue; }
            mask[test] = 0; // drop var
            maskOne(test);
            if(findConflict(_litPo, 0))
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

// compact - scheme add
bool * compact_add(int iterNum)
{
    return NULL;
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

bool ICompactHeuristicMgr::findConflict(bool* litPo, int fVerbose)
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
                {
                    if(fVerbose)
                    {
                        printf("log finConflict()\n");
                        printf("  litPo:\n");
                        for(int k=0; k<_nPo; k++)
                            printf("%i", (_litPo[k])? 1:0);
                        printf("\n  Conflict:\n");
                        printf("  %s %s\n", prev_i, prev_o);
                        printf("  %s %s\n", curr_i, curr_o);
                    }
                    return 1;
                }    
        }
        else
        {
            prev_i = curr_i;
            prev_o = prev_o;
        }  
    }
    return 0;
}

