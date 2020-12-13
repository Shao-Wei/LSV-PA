#ifndef ABC__ext__reencodeHeuristic_h
#define ABC__ext__reencodeHeuristic_h

#include <vector>
#include <string.h>
#include <map>

using namespace std;

class OCNode
{
public:
    OCNode(int i) { id = i; parent = NULL; pivot = -1; left = NULL; right = NULL; containedPat.clear(); encode = NULL; }
    ~OCNode() {}

    void setParent(OCNode* n) { parent = n; }
    void setLeft(OCNode* n) { left = n; }
    void setRight(OCNode* n) { right = n; }
    void setPivot(int i) { pivot = i; }
    void setEncode(char* s) { encode = s; }
    void pushbackcontainedPat(int i) { containedPat.push_back(i); }
    int getSize() { return containedPat.size(); }
    int getPat(int i) { return containedPat[i]; }
    int getId() { return id; }
    OCNode* getParent() { return parent; }
    OCNode* getLeft() { return left; }
    OCNode* getRight() {return right; }
    int getPivot() { return pivot; }
    char* getEncode() { return encode; }
private:
    int id;
    OCNode* parent;
    int pivot;
    OCNode* left;
    OCNode* right;
    vector<int> containedPat;
    char* encode;
};

class OCTree
{
public:
    OCTree() {}
    ~OCTree() {}

    OCNode* getNode(int i) { return nodeList[i]; }
    int getSize() { return nodeList.size(); }

    OCNode* addNode() { OCNode* n = new OCNode(nodeList.size()); nodeList.push_back(n); return n;}
    int print(OCNode* n, int level);
private:
    // nodeList[0] as root
    vector<OCNode*> nodeList;
};

class PartialPattern
{
public:
    PartialPattern(int n, char* c, PartialPattern* p): length(n), patOut(c), uniquePat(p) { }
    ~PartialPattern() { delete patOut; }

    void setEncode(char* s) { encode = s; }
    char getPatIdx(int i) { return patOut[i]; }
    char* getPat() { return patOut; }
    PartialPattern* getUniquePat() { return uniquePat; }
    char* getEncode() { return encode; }
    
private:
    int length;
    char* patOut;
    // points to unique pattern used for tree construction,
    // points to NULL if THIS is unique pattern used for tree construction
    PartialPattern* uniquePat;
    char* encode;
};

class ReencodeHeuristicMgr
{
public:
    ReencodeHeuristicMgr() { length = 0; globalFlag = 0; pTree = NULL; curpTree = NULL; }
    ~ReencodeHeuristicMgr() {}

    void readFile(char* fileName, bool type); // encode PO if type 0; PI otherwise
    int getEncoding(int numNewVar);
    void getMapping(bool type, int* map);
    char* getOCResult(int idx) { return vecPat[idx]->getUniquePat()->getEncode(); }

private:
    int length;
    int result;
    vector<PartialPattern*> vecPat;
    vector<PartialPattern*> uniquePat;
    int globalFlag;
    OCTree* pTree;
    OCTree* curpTree; // managing tree structure
    map<int, int> varMap; // constructed while getEncoding; input idx -> encode idx

    PartialPattern* pushbackvecPat(int n, char* c, PartialPattern* p) { PartialPattern* pat = new PartialPattern(n, c, p);    vecPat.push_back(pat);    return pat; }
    PartialPattern* pushbackuniquePat(int n, char* c)                 { PartialPattern* pat = new PartialPattern(n, c, NULL); uniquePat.push_back(pat); return pat; }
    void encodeUniquePat(OCNode* n, int len);
};


#endif