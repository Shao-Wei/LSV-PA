#include "icompact.h"
#include "opt/rwr/rwr.h" // ntkRewrite
#include "bool/dec/dec.h" // ntkRewrite
#include "opt/cut/cutInt.h"

ABC_NAMESPACE_IMPL_START

/////////////////////////////////////////////////////////
// Aux Functions
/////////////////////////////////////////////////////////
// print in binary
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

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

// check specific bit on first pattern
bool firstPatternOneBit(char * pFileName, int idx)
{
    char bit;
    char buff[102400];
    char * unused __attribute__((unused)); // get rid of fget warnings
    FILE* fpattern = fopen(pFileName, "r");
    for(int i=0; i<6; i++) // get first pattern
        unused = fgets(buff, 102400, fpattern);
    
    bit = buff[idx];

    fclose(fpattern);
    return (bit == '1');
}

// check if two bits are complement on first pattern
bool firstPatternTwoBits(char * pFileName, int piIdx, int poIdx)
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
    return (iBit == oBit);
}

/////////////////////////////////////////////////////////
// Network Verify
// returns 1 if all sim pat are correct, 0 otherwise, -1 if failed
/////////////////////////////////////////////////////////
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
    result = smlVerifyCombGiven(pAig, pFile, fVerbose);
    
    return result;
}

/////////////////////////////////////////////////////////
// Network Append
// fAllPos set to 1 ( = 0 causes unresolved error ). Warnings in Abc_NtkCompareSignals() removed
/////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////
// Network STF Insertion
/////////////////////////////////////////////////////////
void removeSTFault(Aig_Man_t * pAig, Aig_Obj_t * pObj, vector< pair<Aig_Obj_t*, int> >& vFanout, int signal)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, * pTmp;
    for(int i=0, n=vFanout.size(); i<n; i++)
    {
        pFanout = vFanout[i].first;
        // printf("removeSTF %i, %i, %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout) );
        pNewFanin0 = (Aig_Obj_t*)pFanout->pData;
        pNewFanin1 = (vFanout[i].second)? Aig_Not(pObj): pObj;
        pFanout->pData = NULL;

        Aig_ObjDisconnect(pAig, pFanout);

        // special case when both child are const
        if(Aig_ObjIsBuf(pFanout))
        {
            pFanout->Type = AIG_OBJ_AND;
            // Vec_PtrRemove( pAig->vBufs, pFanout);
            // pAig->nObjs[AIG_OBJ_AND]++;
            // pAig->nObjs[AIG_OBJ_BUF]--;
        }   

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

void insertSTFault(Aig_Man_t * pAig, Aig_Obj_t * pObj, vector< pair<Aig_Obj_t*, int> >& vFanout, int signal)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, *pTmp;
    int iFanout = -1, k, c;
    Aig_ObjForEachFanout(pAig, pObj, pFanout, iFanout, k)
    {     
        if(Aig_ObjFanin0(pFanout) == Aig_Regular(pObj))
        {
            c = Aig_ObjFaninC0(pFanout); // reserve phase to pObj
            pFanout->pData = (void*)pFanout->pFanin1; // reserve original fanin
            pNewFanin0 = (signal)? Aig_ManConst1(pAig): Aig_ManConst0(pAig);
            pNewFanin1 = pFanout->pFanin1;
        }
        else if (Aig_ObjFanin1(pFanout) == Aig_Regular(pObj))
        {
            c = Aig_ObjFaninC1(pFanout);
            pFanout->pData = (void*)pFanout->pFanin0;
            pNewFanin0 = pFanout->pFanin0;
            pNewFanin1 = (signal)? Aig_ManConst1(pAig): Aig_ManConst0(pAig);
        }
        else // wrong fanout, collision maybe
        {
            // printf("(skip)\n");
            continue;
        }
        // printf("insertSTF %i, %i, %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout) );
        vFanout.push_back( make_pair(pFanout, c) );
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
                // special case when both child are const after insertion
                if(Aig_Regular(pNewFanin0)->Id == Aig_Regular(pNewFanin1)->Id)
                {
                    if(Aig_ObjIsAnd(pFanout))
                    {
                        pFanout->Type = AIG_OBJ_BUF;
                        // Vec_PtrPush( pAig->vBufs, pFanout);
                        // pAig->nBufMax = Abc_MaxInt( pAig->nBufMax, Vec_PtrSize(pAig->vBufs) );
                        // pAig->nObjs[AIG_OBJ_AND]--;
                        // pAig->nObjs[AIG_OBJ_BUF]++;
                    }
                    pNewFanin0 = (Aig_ObjFaninC0(pFanout) ^ Aig_ObjFaninC1(pFanout))? Aig_ManConst0(pAig): Aig_ManConst1(pAig);
                    pNewFanin1 = NULL;
                }
            }       
        }
        Aig_ObjConnect(pAig, pFanout, pNewFanin0, pNewFanin1); 
    }
}

Abc_Ntk_t * ntkSTFault(Abc_Ntk_t * pNtk, char * simFileName, int fVerbose)
{
    extern int smlSTFaultCandidate2( Aig_Man_t * pAig, char * pFileName, vector< pair<int, int> >& vCandidate);
    int nSuccess = 0, nSkipped = 0;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj;
    int sizeOld = Abc_NtkNodeNum(pNtk), sizeNew;
    abctime clk, clkStart, timeCand = 0, timeVerify = 0, timeTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );

clkStart = Abc_Clock();
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);
    Aig_ManFanoutStart(pAig);
    if(!smlVerifyCombGiven(pAig, simFileName, 0))
    {
        printf("Bad pNtk / sim file pair at ntk STF\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

clk = Abc_Clock();
    // find candidate STF
    vector< pair<int, int> > vCandidate;
    smlSTFaultCandidate2(pAig, simFileName, vCandidate); // candidate in reverse topological order
    if(vCandidate.size() == 0)
    {
        printf("No STF candidate\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }
timeCand += Abc_Clock() - clk;
    for(int i=0, n=vCandidate.size(); i<n; i++)
    {
        pObj = Aig_ManObj(pAig, vCandidate[i].first);
        if(pObj == NULL) // removed by previous STF insertion
        {
            nSkipped++;
            continue;
        }

        vector< pair<Aig_Obj_t*, int> > vFanout;
        insertSTFault(pAig, pObj, vFanout, vCandidate[i].second);
clk = Abc_Clock();
        if(smlVerifyCombGiven(pAig, simFileName, 0))
        {
//             printf("STF inserted at node %i\n", Aig_Regular(pObj)->Id);
            Aig_ManCleanup(pAig);
            nSuccess++;
        }       
        else
        {
            removeSTFault(pAig, pObj, vFanout, vCandidate[i].second);
        }
timeVerify += Abc_Clock() - clk;
    }
    Aig_ManCleanup(pAig);
    pNtkNew = Abc_NtkFromDar(pNtk, pAig);
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);
    sizeNew = Abc_NtkNodeNum(pNtkNew);

    // make sure everything is okay
    if ( !ntkVerifySamples(pNtkNew, simFileName, 1) || !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

timeTotal += Abc_Clock() - clkStart;
    // report stats
    if(fVerbose)
    {
        printf( "> STFault Insertion Statistics:\n");
        printf( "  Total Insertions  = %i (%i skipped) / %lu.\n", nSuccess, nSkipped, vCandidate.size());
        printf( "  Gain              = %8d. (%6.2f %%).\n", sizeOld - sizeNew, 100.0*(sizeOld - sizeNew)/sizeOld );
        ABC_PRT( "  Candidate   ", timeCand );
        ABC_PRT( "  Verify      ", timeVerify );
        ABC_PRT( "  Total       ", timeTotal );
    }
    // clean up
    Aig_ManFanoutStop(pAig);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pNtk);
    return pNtkNew;
}

/////////////////////////////////////////////////////////
// Network Signal Merging
/////////////////////////////////////////////////////////
void signalUnMerge(Aig_Man_t * pAig, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2, vector< pair<Aig_Obj_t*, int> >& vFanout)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, * pTmp;
    for(int i=0, n=vFanout.size(); i<n; i++)
    {
        pFanout = vFanout[i].first;
        // printf("unmerge %i, %i, %i to %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout), Aig_Regular(pObj2)->Id );
        pNewFanin0 = (Aig_Obj_t*)pFanout->pData;
        pNewFanin1 = (vFanout[i].second)? Aig_Not(pObj2): pObj2;
        pFanout->pData = NULL;

        Aig_ObjDisconnect(pAig, pFanout);

        // special case when both child are const
        if(Aig_ObjIsBuf(pFanout))
        {
            pFanout->Type = AIG_OBJ_AND;
            // Vec_PtrRemove( pAig->vBufs, pFanout);
            // pAig->nObjs[AIG_OBJ_AND]++;
            // pAig->nObjs[AIG_OBJ_BUF]--;
        }   

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

void signalMerge(Aig_Man_t * pAig, Aig_Obj_t * pObj1, Aig_Obj_t * pObj2, vector< pair<Aig_Obj_t*, int> >& vFanout)
{
    Aig_Obj_t * pFanout, * pNewFanin0, * pNewFanin1, *pTmp;
    int iFanout = -1, k, c;
    int comp = Aig_IsComplement(pObj1) ^ Aig_IsComplement(pObj2);
    Aig_ObjForEachFanout(pAig, pObj2, pFanout, iFanout, k)
    {     
        if(Aig_ObjFanin0(pFanout) == Aig_Regular(pObj2))
        {
            c = Aig_ObjFaninC0(pFanout); // reserve phase to pObj2
            pFanout->pData = (void*)pFanout->pFanin1; // reserve original fanin
            pNewFanin0 = (comp ^ c)? Aig_Not(Aig_Regular(pObj1)): Aig_Regular(pObj1);
            pNewFanin1 = pFanout->pFanin1;
        }
        else if (Aig_ObjFanin1(pFanout) == Aig_Regular(pObj2))
        {
            c = Aig_ObjFaninC1(pFanout);
            pFanout->pData = (void*)pFanout->pFanin0;
            pNewFanin0 = pFanout->pFanin0;
            pNewFanin1 = (comp ^ c)? Aig_Not(Aig_Regular(pObj1)): Aig_Regular(pObj1);
        }
        else // wrong fanout, collision maybe
        {
            // printf("(skip)\n");
            continue;
        }
        // printf("merge %i, %i, %i to %i\n", pFanout->Id, Aig_ObjFaninId0(pFanout), Aig_ObjFaninId1(pFanout), Aig_Regular(pObj1)->Id );
        vFanout.push_back( make_pair(pFanout, c) );
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
                // special case when both child are same node after insertion
                if(Aig_Regular(pNewFanin0)->Id == Aig_Regular(pNewFanin1)->Id)
                {
                    if(Aig_ObjIsAnd(pFanout))
                    {
                        pFanout->Type = AIG_OBJ_BUF;
                        // Vec_PtrPush( pAig->vBufs, pFanout);
                        // pAig->nBufMax = Abc_MaxInt( pAig->nBufMax, Vec_PtrSize(pAig->vBufs) );
                        // pAig->nObjs[AIG_OBJ_AND]--;
                        // pAig->nObjs[AIG_OBJ_BUF]++;
                    }
                    pNewFanin0 = (Aig_ObjFaninC0(pFanout) ^ Aig_ObjFaninC1(pFanout))? Aig_Not(Aig_Regular(pObj1)): Aig_Regular(pObj1);
                    pNewFanin1 = NULL;
                }
            }       
        }
        Aig_ObjConnect(pAig, pFanout, pNewFanin0, pNewFanin1); 
    }
}

Abc_Ntk_t * ntkSignalMerge(Abc_Ntk_t * pNtk, char * simFileName, int fVerbose)
{
    extern int smlSignalMergeCandidate2( Aig_Man_t * pAig, char * pFileName, vector< pair<int, int> >& vCandidate);
    int nSuccess = 0, nSkipped = 0;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pObj1, * pObj2;
    int sizeOld = Abc_NtkNodeNum(pNtk), sizeNew;
    abctime clk, clkStart, timeCand = 0, timeVerify = 0, timeTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );

clkStart = Abc_Clock();    
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);
    Aig_ManFanoutStart(pAig);
    if(!smlVerifyCombGiven(pAig, simFileName, 0))
    {
        printf("Bad pNtk / sim file pair at ntk Merge\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

clk = Abc_Clock();
    // find candidate merge signals
    vector< pair<int, int> > vCandidate;
    smlSignalMergeCandidate2(pAig, simFileName, vCandidate); // candidate in topological order
    if(vCandidate.size() == 0)
    {
        printf("No signal merge candidate\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }
timeCand += Abc_Clock() - clk;
    for(int i=vCandidate.size()-1; i>=0; i--)
    {
        pObj1 = Aig_ManObj(pAig, vCandidate[i].first);
        pObj2 = Aig_ManObj(pAig, vCandidate[i].second);
        if(pObj1 == NULL || pObj2 == NULL) // removed by previous merging
        {
            nSkipped++;
            continue;
        }

        // try pObj2 merged to pObj1
        vector< pair<Aig_Obj_t*, int> > vFanout2;
        signalMerge(pAig, pObj1, pObj2, vFanout2);
clk = Abc_Clock();
        if(smlVerifyCombGiven(pAig, simFileName, 0))
        {
//             printf("Node %i merged to node %i\n", Aig_Regular(pObj2)->Id, Aig_Regular(pObj1)->Id);
            Aig_ManCleanup(pAig);
            nSuccess++;
        }       
        else
        {
            signalUnMerge(pAig, pObj1, pObj2, vFanout2);
        }
timeVerify += Abc_Clock() - clk;
    }
    Aig_ManCleanup(pAig);
    pNtkNew = Abc_NtkFromDar(pNtk, pAig);
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);
    sizeNew = Abc_NtkNodeNum(pNtkNew);
    // printf("Network size: %i / %i\n", Abc_NtkNodeNum(pNtkNew), Abc_NtkNodeNum(pNtk));

    // make sure everything is okay
//     if(!ntkVerifySamples(pNtkNew, simFileName, 0))
//     {
//         printf("The simulation check has failed.\n");
//         return NULL;
//     }
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

timeTotal += Abc_Clock() - clkStart;
    // report stats
    if(fVerbose)
    {
        printf( "> Signal Merging Statistics:\n");
        printf( "  Total Mergings    = %i (%i skipped) / %lu.\n", nSuccess, nSkipped, vCandidate.size());
        printf( "  Gain              = %8d. (%6.2f %%).\n", sizeOld - sizeNew, 100.0*(sizeOld - sizeNew)/sizeOld );
        ABC_PRT( "  Candidate   ", timeCand );
        ABC_PRT( "  Verify      ", timeVerify );
        ABC_PRT( "  Total       ", timeTotal );
    }
    // clean up
    Aig_ManFanoutStop(pAig);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pNtk);
    return pNtkNew;
}

Abc_Ntk_t * ntkSignalMerge2(Abc_Ntk_t * pNtk, char * simFileName, int fVerbose)
{
    extern int smlSignalMergeCandidate2( Aig_Man_t * pAig, char * pFileName, vector< pair<int, int> >& vCandidate);
    int nSuccess = 0, nSkipped = 0;
    Abc_Ntk_t * pNtkNew;
    Aig_Man_t * pAig;
    Aig_Obj_t * pObj1, * pObj2;
    int sizeOld = Abc_NtkNodeNum(pNtk), sizeNew;
    abctime clk, clkStart, timeCand = 0, timeVerify = 0, timeTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );

clkStart = Abc_Clock();    
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    pAig = Abc_NtkToDar(pNtk, 0, 0);
    Aig_ManFanoutStart(pAig);
    if(!smlVerifyCombGiven(pAig, simFileName, 0))
    {
        printf("Bad pNtk / sim file pair at ntk Merge\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

clk = Abc_Clock();
    // find candidate merge signals
    vector< pair<int, int> > vCandidate;
    smlSignalMergeCandidate2(pAig, simFileName, vCandidate); // candidate in topological order
    if(vCandidate.size() == 0)
    {
        printf("No signal merge candidate\n");
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }
timeCand += Abc_Clock() - clk;
    for(int i=vCandidate.size()-1; i>=0; i--)
    {
        pObj1 = Aig_ManObj(pAig, vCandidate[i].first);
        pObj2 = Aig_ManObj(pAig, vCandidate[i].second);
        if(pObj1 == NULL || pObj2 == NULL) // removed by previous merging
        {
            nSkipped++;
            continue;
        }
        if ( pObj2 == Aig_ObjFanin0(pObj1) || pObj2 == Aig_ObjFanin1(pObj1) )
        {
            nSkipped++;
            continue;
        }
        printf("pObj1 = %i, pObj2 = %i\n", Aig_ObjId(pObj1), Aig_ObjId(pObj2));
        printf("PO 134 fanin = %i, phase = %i\n", Aig_ObjFaninId0(Aig_ManCo(pAig, 134)), Aig_ObjFaninC0(Aig_ManCo(pAig, 134)) );
        Aig_ObjDisconnect(pAig, pObj2);
        pObj2->Type = AIG_OBJ_BUF;
        Aig_ObjConnect( pAig, pObj2, pObj1, NULL );
        if(!smlVerifyCombGiven(pAig, simFileName, 0)) { printf("Wrong here.\n"); }
        Aig_ManCleanup(pAig);
        printf("PO 134 fanin = %i, phase = %i\n", Aig_ObjFaninId0(Aig_ManCo(pAig, 134)), Aig_ObjFaninC0(Aig_ManCo(pAig, 134)) );
        if(!smlVerifyCombGiven(pAig, simFileName, 1))
        {
            printf("Error after merging %i\n", i);
            break;
        }
    }

    pNtkNew = Abc_NtkFromDar(pNtk, pAig);
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);
    sizeNew = Abc_NtkNodeNum(pNtkNew);
    // printf("Network size: %i / %i\n", Abc_NtkNodeNum(pNtkNew), Abc_NtkNodeNum(pNtk));

    // make sure everything is okay
//     if(!ntkVerifySamples(pNtkNew, simFileName, 0))
//     {
//         printf("The simulation check has failed.\n");
//         return NULL;
//     }
    if ( !Abc_NtkCheck( pNtkNew ) )
    {
        printf( "The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        Aig_ManFanoutStop(pAig);
        Aig_ManStop(pAig);
        return pNtk;
    }

timeTotal += Abc_Clock() - clkStart;
    // report stats
    if(fVerbose)
    {
        printf( "> Signal Merging Statistics:\n");
        printf( "  Total Mergings    = %i (%i skipped) / %lu.\n", nSuccess, nSkipped, vCandidate.size());
        printf( "  Gain              = %8d. (%6.2f %%).\n", sizeOld - sizeNew, 100.0*(sizeOld - sizeNew)/sizeOld );
        ABC_PRT( "  Candidate   ", timeCand );
        ABC_PRT( "  Verify      ", timeVerify );
        ABC_PRT( "  Total       ", timeTotal );
    }
    // clean up
    Aig_ManFanoutStop(pAig);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pNtk);
    return pNtkNew;
}

extern "C" { Vec_Ptr_t * Aig_ManDfs( Aig_Man_t * p, int fNodesOnly ); }
Abc_Ntk_t * ntkSignalMerge3(Abc_Ntk_t * pNtk, char * simFileName, int fVerbose)
{
    extern Vec_Ptr_t* smlSignalMergeCandidate3( Aig_Man_t * pAig, char * pFileName);
    
    int nSuccess = 0, nClass = 0;
    Abc_Ntk_t * pNtkNew;
    Aig_Obj_t * pTarget, * pObj;
    Vec_Ptr_t* vClassInfo;
    int sizeOld = Abc_NtkNodeNum(pNtk), sizeNew, i;
    abctime clk, clkStart, timeCand = 0, timeTotal = 0;
    assert( Abc_NtkIsLogic(pNtk) || Abc_NtkIsStrash(pNtk) );
    if(!ntkVerifySamples(pNtk, simFileName, 0))
    {
        printf("Bad pNtk / sim file pair at ntk Merge\n");
        return NULL;
    }

clkStart = Abc_Clock();    
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);
    Aig_ManFanoutStart(pAig);
    
clk = Abc_Clock();
    // find candidate merge signals
    vClassInfo = smlSignalMergeCandidate3(pAig, simFileName);
    if(Vec_PtrSize(vClassInfo) == 0)
    {
        printf("No signal merge candidate\n");
        Vec_PtrFree(vClassInfo);
        Aig_ManStop(pAig);
        return pNtk;
    }
timeCand += Abc_Clock() - clk;
    // printf("Init >> 0: %i, 1: %i, 2: %i, 3: %i, 4: %i, 5: %i, 6: %i, 7: %i\n", pAig->nObjs[0], pAig->nObjs[1], pAig->nObjs[2], pAig->nObjs[3], pAig->nObjs[4], pAig->nObjs[5], pAig->nObjs[6], pAig->nObjs[7]);
    Vec_PtrForEachEntry(Aig_Obj_t*, vClassInfo, pObj, i)
    {
        pTarget = Aig_ManObj(pAig, i);
        if(pTarget == NULL) { continue; }
        if(!Aig_ObjIsNode(pTarget)) { continue; }

        pTarget->nRefs++;
        while(pObj != NULL)
        {
            printf("Replace %i w/ %i\n", pObj->Id, pTarget->Id);
            Aig_ObjReplace(pAig, pObj, pTarget, 0);
            Aig_ManDfs(pAig, 1);
            nSuccess++;
            pObj = (Aig_Obj_t*)Vec_PtrGetEntry(vClassInfo, pObj->Id);
        }
        pTarget->nRefs--;
        nClass++;
    }
    Aig_ManCleanup(pAig);
    printf("Init >> 0: %i, 1: %i, 2: %i, 3: %i, 4: %i, 5: %i, 6: %i, 7: %i\n", pAig->nObjs[0], pAig->nObjs[1], pAig->nObjs[2], pAig->nObjs[3], pAig->nObjs[4], pAig->nObjs[5], pAig->nObjs[6], pAig->nObjs[7]);
    pNtkNew = Abc_NtkFromDar(pNtk, pAig);
    pNtkNew = Abc_NtkStrash(pNtkNew, 0, 0, 0);
    sizeNew = Abc_NtkNodeNum(pNtkNew);

    // make sure everything is okay
    if ( !Abc_NtkCheck( pNtkNew ) || !ntkVerifySamples(pNtkNew, simFileName, 0) )
    {
        printf( "The network check has failed.\n" );
        Abc_NtkDelete( pNtkNew );
        Aig_ManStop(pAig);
        return pNtk;
    }

timeTotal += Abc_Clock() - clkStart;
    // report stats
    if(fVerbose)
    {
        printf( "> Signal Merging Statistics:\n");
        printf( "  Total Mergings    = %i (%i EQ classes).\n", nSuccess, nClass);
        printf( "  Gain              = %8d. (%6.2f %%).\n", sizeOld - sizeNew, 100.0*(sizeOld - sizeNew)/sizeOld );
        ABC_PRT( "  Candidate   ", timeCand );
        ABC_PRT( "  Total       ", timeTotal );
    }
    // clean up
    Vec_PtrFree(vClassInfo);
    Aig_ManFanoutStop(pAig);
    Aig_ManStop(pAig);
    Abc_NtkDelete(pNtk);
    return pNtkNew;
}

/////////////////////////////////////////////////////////
// Rewrite
// Modified from abcRewrite.c to support 1. external careset 
/////////////////////////////////////////////////////////
/*== bool/dec/decAbc.c ==*/
extern "C" { void Dec_GraphUpdateNetwork( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int fUpdateLevel, int nGain ); }
extern "C" { int Dec_GraphToNetworkCount( Abc_Obj_t * pRoot, Dec_Graph_t * pGraph, int NodeMax, int LevelMax ); }

unsigned int cutComputeCareset(Abc_Obj_t * pObj, Cut_Cut_t * pCut, Fra_Sml_t * pSim, unsigned int * uAlter)
{    
    // only compute careset for cuts w/ nLeaves = 4
    if( pCut->nLeaves < 4 )
        return 0;

    unsigned int uCare = 0;
    int uTruth;
    Aig_Obj_t * leafNodes[4];
    int patClass[(pSim->nWordsFrame << 5)] = {0}; // class for each pattern wrt the cut 
    
    for(int n=0; n<4; n++)
        leafNodes[n] = (Aig_Obj_t*)Abc_NtkObj(pObj->pNtk, pCut->pLeaves[n])->pCopy;
    // get patClass (SDC)
    for(int k=0; k<(pSim->nWordsFrame << 5); k++)
    {
        uTruth = 0;
        for(int n=0; n<4; n++)
        {
            if(Abc_InfoHasBit(Fra_ObjSim( pSim, Aig_ObjId(leafNodes[n])), k) ^ Aig_ObjPhase(leafNodes[n])) // must consider node phase
                uTruth |= (1 << n);
        }
        patClass[k] = uTruth;
    }
// printf("Local patterns: 711 - %i; 1348 - %i; 2213 - %i\n", patClass[711], patClass[1348], patClass[2213]);
/*
    int patCount[16] = {0};
    for(int k=0; k<(pSim->nWordsFrame << 5); k++)
        patCount[patClass[k]]++;
    for(int k=0; k<16; k++)
        printf("%i ", patCount[k]);
    printf("\n");
*/      
    // check ODC (care if any pattern in such class alters the output when flipped)
    for(int k=0; k<(pSim->nWordsFrame << 5); k++)
    {
        if(Abc_InfoHasBit(uAlter, k))
            uCare |= (1<<patClass[k]);
    }
    return uCare;
}

void enumUTruth(unsigned uLocal, unsigned uCare, Vec_Int_t * pTruth)
{
    if(uCare == 0xFFFF)
    {
        Vec_IntClear(pTruth);
        Vec_IntPush(pTruth, uLocal);
        return;
    }
    
    Vec_Int_t * pRes = Vec_IntAlloc(0);
    int bit, s, e, i;

    Vec_IntPush(pRes, uLocal);
    for(bit=0; bit<16; bit++)
    {
        if( (uCare >> bit)%2 == 0)
        {
            s = Vec_IntSize(pRes);
            Vec_IntForEachEntryStop(pRes, e, i, s)
                Vec_IntPush(pRes, (unsigned)e ^ (1<<bit));
        }
    }
    Vec_IntClear(pTruth);
    Vec_IntAppend(pTruth, pRes);
    Vec_IntFree(pRes);
}

Dec_Graph_t * cutEvaluate( Rwr_Man_t * p, Abc_Obj_t * pRoot, Cut_Cut_t * pCut, Vec_Ptr_t * vFaninsCur, int nNodesSaved, int LevelMax, int * pGainBest, int fPlaceEnable, unsigned uTruth )
{
    Vec_Ptr_t * vSubgraphs;
    Dec_Graph_t * pGraphBest = NULL; // Suppress "might be used uninitialized"
    Dec_Graph_t * pGraphCur;
    Rwr_Node_t * pNode, * pFanin;
    int nNodesAdded, GainBest, i, k;
    float CostBest;//, CostCur;
    // find the matching class of subgraphs
    vSubgraphs = Vec_VecEntry( p->vClasses, p->pMap[uTruth] );
    p->nSubgraphs += vSubgraphs->nSize;
    // determine the best subgraph
    GainBest = -1;
    CostBest = ABC_INFINITY;
    Vec_PtrForEachEntry( Rwr_Node_t *, vSubgraphs, pNode, i )
    {
        // get the current graph
        pGraphCur = (Dec_Graph_t *)pNode->pNext;
        // copy the leaves
        Vec_PtrForEachEntry( Rwr_Node_t *, vFaninsCur, pFanin, k )
            Dec_GraphNode(pGraphCur, k)->pFunc = pFanin;
        // detect how many unlabeled nodes will be reused
        nNodesAdded = Dec_GraphToNetworkCount( pRoot, pGraphCur, nNodesSaved, LevelMax );
        if ( nNodesAdded == -1 )
            continue;
        assert( nNodesSaved >= nNodesAdded );
/*
        // evaluate the cut
        if ( fPlaceEnable )
        {
            extern float Abc_PlaceEvaluateCut( Abc_Obj_t * pRoot, Vec_Ptr_t * vFanins );

            float Alpha = 0.5; // ???
            float PlaceCost;

            // get the placement cost of the cut
            PlaceCost = Abc_PlaceEvaluateCut( pRoot, vFaninsCur );

            // get the weigted cost of the cut
            CostCur = nNodesSaved - nNodesAdded + Alpha * PlaceCost;

            // do not allow uphill moves
            if ( nNodesSaved - nNodesAdded < 0 )
                continue;

            // decide what cut to use
            if ( CostBest > CostCur )
            {
                GainBest   = nNodesSaved - nNodesAdded; // pure node cost
                CostBest   = CostCur;                   // cost with placement
                pGraphBest = pGraphCur;                 // subgraph to be used for rewriting

                // score the graph
                if ( nNodesSaved - nNodesAdded > 0 )
                {
                    pNode->nScore++;
                    pNode->nGain += GainBest;
                    pNode->nAdded += nNodesAdded;
                }
            }
        }
        else
*/
        {
            // count the gain at this node
            if ( GainBest < nNodesSaved - nNodesAdded )
            {
                GainBest   = nNodesSaved - nNodesAdded;
                pGraphBest = pGraphCur;

                // score the graph
                if ( nNodesSaved - nNodesAdded > 0 )
                {
                    pNode->nScore++;
                    pNode->nGain += GainBest;
                    pNode->nAdded += nNodesAdded;
                }
            }
        }
    }
    if ( GainBest == -1 )
        return NULL;
    *pGainBest = GainBest;
    return pGraphBest;
}

int rwrNodeRewrite( Rwr_Man_t * p, Cut_Man_t * pManCut, Abc_Obj_t * pNode, int fUpdateLevel, int fUseZeros, int fPlaceEnable, Fra_Sml_t * pSim, abctime &timeDCSim )
{
    // icompactGencareset.cpp
    extern void simFlipOneNode( unsigned int * uAlter, Fra_Sml_t * p, Aig_Obj_t * pObj);

// printf("Start rewriting node %i\n", Abc_ObjId(pNode));
    
    int fVeryVerbose = 0;
    Dec_Graph_t * pGraph;
    Cut_Cut_t * pCut;//, * pTemp;
    Abc_Obj_t * pFanin;
    unsigned uPhase;
    unsigned uTruthBest = 0; // Suppress "might be used uninitialized"
    unsigned uLocal, uCare, uTruth;
    bool fuAlterComputed = false;
    unsigned int * uAlter = new unsigned int[pSim->nWordsTotal]();
    Vec_Int_t * pTruth = Vec_IntStart(0), * pClass = Vec_IntStart(0);
    char * pPerm;
    int Required, nNodesSaved;
    int nNodesSaveCur = -1; // Suppress "might be used uninitialized"
    int i, GainCur = -1, GainBest = -1;
    abctime clk, clk2;//, Counter;
    bool fCutEval; // for stats recording

    p->nNodesConsidered++;
    // get the required times
    Required = fUpdateLevel? Abc_ObjRequiredLevel(pNode) : ABC_INFINITY;

    // get the node's cuts
clk = Abc_Clock();
    pCut = (Cut_Cut_t *)Abc_NodeGetCutsRecursive( pManCut, pNode, 0, 0);
    assert( pCut != NULL );
p->timeCut += Abc_Clock() - clk;

//printf( " %d", Rwr_CutCountNumNodes(pNode, pCut) );
/*
    Counter = 0;
    for ( pTemp = pCut->pNext; pTemp; pTemp = pTemp->pNext )
        Counter++;
    printf( "%d ", Counter );
*/
    // go through the cuts
clk = Abc_Clock();
    for ( pCut = pCut->pNext; pCut; pCut = pCut->pNext )
    {
        // consider only 4-input cuts
        if ( pCut->nLeaves < 4 )
            continue;

        fCutEval = false;
        // get the fanin permutation, modified to support external care set enum
        uLocal = 0xFFFF & *Cut_CutReadTruth(pCut);
clk2 = Abc_Clock();
        if (!fuAlterComputed)
        {
            simFlipOneNode(uAlter, pSim, (Aig_Obj_t *)pNode->pCopy); // compute uAlter once for the same root
            fuAlterComputed = true;
        } 
        uCare = cutComputeCareset(pNode, pCut, pSim, uAlter);
        enumUTruth(uLocal, uCare, pTruth);
timeDCSim += Abc_Clock() - clk2;
        Vec_IntForEachEntry(pTruth, uTruth, i)
        {
            if(Vec_IntFind(pClass, p->pMap[uTruth]) != -1)
                continue;
            Vec_IntPush(pClass, p->pMap[uTruth]);
            
            pPerm = p->pPerms4[ (int)p->pPerms[uTruth] ];
            uPhase = p->pPhases[uTruth];
            // collect fanins with the corresponding permutation/phase
            Vec_PtrClear( p->vFaninsCur );
            Vec_PtrFill( p->vFaninsCur, (int)pCut->nLeaves, 0 );
            for ( i = 0; i < (int)pCut->nLeaves; i++ )
            {
                pFanin = Abc_NtkObj( pNode->pNtk, pCut->pLeaves[(int)pPerm[i]] );
                if ( pFanin == NULL )
                    break;
                pFanin = Abc_ObjNotCond(pFanin, ((uPhase & (1<<i)) > 0) );
                Vec_PtrWriteEntry( p->vFaninsCur, i, pFanin );
            }
            if ( i != (int)pCut->nLeaves )
            {
                p->nCutsBad++;
                continue;
            }
            if(!fCutEval)
            {
                p->nCutsGood++;
                fCutEval = true;
            }    

            {
                int Counter = 0;
                Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                    if ( Abc_ObjFanoutNum(Abc_ObjRegular(pFanin)) == 1 )
                        Counter++;
                if ( Counter > 2 )
                    continue;
            }
clk2 = Abc_Clock();

            // mark the fanin boundary 
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                Abc_ObjRegular(pFanin)->vFanouts.nSize++;

            // label MFFC with current ID
            Abc_NtkIncrementTravId( pNode->pNtk );
            nNodesSaved = Abc_NodeMffcLabelAig( pNode );
            // unmark the fanin boundary
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                Abc_ObjRegular(pFanin)->vFanouts.nSize--;
p->timeMffc += Abc_Clock() - clk2;

            // evaluate the cut
clk2 = Abc_Clock();
            pGraph = cutEvaluate( p, pNode, pCut, p->vFaninsCur, nNodesSaved, Required, &GainCur, fPlaceEnable, uTruth );                                       
p->timeEval += Abc_Clock() - clk2;

            // check if the cut is better than the current best one
            if ( pGraph != NULL && GainBest < GainCur )
            {
                // save this form
                nNodesSaveCur = nNodesSaved;
                GainBest  = GainCur;
                p->pGraph  = pGraph;
                p->fCompl = ((uPhase & (1<<4)) > 0);
                uTruthBest = uTruth;
// printf("    uTruth : "); printBits(sizeof(uTruth), &uTruth);
                // collect fanins in the
                Vec_PtrClear( p->vFanins );
                Vec_PtrForEachEntry( Abc_Obj_t *, p->vFaninsCur, pFanin, i )
                    Vec_PtrPush( p->vFanins, pFanin );
            }
        }   
    }
p->timeRes += Abc_Clock() - clk;

    if ( GainBest == -1 )
        return -1;
/*
    if ( GainBest > 0 )
    {
        printf( "Class %d  ", p->pMap[uTruthBest] );
        printf( "Gain = %d. Node %d : ", GainBest, pNode->Id );
        Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanins, pFanin, i )
            printf( "%d ", Abc_ObjRegular(pFanin)->Id );
        Dec_GraphPrint( stdout, p->pGraph, NULL, NULL );
        printf( "\n" );
    }
*/

//    printf( "%d", nNodesSaveCur - GainBest );
/*
    if ( GainBest > 0 )
    {
        if ( Rwr_CutIsBoolean( pNode, p->vFanins ) )
            printf( "b" );
        else
        {
            printf( "Node %d : ", pNode->Id );
            Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanins, pFanin, i )
                printf( "%d ", Abc_ObjRegular(pFanin)->Id );
            printf( "a" );
        }
    }
*/
/*
    if ( GainBest > 0 )
        if ( p->fCompl )
            printf( "c" );
        else
            printf( "." );
*/

    // copy the leaves
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanins, pFanin, i )
        Dec_GraphNode((Dec_Graph_t *)p->pGraph, i)->pFunc = pFanin;
/*
    printf( "(" );
    Vec_PtrForEachEntry( Abc_Obj_t *, p->vFanins, pFanin, i )
        printf( " %d", Abc_ObjRegular(pFanin)->vFanouts.nSize - 1 );
    printf( " )  " );
*/
//    printf( "%d ", Rwr_NodeGetDepth_rec( pNode, p->vFanins ) );

    p->nScores[p->pMap[uTruthBest]]++;
    p->nNodesGained += GainBest;
    if ( fUseZeros || GainBest > 0 )
    {
        p->nNodesRewritten++;
    }

    // report the progress
    if ( fVeryVerbose && GainBest > 0 )
    {
        printf( "Node %6s :   ", Abc_ObjName(pNode) );
        printf( "Fanins = %d. ", p->vFanins->nSize );
        printf( "Save = %d.  ", nNodesSaveCur );
        printf( "Add = %d.  ",  nNodesSaveCur-GainBest );
        printf( "GAIN = %d.  ", GainBest );
        printf( "Cone = %d.  ", p->pGraph? Dec_GraphNodeNum((Dec_Graph_t *)p->pGraph) : 0 );
        printf( "Class = %d.  ", p->pMap[uTruthBest] );
        printf( "\n" );
    }
    Vec_IntFree(pTruth);
    Vec_IntFree(pClass);
    delete [] uAlter;
    return GainBest;
}

Cut_Man_t * Abc_NtkStartCutManForRewrite( Abc_Ntk_t * pNtk )
{
    static Cut_Params_t Params, * pParams = &Params;
    Cut_Man_t * pManCut;
    Abc_Obj_t * pObj;
    int i;
    // start the cut manager
    memset( pParams, 0, sizeof(Cut_Params_t) );
    pParams->nVarsMax  = 4;     // the max cut size ("k" of the k-feasible cuts)
    pParams->nKeepMax  = 250;   // the max number of cuts kept at a node
    pParams->fTruth    = 1;     // compute truth tables
    pParams->fFilter   = 1;     // filter dominated cuts
    pParams->fSeq      = 0;     // compute sequential cuts
    pParams->fDrop     = 0;     // drop cuts on the fly
    pParams->fVerbose  = 0;     // the verbosiness flag
    pParams->nIdsMax   = Abc_NtkObjNumMax( pNtk );
    pManCut = Cut_ManStart( pParams );
    if ( pParams->fDrop )
        Cut_ManSetFanoutCounts( pManCut, Abc_NtkFanoutCounts(pNtk) );
    // set cuts for PIs
    Abc_NtkForEachCi( pNtk, pObj, i )
        if ( Abc_ObjFanoutNum(pObj) > 0 )
            Cut_NodeSetTriv( pManCut, pObj->Id );
    return pManCut;
}

int ntkRewrite( Abc_Ntk_t * pNtk, int fUpdateLevel, int fUseZeros, int fVerbose, int fVeryVerbose, int fPlaceEnable, char * simFileName )
{
    ProgressBar * pProgress;
    Cut_Man_t * pManCut;
    Rwr_Man_t * pManRwr;
    Abc_Obj_t * pNode;
//    Vec_Ptr_t * vAddedCells = NULL, * vUpdatedNets = NULL;
    Dec_Graph_t * pGraph;
    int i, nNodes, nGain, fCompl;
    abctime clk, clk2, clkStart = Abc_Clock();
    Fra_Sml_t * pSim; // sim mgr for careset computation
    Aig_Man_t * pAig;
    abctime timeDCSim = 0, timeDCUpdate = 0; // extra stats

    assert( Abc_NtkIsStrash(pNtk) );
    // cleanup the AIG
    Abc_AigCleanup((Abc_Aig_t *)pNtk->pManFunc);

    pAig = Abc_NtkToDar(pNtk, 0, 0);
    if(!smlVerifyCombGiven(pAig, simFileName, 0))
    {
        printf("Bad pNtk / sim file pair at ntk Rewrite\n");
        Aig_ManStop(pAig);
        return 0;
    }

    // start placement package
//    if ( fPlaceEnable )
//    {
//        Abc_PlaceBegin( pNtk );
//        vAddedCells = Abc_AigUpdateStart( pNtk->pManFunc, &vUpdatedNets );
//    }

    // start the rewriting manager
    pManRwr = Rwr_ManStart( 0 );
    if ( pManRwr == NULL )
        return 0;
    // compute the reverse levels if level update is requested
    if ( fUpdateLevel )
        Abc_NtkStartReverseLevels( pNtk, 0 );
    // start the cut manager
clk = Abc_Clock();
    pManCut = Abc_NtkStartCutManForRewrite( pNtk );
Rwr_ManAddTimeCuts( pManRwr, Abc_Clock() - clk );
    pNtk->pManCut = pManCut;

    if ( fVeryVerbose )
        Rwr_ScoresClean( pManRwr );

    // resynthesize each node once
    pManRwr->nNodesBeg = Abc_NtkNodeNum(pNtk);
    nNodes = Abc_NtkObjNumMax(pNtk);
    pProgress = Extra_ProgressBarStart( stdout, nNodes );
    // start the simulation mgr for careset computation
    pSim = smlSimulateStart(pAig, simFileName); 
    Abc_NtkForEachNode( pNtk, pNode, i )
    {
        Extra_ProgressBarUpdate( pProgress, i, NULL );
        // stop if all nodes have been tried once
        if ( i >= nNodes )
            break;
        // skip persistant nodes
        if ( Abc_NodeIsPersistant(pNode) )
            continue;
        // skip the nodes with many fanouts
        if ( Abc_ObjFanoutNum(pNode) > 1000 )
            continue;

        // for each cut, try to resynthesize it
        nGain = rwrNodeRewrite( pManRwr, pManCut, pNode, fUpdateLevel, fUseZeros, fPlaceEnable, pSim, timeDCSim );

        if ( !(nGain > 0 || (nGain == 0 && fUseZeros)) )
            continue;
        // if we end up here, a rewriting step is accepted

        // get hold of the new subgraph to be added to the AIG
        pGraph = (Dec_Graph_t *)Rwr_ManReadDecs(pManRwr);
        fCompl = Rwr_ManReadCompl(pManRwr);

        // reset the array of the changed nodes
//         if ( fPlaceEnable )
//             Abc_AigUpdateReset( (Abc_Aig_t *)pNtk->pManFunc );

        // complement the FF if needed
        if ( fCompl ) Dec_GraphComplement( pGraph );
clk = Abc_Clock();
        Dec_GraphUpdateNetwork( pNode, pGraph, fUpdateLevel, nGain );
        if ( fCompl ) Dec_GraphComplement( pGraph );

        // use the array of changed nodes to update placement
//        if ( fPlaceEnable )
//            Abc_PlaceUpdate( vAddedCells, vUpdatedNets );

        // sanity check
//         if(!ntkVerifySamples(pNtk, simFileName, 0))
//         {
//             printf("Verifying has failed.\n");
//             return 0;
//         }  
        // update simulation mgr
clk2 = Abc_Clock();
        smlSimulateStop(pSim);
        Aig_ManStop(pAig);
        pAig = Abc_NtkToDar(pNtk, 0, 0);
        pSim = smlSimulateStart(pAig, simFileName);
timeDCUpdate += Abc_Clock() - clk2;
Rwr_ManAddTimeUpdate( pManRwr, Abc_Clock() - clk );
    }
    Extra_ProgressBarStop( pProgress );
Rwr_ManAddTimeTotal( pManRwr, Abc_Clock() - clkStart );
    // print stats
    pManRwr->nNodesEnd = Abc_NtkNodeNum(pNtk);
    if ( fVerbose )
    {
        int nClass, nClassCounter = 0;
        for (nClass = 0; nClass < 222; nClass++ )
            nClassCounter += (pManRwr->nScores[nClass] > 0);

        printf(  "> Rewriting statistics:\n" );
        printf(  "  Total cuts tries  = %8d.\n", pManRwr->nCutsGood );
        printf(  "  Bad cuts found    = %8d.\n", pManRwr->nCutsBad );
        printf(  "  Total subgraphs   = %8d.\n", pManRwr->nSubgraphs );
        printf(  "  Used NPN classes  = %8d.\n", nClassCounter );
        printf(  "  Nodes considered  = %8d.\n", pManRwr->nNodesConsidered );
        printf(  "  Nodes rewritten   = %8d.\n", pManRwr->nNodesRewritten );
        printf(  "  Gain              = %8d. (%6.2f %%).\n", pManRwr->nNodesBeg - pManRwr->nNodesEnd, 100.0*(pManRwr->nNodesBeg - pManRwr->nNodesEnd)/pManRwr->nNodesBeg );
        ABC_PRT( "  Start         ", pManRwr->timeStart );
        ABC_PRT( "  Cuts          ", pManRwr->timeCut );
        ABC_PRT( "  Resynthesis   ", pManRwr->timeRes );
        ABC_PRT( "      Sim DC    ", timeDCSim );
        ABC_PRT( "      Mffc      ", pManRwr->timeMffc );
        ABC_PRT( "      Eval      ", pManRwr->timeEval );
        ABC_PRT( "  Update        ", pManRwr->timeUpdate );
        ABC_PRT( "      Update DC ", pManRwr->timeUpdate );
        ABC_PRT( "  Total         ", pManRwr->timeTotal );
    }
        
    if ( fVeryVerbose )
        Rwr_ScoresReport( pManRwr );
    // delete the managers
    Rwr_ManStop( pManRwr );
    Cut_ManStop( pManCut );
    pNtk->pManCut = NULL;

    // start placement package
//    if ( fPlaceEnable )
//    {
//        Abc_PlaceEnd( pNtk );
//        Abc_AigUpdateStop( pNtk->pManFunc );
//    }

    // put the nodes into the DFS order and reassign their IDs
    {
//        abctime clk = Abc_Clock();
    Abc_NtkReassignIds( pNtk );
//        ABC_PRT( "time", Abc_Clock() - clk );
    }
//    Abc_AigCheckFaninOrder( pNtk->pManFunc );
    // fix the levels
    if ( fUpdateLevel )
        Abc_NtkStopReverseLevels( pNtk );
    else
        Abc_NtkLevel( pNtk );
    // check
    if ( !Abc_NtkCheck( pNtk ) )
    {
        printf( "Abc_NtkRewrite: The network check has failed.\n" );
        return 0;
    }
    return 1;
}

ABC_NAMESPACE_IMPL_END