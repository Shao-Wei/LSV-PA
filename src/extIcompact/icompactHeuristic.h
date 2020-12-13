#ifndef ABC__ext__icompactHeuristic_h
#define ABC__ext__icompactHeuristic_h

#include <map>
#include <vector>
#include <string.h>

using namespace std;

class Pattern
{
public:
    Pattern(int ni, char* i, int no, char* o) { nPi = ni; nPo = no; patIn = i; patOut = o; preserve_idx = 0; preserve_bit = '0'; }
    ~Pattern() {}

    int getnPi() const { return nPi; }
    int getnPo() { return nPo; }
    char* getpatIn() { return patIn; }
    char* getpatOut() { return patOut; }
    char getpatInIdx(int idx) const { return patIn[idx]; }
    char getpatOutIdx(int idx) { return patOut[idx]; }

    void maskOne(int idx, char bit) { preserve_idx = idx; preserve_bit = patIn[idx]; patIn[idx] = '0'; }
    void undoOne() { patIn[preserve_idx] = preserve_bit; }
    
private:
    int nPi;
    int nPo;
    char* patIn;
    char* patOut;
    int   preserve_idx;
    char  preserve_bit;
};

static bool PatternLessThan(Pattern* a, Pattern* b);

class ICompactHeuristicMgr
{
public:
    ICompactHeuristicMgr() {}
    ~ICompactHeuristicMgr() {}

    void readFile(char *fileName);
    void pushbackvecPat(int ni, char* patIn, int no, char* patOut) { Pattern* p = new Pattern(ni, patIn, no, patOut); vecPat.push_back(p); }
    int getnPi() { return nPi; }
    int getnPo() { return nPo; }
    bool getLocked(int idx) { return locked[idx]; }

    void lockEntry(double ratio);
    void maskOne(int test);
    void undoOne();
    bool findConflict(bool* litPo);

private:
    int nPi;
    int nPo;
    vector<bool> locked;
    vector<Pattern*> vecPat;    
};

#endif