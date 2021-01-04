#include "icompact.h"

ABC_NAMESPACE_IMPL_START

// returns the number of input variables used in the cubes of the pla
int espresso_input_count(char* filename)
{
    char buff[102400];
    char* t;

    int nPi;
    bool* lit;
    int count;

    FILE* f = fopen(filename, "r");
    fgets(buff, 102400, f); // skip
    t = strtok(buff, " \n"); // skip
    t = strtok(NULL, " \n");
    nPi = atoi(t);
    fgets(buff, 102400, f); // skip
    fgets(buff, 102400, f); // skip
    
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
    char* t;
    FILE* fPla = fopen(pFileName, "r");

    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPi) { fclose(fPla); return 1; }
    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPo) { fclose(fPla); return 1; }
    
    fclose(fPla);
    return 0;
}

void aux_orderPiPo(Abc_Ntk_t * pNtk, int nPi, int nPo)
{
    Vec_Ptr_t * vPis, * vPos;
    Abc_Obj_t * pObj;
    int i;
    char s[1024];

    // temporarily store the names in the copy field
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = (Abc_Obj_t *)Abc_ObjName(pObj);

    vPis = Vec_PtrAlloc(0);
    vPos = Vec_PtrAlloc(0);
    for (i=0; i<nPi; i++)
    {
        sprintf(s, "i%i", i);
        pObj = Abc_NtkFindCi( pNtk, s );
        if(pObj == NULL)
        {
            pObj = Abc_NtkCreatePi(pNtk);
            Abc_ObjAssignName(pObj, s, NULL);
        }
        Vec_PtrPush( vPis, pObj );
    }
    for (i=0; i<nPo; i++)
    {
        sprintf(s, "o%i", i);
        pObj = Abc_NtkFindCo( pNtk, s );
        Vec_PtrPush( vPos, pObj );
    }
    pNtk->vPis = vPis;
    pNtk->vPos = vPos;

    Abc_NtkOrderCisCos( pNtk );
    // clean the copy fields
    Abc_NtkForEachPi( pNtk, pObj, i )
        pObj->pCopy = NULL;
    Abc_NtkForEachPo( pNtk, pObj, i )
        pObj->pCopy = NULL;
}


ABC_NAMESPACE_IMPL_END