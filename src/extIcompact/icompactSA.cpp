#include "icompactSA.h"
#include <string.h>
#include <cmath>

IcompactSAMgr::IcompactSAMgr(char * fileName)
{
    // parse file
    char buff[10000];
    char* t, * tTmp;
    char* unused __attribute__((unused));
    FILE* fPla = fopen(fileName, "r");
    _fileName = fileName;

    // get essentials from header
    unused = fgets(buff, 10000, fPla); // .i n
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    _nPi = atoi(t);
    unused = fgets(buff, 10000, fPla); // .o n
    unused = fgets(buff, 10000, fPla); // .ilb
    _piNames = new char*[_nPi];
    t = strtok(buff, " \n");
    for(int i=0; i<_nPi; i++)
    {
        t = strtok(NULL, " \n");
        tTmp = new char[strlen(t)+1];
        strcpy(tTmp, t);
        _piNames[i] = tTmp;
    }
    unused = fgets(buff, 10000, fPla); // .ob
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    tTmp = new char[strlen(t)+1];
    strcpy(tTmp, t);
    _poName = tTmp;
    unused = fgets(buff, 10000, fPla); // .type fr
    unused = fgets(buff, 10000, fPla); // .p n
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    _nPat = atoi(t);

    // construct matrix
    _fullMatrix = new bool*[_nPat];
    for(int i=0; i<_nPat; i++)
        _fullMatrix[i] = new bool[_nPi]();
    _fullLabel = new bool[_nPat]();

    for(int p=0; p<_nPat; p++)
    {
        unused = fgets(buff, 10000, fPla);
        t = strtok(buff, " \n");
        for(int i=0; i<_nPi; i++)
            _fullMatrix[p][i] = (t[i]=='1');
        t = strtok(NULL, " \n");
        _fullLabel[p] = (t[0]=='1');
    }
    fclose(fPla);
}

void IcompactSAMgr::trainInit(int fVerbose)
{
    Tree * res = new Tree(_nPi);
    _bestTree = res;
    trainRecursive(res, 0, 0);
    if(fVerbose)
    {
        printf("Model Stats - \n");
        printf("  node count = %li\n", _bestTree->_pNodes.size());
    }
}

void IcompactSAMgr::trainRecursive(Tree * tree, int nodeId, int criterion)
{
    if(nodeId == 0)
    {
        Node * r = new Node(tree, 0, _nPat);
        tree->_pNodes.push_back(r);
        tree->_root = r;
        memset(r->_capture, 1, _nPat);
        // set _isLeaf, _label
        int label0 = 0, label1 = 0;
        for(int i=0; i<_nPat; i++)
        {
            if(!_fullLabel[i]) label0++;
            else label1++;
        }
        if(label0 == 0) { r->_isLeaf = 1; r->_label = 1; }
        else if(label1 == 0) { r->_isLeaf = 1; r->_label = 0; }
        else { r->_isLeaf = 0; }
    }

    Node * nodeToSplit = tree->_pNodes[nodeId];
    if(nodeToSplit->_isLeaf)
        return;
    
    // find split
    bool pFeatureIsUsed[_nPi];
    memset(pFeatureIsUsed, 0, _nPi);
    for(int i=0; i<nodeToSplit->_path.size(); i++)
        pFeatureIsUsed[abs(nodeToSplit->_path[i])-1] = 1;

    float bestQuality = 0; // GINI: 0.5 - 1
    int bestSplit = 0;
    for(int i=0; i<_nPi; i++)
    {
        if(!pFeatureIsUsed[i])
        {
            float quality = calculateGINI(nodeToSplit, i);
            if(quality > bestQuality) { bestQuality = quality; bestSplit = i; }
        }
    }
    nodeToSplit->_split = bestSplit;
    
    // derive children
    int leftId = tree->_pNodes.size();
    Node * nL = new Node(tree, leftId, _nPat);
    tree->_pNodes.push_back(nL);
    nL->_path = nodeToSplit->_path;
    nL->_path.push_back(-1*(bestSplit+1));
    nodeToSplit->_left = nL;
    int rightId = leftId + 1;
    Node * nR = new Node(tree, rightId, _nPat);
    tree->_pNodes.push_back(nR);
    nR->_path = nodeToSplit->_path;
    nR->_path.push_back(bestSplit+1);
    nodeToSplit->_right = nR;

    int lLabel0 = 0, lLabel1 = 0, rLabel0 = 0, rLabel1 = 0;
    for(int i=0; i<_nPat; i++)
        if(nodeToSplit->_capture[i])
        {
            if(!_fullMatrix[i][bestSplit])
            {
                nL->_capture[i] = 1;
                if(!_fullLabel[i]) lLabel0++;
                else lLabel1++;
            }
            else
            {
                nR->_capture[i] = 1;
                if(!_fullLabel[i]) rLabel0++;
                else rLabel1++;
            }
        }
    if(lLabel0 == 0)      { nL->_isLeaf = 1; nL->_label = 1; nL->_left = NULL; nL->_right = NULL; }
    else if(lLabel1 == 0) { nL->_isLeaf = 1; nL->_label = 0; nL->_left = NULL; nL->_right = NULL; }
    else                  { nL->_isLeaf = 0; trainRecursive(tree, leftId, 0); }
    if(rLabel0 == 0)      { nR->_isLeaf = 1; nR->_label = 1; nR->_left = NULL; nR->_right = NULL; }
    else if(rLabel1 == 0) { nR->_isLeaf = 1; nR->_label = 0; nR->_left = NULL; nR->_right = NULL; }
    else                  { nR->_isLeaf = 0; trainRecursive(tree, rightId, 0); }
}

float IcompactSAMgr::calculateGINI(Node* n, int split)
{
    float lLabel0 = 0, lLabel1 = 0, rLabel0 = 0, rLabel1 = 0;
    float lCount = 0, rCount = 0;
    for(int i=0; i<_nPat; i++)
        if(n->_capture[i])
        {
            if(!_fullMatrix[i][split])
            {
                lCount+=1.0;
                if(!_fullLabel[i]) lLabel0+=1.0;
                else lLabel1+=1.0;
            }
            else
            {
                rCount+=1.0;
                if(!_fullLabel[i]) rLabel0+=1.0;
                else rLabel1+=1.0;
            }
        }
    return  (pow(lLabel0/lCount, 2) + pow(lLabel1/lCount, 2)) * lCount/(lCount+rCount) + \
            (pow(rLabel0/rCount, 2) + pow(rLabel1/rCount, 2)) * rCount/(lCount+rCount);
}

void IcompactSAMgr::writeBlif(char * fileName)
{
    FILE * fOut = fopen(fileName, "w");
    // header
    fprintf(fOut, ".model %s\n", _fileName);
    fprintf(fOut, ".inputs");
    for(int i=0; i<_nPi; i++)
        fprintf(fOut, " %s", _piNames[i]);
    fprintf(fOut, "\n.outputs %s\n\n", _poName);

    extractDTree(_bestTree->_root, fOut);
    fprintf(fOut, ".names n_0 %s\n1 1\n", _poName);
    fclose(fOut);
}

void IcompactSAMgr::extractDTree(Node * n, FILE * fOut)
{
    if(n == NULL)
        return;
    if(n->_isLeaf)
    {
        fprintf(fOut, ".names n_%i\n", n->_id);
        if(n->_label == 1)
            fprintf(fOut, "1\n");
    }
    else // internal node
    {
        extractDTree(n->_left, fOut);
        extractDTree(n->_right, fOut);
        fprintf(fOut, ".names n_%i n_%i %s n_%i\n", \
            n->_left->_id, n->_right->_id, _piNames[n->_split], n->_id);
        fprintf(fOut, "-11 1\n1-0 1\n");
    }
}
