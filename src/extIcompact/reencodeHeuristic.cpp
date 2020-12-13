#include "reencodeHeuristic.h"
#include "icompact.h"
#include <iostream>
#include <algorithm> // shuffle

int OCTree::print(OCNode* n, int level)
{
    if(n == NULL)
        return 0;

    for(int i=0; i<level; i++)
        printf("  ");
    if(n->getLeft() == NULL)
    {
        printf("* %i (%s)\n", n->getSize(), n->getEncode());
        return n->getSize();
    } 
    else    
        printf("%i\n", n->getPivot());

    int lCount, rCount;
    lCount = print(n->getLeft(), level+1);
    rCount = print(n->getRight(), level+1);

    return lCount + rCount;
}

void ReencodeHeuristicMgr::readFile(char* fileName, bool type)
{
    FILE * fRead = fopen(fileName, "r");
    char buff[10000];
    char *t;
    int nPi, nPo;
    map<string, PartialPattern*> checkMap;

    fgets(buff, 10000, fRead); // .i n
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    nPi = atoi(t);
    fgets(buff, 10000, fRead); // .o m
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    nPo = atoi(t);
    length = (type)? nPi: nPo;
    fgets(buff, 10000, fRead); // .type fr
    while(fgets(buff, 10000, fRead))
    {
        char* one_line_o = new char[length+1];
        one_line_o[length] = '\0';
        t = strtok(buff, " \n"); // get PI pattern
        if(!type)
            t = strtok(NULL, " \n"); // get PO pattern     
        
        std::string s(t);
        if(checkMap.count(s) == 0)
            checkMap[s] = pushbackuniquePat(length, one_line_o);

        for(size_t k=0; k<length; k++)
            one_line_o[k] = t[k];
        pushbackvecPat(length, one_line_o, checkMap[s]);  
    }
    fclose(fRead);
}

int ReencodeHeuristicMgr::getEncoding(int numNewVar)
{
    int limit = 1 << numNewVar;
    OCNode* node, *lnode, *rnode;
    int numRemain = uniquePat.size();
    bool varUsed[length] = {0};
    int varValue[length];
    int pcf, ncf, maxCf, maxIdx;
    vector<int> leafNodeList, tmpNodeList;
    globalFlag++; // flag == globalFlag (dropped)

    curpTree = new OCTree();
    node = curpTree->addNode();
    leafNodeList.push_back(node->getId());
    for(int i=0, n=uniquePat.size(); i<n; i++)
        node->pushbackcontainedPat(i);
    
    while(numRemain > 0)
    {
        // printf("Remaining: %i\n", numRemain);
        for(int i=0; i<length; i++)
            varValue[i] = 0;
        maxCf = maxIdx = 0;
        for(int v=0; v<length; v++)
        {
            if(varUsed[v] != 0)
                continue;
            pcf = ncf = 0;
            for(int l=0; l<leafNodeList.size(); l++)
            {
                node = curpTree->getNode(leafNodeList[l]);
                for(int p=0, n=node->getSize(); p<n; p++)
                {
                    if(uniquePat[node->getPat(p)]->getPatIdx(v) == '1')
                        pcf++;
                    else
                        ncf++; // does not consider '-'s
                    varValue[v] += (pcf<ncf)? pcf: ncf; // add the smaller
                }
            }
            if(varValue[v]>maxCf)
            {
                maxCf = varValue[v];
                maxIdx = v;
            }
        }

        // split
        varUsed[maxIdx] = 1;
        bool fSplit = 0;
        tmpNodeList.clear();
        for(int l=0; l<leafNodeList.size(); l++)
        {
            node = curpTree->getNode(leafNodeList[l]);
            lnode = curpTree->addNode();
            rnode = curpTree->addNode();
            for(int p=0, n=node->getSize(); p<n; p++)
            {
                if(uniquePat[node->getPat(p)]->getPatIdx(maxIdx) == '1')
                    rnode->pushbackcontainedPat(node->getPat(p));
                else
                    lnode->pushbackcontainedPat(node->getPat(p));; // does not consider '-'  
            }
            if(lnode->getSize()==0 || rnode->getSize()==0)
                tmpNodeList.push_back(node->getId());
            else
            {
                fSplit = 1;
                node->setPivot(maxIdx);
                lnode->setParent(node);
                node->setLeft(lnode);
                if(lnode->getSize() <= limit)
                    numRemain -= lnode->getSize();
                else
                    tmpNodeList.push_back(lnode->getId());
                rnode->setParent(node);
                node->setRight(rnode);
                if(rnode->getSize() <= limit)
                    numRemain -= rnode->getSize();
                else
                    tmpNodeList.push_back(rnode->getId());
            }
        }
        if(!fSplit)
        {
            printf("Fail to split. Remaining %i.\n", numRemain);
            break;
        }
        leafNodeList.clear();
        leafNodeList.resize(tmpNodeList.size());
        leafNodeList.assign(tmpNodeList.begin(), tmpNodeList.end());
    }
    
    // get mapping 
    int encodeLength = 0;
    varMap.clear();
    for(int i=0; i<length; i++)
    {
        if(varUsed[i])
        {
            varMap[i] = encodeLength;
            encodeLength++;
        }
    }

    encodeUniquePat(curpTree->getNode(0), encodeLength + numNewVar);

    // for(int i=0; i<uniquePat.size(); i++)
    //     printf("%s %s\n", uniquePat[i]->getPat(), uniquePat[i]->getEncode());

    // int total = curpTree->print(curpTree->getNode(0), 0);
    // printf("Unique pattern: %ld\n", uniquePat.size());
    // printf("Total         : %i\n\n", total);
    result = encodeLength + numNewVar;
    return result;
}

void ReencodeHeuristicMgr::getMapping(bool type, int* record)
{
    if(type) // imap
    {
        map<int, int>::iterator iter;
        for(int i=0; i<result; i++)
            record[i] = -1;
        for(int i=0; i<length; i++)
        {
            iter = varMap.find(i);
            if(iter != varMap.end())
                record[iter->second] = i;
        }
        // printf("varMap:\n");
        // for(int i=0; i<result; i++)
        //     printf("%i, ", record[i]);
        // printf("\n");
    }
    else // omap
    {
        map<int, int>::iterator iter;
        for(int i=0; i<length; i++)
            record[i] = -1;
        for(int i=0; i<length; i++)
        {
            iter = varMap.find(i);
            if(iter != varMap.end())
                record[i] = iter->second;
        }
        // printf("varMap:\n");
        // for(int i=0; i<length; i++)
        //     printf("%i, ", record[i]);
        // printf("\n");
    }   
}

void ReencodeHeuristicMgr::encodeUniquePat(OCNode* n, int len)
{
    int idx;
    char* e = new char[len+1];
    e[len] = '\0';

    if(n->getParent() == NULL)
    {
        for(int i=0; i<len; i++)
            e[i] = '-';
        n->setEncode(e);
    }
    else
    {
        strcpy(e, n->getParent()->getEncode());
        idx = varMap[n->getParent()->getPivot()];
        e[idx] = (n->getParent()->getLeft() == n)? '0': '1'; // bug
        n->setEncode(e);
    }

    if(n->getLeft() != NULL)
    {
        encodeUniquePat(n->getLeft(), len);
        encodeUniquePat(n->getRight(), len);
    }
    else
    {
        size_t encodeLength = varMap.size();
        int numNewVar = 64 - __builtin_clzll(n->getSize()) - 1;
        for(size_t i=0; i<n->getSize(); i++)
        {
            char * final = new char[len+1];
            final[len] = '\0'; 
            strcpy(final, e);
            for(int k=0; k<numNewVar; k++)
                final[encodeLength+k] = ((i >> k) % 2)? '1': '0';
            uniquePat[n->getPat(i)]->setEncode(final);
        }
    }
}
