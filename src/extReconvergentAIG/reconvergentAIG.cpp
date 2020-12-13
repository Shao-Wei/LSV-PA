#include "reconvergentAIG.h"

// node generated selecting all existing nodes as fanin; link all existing nodes to Po at once
Abc_Ntk_t* ReconvergentAIGMgr::generateAND_1(int nNodesMax, int nPi)
{
    _nNodesMax = nNodesMax;
    _nPi = nPi;
    // deleting existing _pNtk causes double free. When & where is ntk freed?
    
    // random generate
    Abc_Obj_t *pObj, *pFanin0, *pFanin1, *pNode, *pPo;
    int i;
    _pNtk = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
    Vec_Ptr_t *vFanins = Vec_PtrAlloc(2);
    for(i=0; i<nPi; i++)
        pObj = Abc_NtkCreatePi(_pNtk); // ID start from 1; ID 0 occupied during Abc_NtkAlloc
    
    // stage 1: random nodes
    while(Abc_NtkNodeNum(_pNtk) < nNodesMax)
    {
        pFanin0 = Abc_NtkObj(_pNtk, rand()%Abc_NtkObjNum(_pNtk)+1);
        Vec_PtrSetEntry(vFanins, 0, pFanin0);
        pFanin1 = Abc_NtkObj(_pNtk, rand()%Abc_NtkObjNum(_pNtk)+1);
        Vec_PtrSetEntry(vFanins, 1, pFanin1);
        assert(pFanin0 != NULL && pFanin1 != NULL);
        pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    }

    // stage 2: connect all nodes
    for(i=0; i<nNodesMax; i++)
    {
        pNode = Abc_NtkObj(_pNtk, i+1);
        pFanin0 = pObj;
        Vec_PtrSetEntry(vFanins, 0, pFanin0);
        pFanin1 = pNode;
        Vec_PtrSetEntry(vFanins, 1, pFanin1);
        assert(pFanin0 != NULL && pFanin1 != NULL);
        pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    }
    pPo = Abc_NtkCreatePo(_pNtk);
    Abc_ObjAddFanin(pPo, pObj);

    // Abc_NtkCleanup(_pNtk, 1); // should not remove any node
    Abc_FrameReplaceCurrentNetwork(_pAbc, _pNtk);
    _flag = executeCommand("strash");
    assert(!_flag);
    _pNtk = Abc_FrameReadNtk(_pAbc);
    _pNtkAnd = Abc_NtkNodeNum(_pNtk);
    _pNtkLev = Abc_NtkLevel(_pNtk);
    // Abc_NtkPrintStats(_pNtk, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return _pNtk;
}

// node generated selecting all existing nodes as fanin; link nodes not connected to Po at once
Abc_Ntk_t* ReconvergentAIGMgr::generateAND_1_aug(int nNodesMax, int nPi)
{
    _nNodesMax = nNodesMax;
    _nPi = nPi;

    // random generate
    Abc_Obj_t *pObj, *pFanin0, *pFanin1, *pNode, *pPo;
    int i, vecAllSize;
    char pName[1000];

    _pNtk = Abc_NtkAlloc(ABC_NTK_STRASH, ABC_FUNC_AIG, 1);
    // _pNtk->pName = Extra_UtilStrsav( pName );
    for(i=0; i<nPi; i++)
    {
        pObj = Abc_NtkCreatePi(_pNtk);
        sprintf(pName, "i%i", i);
        Abc_ObjAssignName( pObj, pName, NULL ); 
    }

    // stage 1: random nodes
    while(Abc_NtkNodeNum(_pNtk) < nNodesMax)
    {
        vecAllSize = Abc_NtkObjNum(_pNtk);
        pFanin0 = Abc_NtkObj(_pNtk, rand()%vecAllSize);
        assert(pFanin0 != NULL);
        pFanin0->fMarkA = 1;
        pFanin1 = Abc_NtkObj(_pNtk, rand()%vecAllSize);
        assert(pFanin1 != NULL);
        pFanin1->fMarkA = 1;
        pObj = Abc_AigAnd((Abc_Aig_t*)_pNtk->pManFunc, pFanin0, pFanin1);
    }
    
    // stage 2: connect all nodes
    pObj->fMarkA = 1;
    vecAllSize = Abc_NtkObjNum(_pNtk);
    for(i=0; i<vecAllSize; i++)
    {
        pNode =  Abc_NtkObj(_pNtk, i);
        assert(pNode != NULL);
        if(pNode->fMarkA == 1)
            continue;
        pFanin0 = pObj;
        pFanin1 = pNode;
        pObj = Abc_AigAnd((Abc_Aig_t*)_pNtk->pManFunc, pFanin0, pFanin1);
    }
    pPo = Abc_NtkCreatePo(_pNtk);
    sprintf(pName, "o0");
    Abc_ObjAssignName( pPo, pName, NULL ); 
    Abc_ObjAddFanin(pPo, pObj);

    Abc_AigCleanup((Abc_Aig_t*) _pNtk->pManFunc);

    // reset fMarkA for ntk delete
    Abc_NtkForEachObj(_pNtk, pObj, i)
        pObj->fMarkA = 0;

    if (!Abc_NtkCheck(_pNtk))
    {
        printf( "The AIG construction has failed.\n" );
        Abc_NtkDelete(_pNtk);
        return NULL;
    }

    Abc_NtkStrash(_pNtk, 0, 0, 0);
    _pNtkAnd = Abc_NtkNodeNum(_pNtk);
    _pNtkLev = Abc_AigLevel(_pNtk);
    Abc_NtkPrintStats(_pNtk, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return _pNtk;
}

// node generated selecting Pi as fanin; link all existing nodes to Po at once
Abc_Ntk_t* ReconvergentAIGMgr::generateAND_2(int nNodesMax, int nPi)
{
    // random generate
    Abc_Obj_t *pObj, *pFanin0, *pFanin1, *pNode, *pPo;
    int i;
    _pNtk = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
    Vec_Ptr_t *vFanins = Vec_PtrAlloc(2);
    for(i=0; i<nPi; i++)
        pObj = Abc_NtkCreatePi(_pNtk); // ID start from 1; ID 0 occupied during Abc_NtkAlloc
    
    // stage 1: random nodes
    while(Abc_NtkNodeNum(_pNtk) < nNodesMax)
    {
        pFanin0 = Abc_NtkPi(_pNtk, rand()%nPi);
        Vec_PtrSetEntry(vFanins, 0, pFanin0);
        pFanin1 = Abc_NtkPi(_pNtk, rand()%nPi);
        Vec_PtrSetEntry(vFanins, 1, pFanin1);
        assert(pFanin0 != NULL && pFanin1 != NULL);
        pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    }

    // stage 2: connect all nodes
    for(i=0; i<nNodesMax; i++)
    {
        pNode = Abc_NtkObj(_pNtk, i+1);
        if(Abc_ObjIsPi(pNode))
            continue;
        pFanin0 = pObj;
        Vec_PtrSetEntry(vFanins, 0, pFanin0);
        pFanin1 = pNode;
        Vec_PtrSetEntry(vFanins, 1, pFanin1);
        assert(pFanin0 != NULL && pFanin1 != NULL);
        pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    }
    pPo = Abc_NtkCreatePo(_pNtk);
    Abc_ObjAddFanin(pPo, pObj);

    // Abc_NtkCleanup(_pNtk, 1); // should not remove any node
    Abc_FrameReplaceCurrentNetwork(_pAbc, _pNtk);
    _flag = executeCommand("strash");
    assert(!_flag);
    _pNtk = Abc_FrameReadNtk(_pAbc);
    _pNtkAnd = Abc_NtkNodeNum(_pNtk);
    _pNtkLev = Abc_NtkLevel(_pNtk);
    // Abc_NtkPrintStats(_pNtk, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return _pNtk;
}

// node generated selecting one extra Pi as fanin; targeting ntk w/ high level
Abc_Ntk_t* ReconvergentAIGMgr::generateAND_3(int nNodesMax, int nPi)
{
    // random generate
    Abc_Obj_t *pObj, *pFanin0, *pFanin1, *pNode, *pPo;
    int i;
    _pNtk = Abc_NtkAlloc(ABC_NTK_LOGIC, ABC_FUNC_AIG, 1);
    Vec_Ptr_t *vFanins = Vec_PtrAlloc(2);
    for(i=0; i<nPi; i++)
        pObj = Abc_NtkCreatePi(_pNtk); // ID start from 1; ID 0 occupied during Abc_NtkAlloc
    
    pFanin0 = Abc_NtkPi(_pNtk, rand()%nPi);
    Vec_PtrSetEntry(vFanins, 0, pFanin0);
    pFanin1 = Abc_NtkPi(_pNtk, rand()%nPi);
    Vec_PtrSetEntry(vFanins, 1, pFanin1);
    assert(pFanin0 != NULL && pFanin1 != NULL);
    pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    while(Abc_NtkNodeNum(_pNtk) < nNodesMax)
    {
        pFanin0 = pObj;
        Vec_PtrSetEntry(vFanins, 0, pFanin0);
        pFanin1 = Abc_NtkPi(_pNtk, rand()%nPi);
        Vec_PtrSetEntry(vFanins, 1, pFanin1);
        assert(pFanin0 != NULL && pFanin1 != NULL);
        pObj = Abc_NtkCreateNodeAnd(_pNtk, vFanins);
    }

    pPo = Abc_NtkCreatePo(_pNtk);
    Abc_ObjAddFanin(pPo, pObj);

    // Abc_NtkCleanup(_pNtk, 1); // should not remove any node
    Abc_FrameReplaceCurrentNetwork(_pAbc, _pNtk);
    _flag = executeCommand("strash");
    assert(!_flag);
    _pNtk = Abc_FrameReadNtk(_pAbc);
    _pNtkAnd = Abc_NtkNodeNum(_pNtk);
    _pNtkLev = Abc_NtkLevel(_pNtk);
    // Abc_NtkPrintStats(_pNtk, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return _pNtk;
}

void ReconvergentAIGMgr::minimize(int nIter)
{
    if(_pNtk == NULL)
    {
        printf("No generated AIG.\n");
        return;
    }
    
    // minimize
    // Abc_NtkDup & Abc_FrameReplaceCurrentNetwork mess up _pNtk type causing strash failure
    Abc_Ntk_t *pNtkTmp = Abc_NtkDup(_pNtk);
    Abc_FrameReplaceCurrentNetwork(_pAbc, pNtkTmp); 
    
    for(int i=0; i<nIter; i++)
    {
        _flag = executeCommand("balance -l");
        assert(!_flag);
        _flag = executeCommand("resub -K 6 -l");
        assert(!_flag);
        _flag = executeCommand("rewrite -l");
        assert(!_flag);
    }
    _flag = executeCommand("strash");
    assert(!_flag);
    _pNtkNew = Abc_FrameReadNtk(_pAbc);
    _pNtkNewAnd = Abc_NtkNodeNum(_pNtkNew);
    _pNtkNewLev = Abc_NtkLevel(_pNtkNew);
    // Abc_NtkPrintStats(_pNtkNew, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int ReconvergentAIGMgr::minimizeCurve(int nIterMax, char* fileName)
{
    if(_pNtk == NULL)
    {
        printf("No generated AIG.\n");
        return 0;
    }
    
    // minimize
    // Abc_NtkDup & Abc_FrameReplaceCurrentNetwork mess up _pNtk type causing strash failure
    int prevAnd = _pNtkAnd;
    // printf("_pNtk type: %i\n", _pNtk->ntkType);
    // Abc_Ntk_t *pNtkTmp = Abc_NtkDup(_pNtk);
    Abc_FrameReplaceCurrentNetwork(_pAbc, _pNtk); 
    FILE* f = fopen(fileName, "a");
    fprintf(f, "Original, %i, %i\n", _pNtkAnd, _pNtkLev);
    for(int i=0; i<nIterMax; i++)
    {
        _flag = executeCommand("balance -l");
        assert(!_flag);
        _flag = executeCommand("resub -K 6 -l");
        assert(!_flag);
        _flag = executeCommand("rewrite -l");
        assert(!_flag);
        _flag = executeCommand("strash");
        assert(!_flag);
        _pNtkNew = Abc_FrameReadNtk(_pAbc);
        _pNtkNewAnd = Abc_NtkNodeNum(_pNtkNew);
        _pNtkNewLev = Abc_NtkLevel(_pNtkNew);
        if(i%10 == 0)
            printf("iter %i: AND %i, LEV %i.\n", i, _pNtkNewAnd, _pNtkNewLev);
        fprintf(f, "%i, %i, %i\n", i, _pNtkNewAnd, _pNtkNewLev);
        if(_pNtkNewAnd < prevAnd)
            prevAnd = _pNtkNewAnd;
        else
        {
            // printf("Reached end at iter %i (max: %i)\n", i, nIterMax);
            fprintf(f, "Reached end at iter %i (max: %i)\n", i, nIterMax);
            fclose(f);
            return i;
        }
    }
    fclose(f);
    return nIterMax;
}

void ReconvergentAIGMgr::eval()
{
    printf("nNodesMax = %i; nPi = %i\n", _nNodesMax, _nPi);
    printf("\tAND: Begin = %i. End = %i. ( R = %d%% )\n", _pNtkAnd, _pNtkNewAnd, 100*(_pNtkAnd - _pNtkNewAnd)/_pNtkAnd);
    printf("\tLEV: Begin = %i. End = %i. ( R = %d%% )\n", _pNtkLev, _pNtkNewLev, 100*(_pNtkLev - _pNtkNewLev)/_pNtkLev);
}

void ReconvergentAIGMgr::printLog(char* fileName)
{
    FILE* f = fopen(fileName, "a");
    fprintf(f, "%i, %i, %i, %i, %i\n", _nNodesMax, _pNtkAnd, _pNtkNewAnd, _pNtkLev, _pNtkNewLev);
    fclose(f);
}

bool ReconvergentAIGMgr::executeCommand(char* s)
{
    sprintf( _command, s);
    if(Cmd_CommandExecute(_pAbc, _command))
    {
        printf("Failed at %s\n", s);
        return 1;
    }
    return 0;
}