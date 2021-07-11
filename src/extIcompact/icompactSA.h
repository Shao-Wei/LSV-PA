#ifndef ABC__ext__ICOMPACTSA_H
#define ABC__ext__ICOMPACTSA_H
#include <iostream>
#include <vector>
#include "memory.h"

using namespace std;
/*
This part of code implements Decision Tree building 
optimized by simulated annealing.
It is used by command "ntkSAconstruct".
*/

class Tree;
class Node;

class IcompactSAMgr
{
public:
    IcompactSAMgr(char * fileName);
    ~IcompactSAMgr() {}

    void trainInit(int fVerbose);
    void SA();
    void writeBlif(char * fileName);
private:
    // basic info from pla
    char* _fileName;
    int _nPi, _nPat;
    char **_piNames, *_poName;
    bool **_fullMatrix, * _fullLabel;

    Tree * _bestTree;

    void trainRecursive(Tree * tree, int nodeId, int criterion);
    float calculateGINI(Node* n, int split);
    void extractDTree(Node * n, FILE * fOut);
};

class Tree
{
public:
    Tree(int nFeature): _nFeature(nFeature) {}
    ~Tree() {}

    Node * _root;
    vector<Node*> _pNodes;
    int _nFeature;
};

class Node
{
public:
    Node(Tree * tree, int id, int nPat): _tree(tree), _id(id), _isLeaf(0) 
    { _capture = new bool[nPat]; memset(_capture, 0, nPat); }
    ~Node() { delete [] _capture; }

    Tree * _tree;
    int _id;
    bool * _capture; 
    vector<int> _path; // 1-index
    bool _isLeaf;
    int _label;
    int _split; // 0-index
    Node * _left;
    Node * _right;
};


#endif