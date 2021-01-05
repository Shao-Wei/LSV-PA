#ifndef ABC__ext__icompactHeuristic_h
#define ABC__ext__icompactHeuristic_h

#include <map>
#include <vector>
#include <string.h>

using namespace std;

class Pattern
{
public:
    Pattern(int ni, char* i, int no, char* o) { _nPi = ni; _nPo = no; _patIn = i; _patOut = o; _preserve_idx = 0; _preserve_bit = '0'; }
    ~Pattern() { delete [] _patIn; delete [] _patOut; }

    int   _nPi;
    int   _nPo;
    char* _patIn;
    char* _patOut;

    // only class ICompactHeuristicMgr uses mask & undo
    friend class ICompactHeuristicMgr;
private:
    int   _preserve_idx;
    char  _preserve_bit;

    // modify pattern
    void maskOne(int idx) { _preserve_idx = idx; _preserve_bit = _patIn[idx]; _patIn[idx] = '0'; }
    void undoOne() { _patIn[_preserve_idx] = _preserve_bit; }
    
};

static bool PatternLessThan(Pattern* a, Pattern* b);

class ICompactHeuristicMgr
{
public:
    ICompactHeuristicMgr(char *fileName);
    ~ICompactHeuristicMgr();

    void lockEntry(double ratio);
    void supportInfo(int **info) { _supportInfo = info; } // to restrict compact 
    void rmSupportInfo() { _supportInfo = NULL; }

    bool ** compact_drop_each(int iterNum);
    bool * compact_drop(int iterNum, int fAllPo);
    bool * compact_add(int iterNum); // later. need diff ds

    int getnPi() { return _nPi; }
    int getnPo() { return _nPo; }
    bool * getMinMask() { return _minMask; }
    bool ** getEachMinMask() { return _eachMinMask; }
    
private:
    int _nPi;
    int _nPo;
    vector<Pattern*> _vecPat;

    bool *_locked; // record locked pi
    bool *_litPo; // record cared po 
    int ** _supportInfo;
    vector<int> _varOrder;
    bool *_minMask; // len _nPi
    bool **_eachMinMask; // len _nPo * _nPi

    // aux
    void pushbackvecPat(int ni, char* patIn, int no, char* patOut) { Pattern* p = new Pattern(ni, patIn, no, patOut); _vecPat.push_back(p); }
    void varOrder_randomSuffle();
    bool isLocked(int idx) { return _locked[idx]; }
    void maskOne(int test);
    void undoOne();
    bool findConflict(bool* litPo, int fVerbose);
};

#endif