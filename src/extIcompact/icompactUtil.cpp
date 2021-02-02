#include "icompact.h"

ABC_NAMESPACE_IMPL_START

// returns the number of input variables used in the cubes of the pla
int espresso_input_count(char* filename)
{
    char buff[102400];
    char* t;
    char * unused __attribute__((unused));

    int nPi;
    bool* lit;
    int count;

    FILE* f = fopen(filename, "r");
    unused = fgets(buff, 102400, f); // skip
    t = strtok(buff, " \n"); // skip
    t = strtok(NULL, " \n");
    nPi = atoi(t);
    unused = fgets(buff, 102400, f); // skip
    unused = fgets(buff, 102400, f); // skip
    
    lit = new bool[nPi];
    for(int i=0; i<nPi; i++)
        lit[i] = 0;
    while(fgets(buff, 102400, f))
    {
        if(buff[1] == 'e')
            break;
        for(int i=0; i<nPi; i++)
        {
            if(buff[i] == '1' || buff[i] == '0')
                lit[i] = 1;
        }
    }

    count = 0;
    for(int i=0; i<nPi; i++)
    {
        if(lit[i] == 1)
            count++;
    }

    // clean up
    delete [] lit;
    fclose(f);

    return count;
}

// checks if pi/po is consistant in pFileName
int check_pla_pipo(char *pFileName, int nPi, int nPo)
{
    char buff[102400];
    char * t;
    char * unused __attribute__((unused));
    FILE* fPla = fopen(pFileName, "r");

    unused = fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPi) { fclose(fPla); return 1; }
    unused = fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPo) { fclose(fPla); return 1; }
    
    fclose(fPla);
    return 0;
}

// returns 1 if all sim pat are correct, 0 otherwise, -1 if failed
int ntkVerifySamples(Abc_Ntk_t* pNtk, char *pFile, int fVerbose)
{
    int result;
    int nPi_file, nPo_file, nPi_ntk, nPo_ntk;
    Aig_Man_t * pAig;

    // checkings
    assert(pNtk != NULL);
    Abc_Ntk_t *pNtkFile = Io_Read(pFile, Io_ReadFileType(pFile), 1, 0);
    if(pNtk == NULL)
    {
        printf("Verify circuit: Bad input file %s.\n", pFile);
        return -1;
    }
    nPi_file = Abc_NtkPiNum(pNtkFile);
    nPo_file = Abc_NtkPoNum(pNtkFile);
    nPi_ntk = Abc_NtkPiNum(pNtk);
    nPo_ntk = Abc_NtkPoNum(pNtk);
    if(nPi_file != nPi_ntk || nPo_file != nPo_ntk)
    {
        printf("Verify circuit: Inconsistent pi/po.\n");
        return -1;
    }

    // verify
    pAig = Abc_NtkToDar(pNtk, 0, 0);
    if(!fVerbose)
        result = smlVerifyCombGiven(pAig, pFile, NULL);
    else
    {
        int * pCount = new int[nPo_file]();
        result = smlVerifyCombGiven(pAig, pFile, pCount);
        printf("correct pat num for each po:\n  ");
        for(int i=0; i<nPo_file; i++)
            printf(" %i", pCount[i]);
        printf("\n");

        delete [] pCount;
    }
    
    return result;
}

// modified from base/abc/abcCheck.c Abc_NtkComparePis()
int ntkComparePis( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_Obj_t * pObj1;
    int i;
    if ( Abc_NtkPiNum(pNtk1) != Abc_NtkPiNum(pNtk2) )
    {
        // printf( "Networks have different number of primary inputs.\n" );
        return 0;
    }
    // for each PI of pNet1 find corresponding PI of pNet2 and reorder them
    Abc_NtkForEachPi( pNtk1, pObj1, i )
    {
        if ( strcmp( Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) ) != 0 )
        {
            printf( "Primary input #%d is different in network 1 ( \"%s\") and in network 2 (\"%s\").\n", 
                i, Abc_ObjName(pObj1), Abc_ObjName(Abc_NtkPi(pNtk2,i)) );
            return 0;
        }
    }
    return 1;
}

// modified from base/abc/abcCheck.c Abc_NtkCompareSignals()
int ntkCompareSignals( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2, int fComb )
{
    Abc_NtkOrderObjsByName( pNtk1, fComb );
    Abc_NtkOrderObjsByName( pNtk2, fComb );
    if ( !ntkComparePis( pNtk1, pNtk2, fComb ) )
        return 0;
    return 1;
}

// modified from base/abci/abcStrash.c Abc_NtkAppend()
// fAllPos set to 1 ( = 0 causes unresolved error ). Warnings in Abc_NtkCompareSignals() removed.
int ntkAppend( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2)
{
    Abc_Obj_t * pObj;
    char * pName;
    int i, nNewCis;
    // the first network should be an AIG
    assert( Abc_NtkIsStrash(pNtk1) );
    assert( Abc_NtkIsStrash(pNtk2) ); 
    if ( Abc_NtkIsLogic(pNtk2) && !Abc_NtkToAig(pNtk2) )
    {
        printf( "Converting to AIGs has failed.\n" );
        return 0;
    }
    // check that the networks have the same PIs
    // reorder PIs of pNtk2 according to pNtk1
    ntkCompareSignals( pNtk1, pNtk2, 1 );

    // perform strashing
    nNewCis = 0;
    Abc_NtkCleanCopy( pNtk2 );
    if ( Abc_NtkIsStrash(pNtk2) )
        Abc_AigConst1(pNtk2)->pCopy = Abc_AigConst1(pNtk1);
    Abc_NtkForEachCi( pNtk2, pObj, i )
    {
        pName = Abc_ObjName(pObj);
        pObj->pCopy = Abc_NtkFindCi(pNtk1, Abc_ObjName(pObj));
        if ( pObj->pCopy == NULL )
        {
            pObj->pCopy = Abc_NtkDupObj(pNtk1, pObj, 1);
            nNewCis++;
        }
    }
    if ( nNewCis )
        printf( "Warning: Procedure Abc_NtkAppend() added %d new CIs.\n", nNewCis );
    
    Abc_NtkForEachNode( pNtk2, pObj, i )
        pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtk1->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );
    Abc_NtkForEachPo( pNtk2, pObj, i )
    {
        Abc_NtkDupObj( pNtk1, pObj, 0 );
        Abc_ObjAddFanin( pObj->pCopy, Abc_ObjChild0Copy(pObj) );
        Abc_ObjAssignName( pObj->pCopy, Abc_ObjName(pObj), NULL );
    }
    // make sure that everything is okay
    if ( !Abc_NtkCheck( pNtk1 ) )
    {
        printf( "Abc_NtkAppend: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

char ** setDummyNames(int len, char * baseStr)
{
    char ** result = new char*[len];
    for(int i=0; i<len; i++)
    {
        char * buffer = new char[64]();
        sprintf(buffer, "%s%i", baseStr, i);
        result[i] = buffer;
    }
    return result;
}

bool singleSupportComplement(char * pFileName, int piIdx, int poIdx)
{
    char iBit, oBit;
    char buff[102400];
    char * unused __attribute__((unused)); // get rid of fget warnings
    FILE* fpattern = fopen(pFileName, "r");
    for(int i=0; i<6; i++) // get first pattern
        unused = fgets(buff, 102400, fpattern);
    
    iBit = buff[piIdx];
    oBit = buff[poIdx];

    fclose(fpattern);
    return (iBit != oBit);
}

void removeSTFault(Aig_Man_t * pAig, Aig_Obj_t * pObj, vector< pair<Aig_Obj_t*, int> >& vFanout, int signal)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, * pTmp;
    for(int i=0, n=vFanout.size(); i<n; i++)
    {
        pFanout = vFanout[i].first;
        // printf("removeSTF %i, %i, %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout) );
        if( Aig_ObjFaninId0(pFanout) == 0 )
        {
            
            pNewFanin0 = (vFanout[i].second)? Aig_Not(pObj): pObj;
            pNewFanin1 = pFanout->pFanin1;
        }
        else
        {
            pNewFanin0 = pFanout->pFanin0;
            pNewFanin1 = (vFanout[i].second)? Aig_Not(pObj): pObj;
        }
        Aig_ObjDisconnect(pAig, pFanout);

        if(pNewFanin0 == NULL)
        {
            pNewFanin0 = pNewFanin1;
            pNewFanin1 = NULL;
        }
        else 
        {
            if (pNewFanin1 != NULL)
            {
                if(Aig_Regular(pNewFanin0)->Id > Aig_Regular(pNewFanin1)->Id)
                {
                    pTmp = pNewFanin0;
                    pNewFanin0 = pNewFanin1;
                    pNewFanin1 = pTmp;
                }
            }       
        }
        Aig_ObjConnect(pAig, pFanout, pNewFanin0, pNewFanin1);
    }
}

int insertSTFault(Aig_Man_t * pAig, Aig_Obj_t * pObj, vector< pair<Aig_Obj_t*, int> >& vFanout, int signal)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, *pTmp;
    int iFanout = -1, k, c;
    Aig_ObjForEachFanout(pAig, pObj, pFanout, iFanout, k)
    {     
        // printf("insertSTF %i, %i, %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout) );
        if(Aig_ObjFanin0(pFanout) == Aig_Regular(pObj))
        {
            c = Aig_ObjFaninC0(pFanout);
            pNewFanin0 = (signal)? Aig_ManConst1(pAig): Aig_ManConst0(pAig);
            pNewFanin1 = pFanout->pFanin1;
        }
        else if (Aig_ObjFanin1(pFanout) == Aig_Regular(pObj))
        {
            c = Aig_ObjFaninC1(pFanout);
            pNewFanin0 = pFanout->pFanin0;
            pNewFanin1 = (signal)? Aig_ManConst1(pAig): Aig_ManConst0(pAig);
        }
        else // collision
        {
            // printf("(skip)\n");
            continue;
        }
        vFanout.push_back( make_pair(pFanout, c) );
        Aig_ObjDisconnect(pAig, pFanout);
        // if( Aig_TableLookup(pAig, pFanout) != NULL )
        //     printf("not deleted: %i, %i, %i\n", Aig_Regular(pFanout)->Id, Aig_ObjFanin0(pFanout)->Id, Aig_ObjFanin1(pFanout)->Id);

        if(pNewFanin0 == NULL)
        {
            pNewFanin0 = pNewFanin1;
            pNewFanin1 = NULL;
        }
        else 
        {
            if (pNewFanin1 != NULL)
            {
                if(Aig_Regular(pNewFanin0)->Id > Aig_Regular(pNewFanin1)->Id)
                {
                    pTmp = pNewFanin0;
                    pNewFanin0 = pNewFanin1;
                    pNewFanin1 = pTmp;
                }
            }       
        }
        Aig_ObjConnect(pAig, pFanout, pNewFanin0, pNewFanin1); 
    }
    return 1;
}

Abc_Ntk_t * ntkSTFault(Abc_Ntk_t * pNtk, char * simFileName)
{
    int nSuccess;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);
    Aig_ManFanoutStart(pAig);

    // find candidate STF
    vector< pair<int, int> > vCandidate;
    smlSTFaultCandidate(pAig, simFileName, vCandidate);
    nSuccess = 0;
    for(int i=0, n=vCandidate.size(); i<n; i++)
    {
        pObj = Aig_ManObj(pAig, vCandidate[i].first);
        // printf("pObj id: %i\n", pObj->Id);
        vector< pair<Aig_Obj_t*, int> > vFanout;
        insertSTFault(pAig, pObj, vFanout, vCandidate[i].second);
        if(smlVerifyCombGiven(pAig, simFileName, NULL))
        {
            Aig_ManCleanup(pAig);
            nSuccess++;
        }       
        else
        {
            removeSTFault(pAig, pObj, vFanout, vCandidate[i].second);
            // if(!smlVerifyCombGiven(pAig, simFileName, NULL))
            //     printf("FFFFKKKKKKK\n");
        }
    }
    printf("Success in inserting ST0/1 at candidates: %i / %lu\n", nSuccess, vCandidate.size());
    Aig_ManCleanup(pAig);
    pNtkNew = Abc_NtkFromDar(pNtk, pAig);
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);

    // make sure everything is okay
    if(!ntkVerifySamples(pNtkNew, simFileName, 0))
    {
        printf("pNtk fucked up\n");
        return NULL;
    }
    else
        printf("Network size: %i / %i\n", Abc_NtkNodeNum(pNtkNew), Abc_NtkNodeNum(pNtk));

    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        return NULL;
    }

    // clean up
    Aig_ManFanoutStop(pAig);
    Aig_ManStop(pAig);
    return pNtkNew;
}

ABC_NAMESPACE_IMPL_END