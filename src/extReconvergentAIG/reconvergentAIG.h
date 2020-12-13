#ifndef ABC__ext__reconvergentAIG_h
#define ABC__ext__reconvergentAIG_h

#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "aig/aig/aig.h" // Aig_Man_t*
#include <stdlib.h>
#include <string.h>
#include <time.h>

ABC_NAMESPACE_HEADER_START

class ReconvergentAIGMgr
{
public:
    ReconvergentAIGMgr(Abc_Frame_t_* pAbc): _pAbc(pAbc), _pNtk(NULL), _pNtkNew(NULL) {}
    ~ReconvergentAIGMgr() {}
    Abc_Ntk_t* generateAND_1(int nNodesMax, int nPi);
    Abc_Ntk_t* generateAND_1_aug(int nNodesMax, int nPi);  
    Abc_Ntk_t* generateAND_2(int nNodesMax, int nPi); 
    Abc_Ntk_t* generateAND_3(int nNodesMax, int nPi); 
    void minimize(int nIter);
    void eval();

    int getNewAND() { return _pNtkNewAnd; }
    int getNewLev() { return _pNtkNewLev; }

    // informal functions for experiment
    void printLog(char* fileName);
    int minimizeCurve(int nIterMax, char* fileName);

private:
    bool executeCommand(char* s);

    Abc_Frame_t_* _pAbc;
    Abc_Ntk_t *_pNtk, *_pNtkNew;
    int _nNodesMax, _nPi;
    int _pNtkAnd, _pNtkLev, _pNtkNewAnd, _pNtkNewLev;
    int* _vPiFanout;
    char _command[1000];
    bool _flag;
    char _minimizeCommand[1000] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";
};


ABC_NAMESPACE_HEADER_END
#endif