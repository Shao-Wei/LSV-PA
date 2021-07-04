#include "icompactOSDT.h"
#include <string.h>
#include <assert.h>
/************************
 * Class IcompactOSDTMgr
 ************************/
IcompactOSDTMgr::IcompactOSDTMgr(char* fileName)
{
    // variables set to default
    _bestTree = NULL;
    _leafUnique = _leafLookup = 0;
    _treeUnique = _treeLookup = 0;

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

IcompactOSDTMgr::~IcompactOSDTMgr()
{
    for(int i=0; i<_nPi; i++)
        delete [] _piNames[i];
    delete [] _piNames;
    delete [] _poName;
    for(int i=0; i<_nPat; i++)
        delete [] _fullMatrix[i];
    delete [] _fullMatrix;
    delete [] _fullLabel;

    map< vector<int>, SparseDLeaf*>::iterator leafIter;
    map< vector<SparseDLeaf*>, SparseDTree*>::iterator treeIter;
    for(leafIter = _leafCache.begin(); leafIter != _leafCache.end(); ++leafIter)
        delete leafIter->second;
    for(treeIter = _treeCache.begin(); treeIter != _treeCache.end(); ++treeIter)
        delete treeIter->second;

}

void IcompactOSDTMgr::train(int fVerbose)
{
    vector<int> giniOrder; // sort features by decending gini
    priority_queue<SparseDTree*, vector<SparseDTree*>, LevelGreaterThan> workList;
    SparseDTree * tree, * treeParent;
    SparseDLeaf * leaf, *leaf2, * leafToBeRemoved;
    vector<SparseDLeaf*> pLeaf, pLeafToBeSplit, pLeafUnchanged;
    vector<int> pRule;
    map< vector<SparseDLeaf*>, SparseDTree*>::iterator treeIter;
    int newRule;
    int treeTraverseCount = 0; // printing progress

    // flags for bounds
    bool fNoSplit;
    bool fLookAhead;

    _bestTree = NULL;

    leaf = new SparseDLeaf(_fullMatrix, _fullLabel, _nPi, _nPat); // leaf w/ no rules, not cached since no possible symmetry
    pLeaf.push_back(leaf);
    tree = new SparseDTree(pLeaf, 0, 0); // root tree, not cached since no possible symmetry
    workList.push(tree);

    while(!workList.empty())
    {
        treeParent = workList.top();
        workList.pop();
        treeTraverseCount++;
        if(fVerbose && treeTraverseCount%20==0)
        {
            printf("Progress = %i (trees in work list = %li)\n", treeTraverseCount, workList.size());
            printf("  Tree level = %i (leaves to split = %li)\n", treeParent->getLevel(), treeParent->getLeafToBeSplit().size());
        }
            

        pLeafToBeSplit = treeParent->getLeafToBeSplit();
        pLeafUnchanged = treeParent->getLeafUnchanged();
        vector< vector<int> > ruleCombinations = getRulesForSplits(treeParent);
        for(int i=0, m=ruleCombinations.size(); i<m; i++)
        {
            vector<SparseDLeaf*> pNewLeaves = pLeafUnchanged;
            int nUnchanged = pNewLeaves.size();
            for(int l=0, n=pLeafToBeSplit.size(); l<n; l++)
            {
                leafToBeRemoved = pLeafToBeSplit[l];
                newRule = ruleCombinations[i][l];
                leaf = getNewLeaf(leafToBeRemoved, newRule);
                leaf2 = getNewLeaf(leafToBeRemoved, -1*newRule);
                pNewLeaves.push_back(leaf);
                pNewLeaves.push_back(leaf2);
            }

            sort(pNewLeaves.begin(), pNewLeaves.end());
            treeIter = _treeCache.find(pNewLeaves);
            if(treeIter != _treeCache.end())
                _treeLookup++; // do nothing
            else
            {
                _treeUnique++;
                tree = new SparseDTree(pNewLeaves, treeParent->getCost()+pLeafToBeSplit.size(), treeParent->getLevel()+1);
                _treeCache[pNewLeaves] = tree;
                
                // if 100% train acc
                if(tree->noFurtherSplit())
                {
                    if(_bestTree == NULL)
                    {
                        _bestTree = tree;
                         if(fVerbose)
                            printf("New best tree = %i\n", tree->getCost());
                    }
                    else if(tree->getCost() < _bestTree->getCost())
                    {
                         _bestTree = tree;
                         if(fVerbose)
                            printf("New best tree = %i\n", tree->getCost());
                    } 
                }     

                // conditions to push into work list
                if(_bestTree == NULL)
                    workList.push(tree);
                else
                {
                    fNoSplit = tree->noFurtherSplit();
                    fLookAhead = ((tree->getCost()+pNewLeaves.size()-nUnchanged) < _bestTree->getCost());
                    if(fNoSplit & fLookAhead)
                        workList.push(tree);
                }
            }
        }
    }
    printStatistics();
}

SparseDLeaf * IcompactOSDTMgr::getNewLeaf(SparseDLeaf * parent, int newRule)
{
    vector<int> pRule;
    SparseDLeaf * result;
    map< vector<int>, SparseDLeaf*>::iterator it;
    
    pRule = parent->getRules();
    pRule.push_back(newRule);
    sort(pRule.begin(), pRule.end());
    it = _leafCache.find(pRule);
    if(it != _leafCache.end())
    {
        _leafLookup++;
        result = it->second;
    }
    else
    {
        _leafUnique++;
        result = new SparseDLeaf(parent, newRule, _fullMatrix, _fullLabel, _nPi, _nPat);
        _leafCache[pRule] = result;
    }
    return result;
}

void product(vector< vector<int> >& prev, vector<int> set)
{
    int prevSize = prev.size();
    for(int i=1; i<set.size(); i++)
    {
        for(int k=0; k<prevSize; k++)
        {
            vector<int> tmp = prev[k];
            tmp.push_back(set[i]);
            prev.push_back(tmp);
        }
    }
    for(int k=0; k<prevSize; k++)
        prev[k].push_back(set[0]);
}

vector< vector<int> > IcompactOSDTMgr::getRulesForSplits(SparseDTree * parent)
{
    vector< vector<int> > result;
    
    // appliable aplit rules for each leaf = all rules - leaf rules - useless rules
    vector< vector<int> > appliableRules;
    vector<SparseDLeaf*> pLeafToBeSplit = parent->getLeafToBeSplit();
    vector<int> newRules;
    for(int i=0; i<pLeafToBeSplit.size(); i++)
    {
        newRules = pLeafToBeSplit[i]->getValidSplitRules();
        appliableRules.push_back(newRules);
    }
    
    // cartesian product
    for(int i=0; i<appliableRules[0].size(); i++)
    {
        vector<int> tmp;
        tmp.push_back(appliableRules[0][i]);
        result.push_back(tmp);
    }
    for(int i=1; i<appliableRules.size(); i++)
        product(result, appliableRules[i]);

    return result;
}

DNode * constructDTree(DTree * tree, DNode * parent, vector<SparseDLeaf*> pLeaf, int level)
{
    if(pLeaf.size() == 0) // no leaf
    {
        DNode * node = new DNode();
        node->_id = tree->_pNode.size();
        node->_rule = 0; // is leaf
        node->_label = 0; // don't care
        node->_nParent = parent;
        node->_nLeft = NULL;
        node->_nRight = NULL;
        tree->_pNode.push_back(node);
        return node;
    }
    vector<SparseDLeaf*> pLeftLeaf, pRightLeaf;
    int rule = abs(pLeaf[0]->getRule(level));
    if(rule == 0) // reached end of path - single sparse leaf
    {
        assert(pLeaf.size() == 1);
        DNode * node = new DNode();
        node->_id = tree->_pNode.size();
        node->_rule = 0; // is leaf
        node->_label = pLeaf[0]->getLabel();
        node->_nParent = parent;
        node->_nLeft = NULL;
        node->_nRight = NULL;
        tree->_pNode.push_back(node);
        return node;
    }
    assert(pLeaf.size() > 1);

    for(int i=0; i<pLeaf.size(); i++)
    {
        if(pLeaf[i]->getRule(level) < 0)
            pLeftLeaf.push_back(pLeaf[i]);
        else
            pRightLeaf.push_back(pLeaf[i]);
    }
    DNode * node = new DNode();
    node->_id = tree->_pNode.size();
    node->_rule = rule;
    node->_label = -1;
    node->_nParent = parent;
    node->_nLeft = constructDTree(tree, node, pLeftLeaf, level+1);
    node->_nRight = constructDTree(tree, node, pRightLeaf, level+1);
    tree->_pNode.push_back(node);
    return node;
}

void IcompactOSDTMgr::extractDTree(DNode * n, FILE * fOut)
{
    if(n->_rule == 0) // leaf
    {
        fprintf(fOut, ".names n_%i\n", n->_id);
        if(n->_label == 1)
            fprintf(fOut, "1\n");
    }
    else // internal node
    {
        extractDTree(n->_nLeft, fOut);
        extractDTree(n->_nRight, fOut);
        fprintf(fOut, ".names n_%i n_%i %s n_%i\n\n", \
            n->_nLeft->_id, n->_nRight->_id, _piNames[(n->_rule)-1], n->_id);
        fprintf(fOut, "-11 1\n1-0 1\n");
    }
}

void IcompactOSDTMgr::bestTree2Blif(char * fileName)
{
    vector<SparseDLeaf*> pLeaf = _bestTree->getLeafUnchanged();
    // construct full decision tree
    DTree * result = new DTree();
    constructDTree(result, NULL, pLeaf, 0); // from sparse to normal tree

    // write to blif in topological order
    FILE * fOut = fopen(fileName, "w");
    extractDTree(result->_pNode[0], fOut);
    fclose(fOut);
}

void IcompactOSDTMgr::printStatistics()
{
    printf("Stats:\n");
    printf("  Leaf Unique = %i; Leaf Lookup = %i\n", _leafUnique, _leafLookup);
    printf("  Tree Unique = %i; Tree Lookup = %i\n", _treeUnique, _treeLookup);
}
/************************
 * Class SparseDTree
 ************************/
vector<SparseDLeaf*> SparseDTree::getLeafToBeSplit()
{
    vector<SparseDLeaf*> result;
    for(int i=0, n=_pLeaf.size(); i<n; i++)
        if(_pLeaf[i]->getFSplit())
            result.push_back(_pLeaf[i]);
    return result;
}

vector<SparseDLeaf*> SparseDTree::getLeafUnchanged()
{
    vector<SparseDLeaf*> result;
    for(int i=0, n=_pLeaf.size(); i<n; i++)
        if(!_pLeaf[i]->getFSplit())
            result.push_back(_pLeaf[i]);
    return result;
}

bool SparseDTree::noFurtherSplit()
{
    bool flag = true;
    for(int i=0, n=_pLeaf.size(); i<n; i++)
        if(_pLeaf[i]->getFSplit()) { flag = false; break; }
    return flag;
}

/************************
 * Class SparseDLeaf
 ************************/
SparseDLeaf::SparseDLeaf(SparseDLeaf * parent, int newRule, bool ** fullMatrix, bool * fullLabel, int nPi, int nPat)
{
    int rule, fPos, fNeg, fLabelPos, fLabelNeg;
    _pCapture = new bool[nPat];   
    vector<int> pRules = parent->getRules();
    pRules.push_back(newRule);
    vector<int> pInRules;
    for(int i=0; i<nPi; i++)
        pInRules.push_back(0);
    for(int i=0; i<pRules.size(); i++)
        pInRules[abs(pRules[i]-1)] = 1;
    bool fConflict, phase;
    for(int i=0; i<nPat; i++)
    {
        fConflict = 0;
        for(int r=0; r<pRules.size(); r++)
        {
            rule = abs(pRules[r]) - 1;
            phase = (pRules[r] > 0);
            if(fullMatrix[i][rule] != phase)
            {
                fConflict = 1;
                break;
            }
        }
        _pCapture[i] = !fConflict;
    }

    fLabelPos = fLabelNeg = 0;
    for(int i=0; i<nPat; i++)
    {
        if(_pCapture)
        {
            if(fullLabel[i])
                fLabelPos++;
            if(!fullLabel[i])
                fLabelNeg++;
            if(fLabelPos && fLabelNeg)
                break;
        }
    }
    _fSplit = (fLabelPos && fLabelNeg);
    _label = (fLabelPos > fLabelNeg); // number does not matter when !_fSplit

    for(int r=0; r<nPi; r++)
    {
        if(!pInRules[r])
        {
            fPos = fNeg = 0;
            for(int i=0; i<nPat; i++)
            {
                if(_pCapture)
                {
                    if(fullMatrix[i][r])
                        fPos = 1;
                    if(!fullMatrix[i][r])
                        fNeg = 1;
                    if(fPos && fNeg)
                    {
                        _pValidSplitRules.push_back(r+1);
                        break;
                    }
                }      
            }
        }     
    }
}

SparseDLeaf::SparseDLeaf(bool ** fullMatrix, bool * fullLabel, int nPi, int nPat)
{
    int fPos, fNeg, fLabelPos, fLabelNeg;
    _pCapture = new bool[nPat];    
    for(int i=0; i<nPat; i++)
        _pCapture[i] = 1;
    _nCapture = nPat;

    fLabelPos = fLabelNeg = 0;
    for(int i=0; i<nPat; i++)
    {
        if(fullLabel[i])
            fLabelPos++;
        if(!fullLabel[i])
            fLabelNeg++;
        if(fLabelPos && fLabelNeg)
            break;
    }
    _fSplit = (fLabelPos && fLabelNeg);
    _label = (fLabelPos > fLabelNeg); // number does not matter when !_fSplit

    for(int r=0; r<nPi; r++)
    {
        fPos = fNeg = 0;
        for(int i=0; i<nPat; i++)
        {
            if(fullMatrix[i][r])
                fPos = 1;
            if(!fullMatrix[i][r])
                fNeg = 1;
            if(fPos && fNeg)
            {
                _pValidSplitRules.push_back(r+1);
                break;
            }
                
        }
    }
}
