#include "icompactHeuristic.h"
#include "icompact.h"
#include <iostream>
#include <algorithm> // sort
#include <map>
#include <vector>
#include <string.h> 

Pattern::Pattern(int ni, char* ipat, int no, char* opat)
{
    _nPi = ni;
    _nPo = no;

    _patIn = new char[_nPi+1];
    _patIn[_nPi] = '\0';
    _oriPatIn = new char[_nPi+1];
    _oriPatIn[_nPi] = '\0';
    for(int i=0; i<_nPi; i++) { _patIn[i] = ipat[i]; _oriPatIn[i] = ipat[i]; }
    _patOut = new char[_nPo+1];
    _patOut[_nPo] = '\0';
    for(int i=0; i<_nPo; i++) { _patOut[i] = opat[i]; }

    _preserve_idx = 0;
    _preserve_bit = '0';    
}

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

ICompactHeuristicMgr::ICompactHeuristicMgr(char *fileName, int fRemove)
{
    FILE *fRead = fopen(fileName, "r");
    char buff[10000];
    char *t, *t1, *t2;
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
    unused = fgets(buff, 10000, fRead); // .p
    while(fgets(buff, 10000, fRead))
    {
        t1 = strtok(buff, " \n");
        t2 = strtok(NULL, " \n");
        pushbackvecPat(_nPi, t1, _nPo, t2);
    }

    _fNotMasked = 1;
    _locked = new bool[_nPi](); // init to zero
    _litPo = new bool[_nPo](); // init to zero
    _currMask = new bool[_nPi];
    for(int i=0; i<_nPi; i++)
        _currMask[i] = 1;
    _supportInfo = NULL;
    _minMask = new bool[_nPi];
    _eachMinMask = new bool*[_nPo];
    for(int i=0; i<_nPo; i++)
        _eachMinMask[i] = new bool[_nPi];    

    _oriPatCount = _vecPat.size();
    _uniquePatCount = uniquePat(fRemove);
    
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

int ICompactHeuristicMgr::uniquePat(int fRemove)
{
    if(!_fNotMasked) { reset(); }
    
    int newCount = 1;

    sort(_vecPat.begin(), _vecPat.end(), PatternLessThan);

    vector<Pattern*>::iterator it = _vecPat.begin();
    Pattern *prev = *it, *curr;
    it++;
    while(it != _vecPat.end())
    {
        curr = *it;
        if(strcmp(prev->_patIn, curr->_patIn) == 0)
        {
            if(fRemove)
                it = _vecPat.erase(it);
            else
                it++;
        }
        else
        {
            newCount++;
            prev = *it;
            it++;
        }
    }
    return newCount;
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
        reset();
        minMask = compact_drop(iterNum, 0);
        if(minMask == NULL) { return NULL; }
        // update _eachMinMask
        for(int k=0; k<_nPi; k++)
            _eachMinMask[i][k] = minMask[k];
    }
    reset();

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
bool * ICompactHeuristicMgr::compact_add(int iterNum)
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
    _fNotMasked = 0;
    _currMask[test] = 0;
    _preservedIdx = test;
    for(int i=0, n=_vecPat.size(); i<n; i++)
        _vecPat[i]->maskOne(test);
}

void ICompactHeuristicMgr::undoOne()
{
    _currMask[_preservedIdx] = 1;
    for(int i=0, n=_vecPat.size(); i<n; i++)
        _vecPat[i]->undoOne();
}

void ICompactHeuristicMgr::reset()
{
    _fNotMasked = 1;
    for(int i=0, n=_vecPat.size(); i<n; i++)
        _vecPat[i]->reset();      
}

bool ICompactHeuristicMgr::findConflict(bool* litPo, int fVerbose)
{    
    sort(_vecPat.begin(), _vecPat.end(), PatternLessThan);
    // for(int i=0; i<vecPat.size(); i++)
    //     printf("%s\n", vecPat[i]->getpatIn());
    Pattern *prevPat, *currPat;
    char* curr_i, *curr_o, *prev_i, *prev_o;
    prevPat = _vecPat[0];
    prev_i = _vecPat[0]->_patIn;
    prev_o = _vecPat[0]->_patOut;
    for(int i=1, n=_vecPat.size(); i<n; i++)
    {
        currPat = _vecPat[i];
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
                        printf("log findConflict()\n");
                        printf("  current mask / litPo:\n  ");
                        for(int k=0; k<_nPi; k++)
                            printf("%i", (_currMask[k])? 1:0);
                        printf(" ");
                        for(int k=0; k<_nPo; k++)
                            printf("%i", (_litPo[k])? 1:0);
                        printf("\n  Conflict:\n");
                        printf("  %s %s\n", prevPat->_oriPatIn, prev_o);
                        printf("  %s\n", prev_i);
                        printf("  %s %s\n", currPat->_oriPatIn, curr_o);
                        printf("  %s\n", curr_i);
                    }
                    return 1;
                }    
        }
        else
        {
            prevPat = currPat;
            prev_i = curr_i;
            prev_o = curr_o;
        }  
    }
    return 0;
}

