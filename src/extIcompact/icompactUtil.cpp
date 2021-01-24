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
    if(!fVerbose)
        result = smlVerifyCombGiven(pNtk, pFile, NULL);
    else
    {
        int * pCount = new int[nPo_file]();
        result = smlVerifyCombGiven(pNtk, pFile, pCount);
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

ABC_NAMESPACE_IMPL_END