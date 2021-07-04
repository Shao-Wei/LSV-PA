#ifndef ABC__ext__icompactOSDT_h
#define ABC__ext__icompactOSDT_h

#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>
#include <map>
using namespace std;

/*
This part of code implements Optimal Sparse Decision
Tree inspired by Hu, Xiyang, Cynthia Rudin, and Margo 
Seltzer. "Optimal sparse decision trees." Advances in 
Neural Information Processing Systems (NeurIPS) (2019).

It is used by command "ntkOSDTconstruct".
*/

/*
TODO:
    IcompactOSDTMgr extract tree null leaf??
*/
class SparseDLeaf;
class SparseDTree;
class DTree;
class DNode;

class IcompactOSDTMgr
{
public:
    IcompactOSDTMgr(char* fileName);
    ~IcompactOSDTMgr();

    void train(int fVerbose);
    void bestTree2Blif(char * fileName);

private:
    // basic info from pla
    char* _fileName;
    int _nPi, _nPat;
    char **_piNames, *_poName;
    bool **_fullMatrix, * _fullLabel;

    // tree structure
    SparseDTree * _bestTree;
    
    map< vector<int>, SparseDLeaf*> _leafCache;
    map< vector<SparseDLeaf*>, SparseDTree*> _treeCache;

    int _leafUnique, _leafLookup;
    int _treeUnique, _treeLookup;

    SparseDLeaf * getNewLeaf(SparseDLeaf * parent, int newRule);
    vector< vector<int> > getRulesForSplits(SparseDTree * parent); // generate all combinations for split
    void extractDTree(DNode * n, FILE * fOut);
    void printStatistics();
};

class SparseDTree
{
public:
    SparseDTree(vector<SparseDLeaf*> pLeaf, int cost, int level)
    {
        sort(pLeaf.begin(), pLeaf.end());
        _pLeaf = pLeaf;
        _cost = cost;
        _level = level;
    }
    ~SparseDTree() {}

    vector<SparseDLeaf*> getLeafToBeSplit();
    vector<SparseDLeaf*> getLeafUnchanged();
    bool noFurtherSplit();

    int getCost() const { return _cost; }
    int getLevel() const { return _level; }

private:
    vector<SparseDLeaf*> _pLeaf; // unchanged or tobesplit stored in SparseDLeaf
    int _cost;
    int _level;
};

struct LevelGreaterThan // decending
{
    bool operator()(SparseDTree* a, SparseDTree* b)
    {
        return a->getLevel() > b->getLevel(); 
    }
};

class SparseDLeaf
{
public:
    SparseDLeaf(SparseDLeaf * parent, int newRule, bool ** fullMatrix, bool * fullLabel, int nPi, int nPat);
    SparseDLeaf(bool ** fullMatrix, bool * fullLabel, int nPi, int nPat);
    ~SparseDLeaf() { delete [] _pCapture; }

    vector<int> getRules() { return _pRule; }
    int getRule(int idx) { return (idx >= _pRule.size() || idx < 0)? 0: _pRule[idx]; }
    bool getFSplit() { return _fSplit; }
    int getLabel() { return _label; }
    vector<int> getValidSplitRules() { return _pValidSplitRules; }
private:    
    vector<int> _pRule; // rule index start from 1, - indicates negation
    bool * _pCapture;
    int _nCapture;
    bool _fSplit; // need further split
    int _label;
    vector<int> _pValidSplitRules;
    
};

class DTree
{
public:
    DTree() {}
    ~DTree() {}
    
    vector<DNode*> _pNode;
};

class DNode
{
public:
    DNode() {}
    ~DNode() {}

    int _id;
    int _rule;
    int _label; // -1 empty leaf, 0/1 sparse leaf label
    DNode * _nParent;
    DNode * _nLeft;
    DNode * _nRight;
};

#endif