#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include <vector>
#include <algorithm>

ABC_NAMESPACE_IMPL_START

using namespace std;

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"icompactCommand.cc\".\n" );
    return NULL;
}

static int sopunate_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;
    
    Abc_Ntk_t *pNtk;
    Abc_Obj_t *pObj, *pFanin;
    int i, j, k;
    char *buff, *t;

    vector<int> vecInput, vecPcount, vecNcount, vecPos, vecNeg, vecBi;
    bool fOutput;
 
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vh" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    pNtk = Abc_FrameReadNtk(pAbc);
    if(!Abc_NtkIsSopLogic(pNtk))
    {
        printf("Error: input network is not in sop format.\n");
        return 1;
    }

    Abc_NtkForEachNode(pNtk, pObj, i)
    {
        vecInput.clear();
        vecPcount.clear();
        vecNcount.clear();
        vecPos.clear();
        vecNeg.clear();
        vecBi.clear();

        Abc_ObjForEachFanin(pObj, pFanin, j)
        {
            vecInput.push_back(Abc_ObjId(pFanin));
            vecPcount.push_back(0);
            vecNcount.push_back(0);
        }

        if(vecInput.size() == 0)
            continue;

        buff = (char*)pObj->pData;
        t = strtok(buff, "\n");
        fOutput = (t[vecInput.size()+1] == '1');
        while(true)
        {
            for(k=0; k<vecInput.size(); k++)
            {
                if(t[k] == '1')
                    vecPcount[k]++;
                if(t[k] == '0')
                    vecNcount[k]++;
            }
            t = strtok(NULL, "\n");
            if(t == NULL)
                break;
        }

        for(k=0; k<vecInput.size(); k++)
        {
            if(fOutput && (vecNcount[k] == 0))
                vecPos.push_back(vecInput[k]);
            if(!fOutput && (vecPcount[k] == 0))
                vecPos.push_back(vecInput[k]);
            if(fOutput && (vecPcount[k] == 0))
                vecNeg.push_back(vecInput[k]);
            if(!fOutput && (vecNcount[k] == 0))
                vecNeg.push_back(vecInput[k]);
            if((vecNcount[k]!=0) && (vecPcount[k]!=0))
                vecBi.push_back(vecInput[k]);            
        }

        printf("node %s:\n", Abc_ObjName(pObj));
        if(vecPos.size())
        {
            sort(vecPos.begin(), vecPos.end());
            printf("+unate inputs: ");
            for(k=0; k<vecPos.size(); k++)
            {
                if(k!=0)
                    printf(",");
                printf("%s", Abc_ObjName(Abc_NtkObj(pNtk, vecPos[k])));
            }
            printf("\n");
        }
        if(vecNeg.size())
        {
            sort(vecNeg.begin(), vecNeg.end());
            printf("-unate inputs: ");
            for(k=0; k<vecNeg.size(); k++)
            {
                if(k!=0)
                    printf(",");
                printf("%s", Abc_ObjName(Abc_NtkObj(pNtk, vecNeg[k])));
            }
            printf("\n");
        }
        if(vecBi.size())
        {
            sort(vecBi.begin(), vecBi.end());
            printf("binate inputs: ");
            for(k=0; k<vecBi.size(); k++)
            {
                if(k!=0)
                    printf(",");
                printf("%s", Abc_ObjName(Abc_NtkObj(pNtk, vecBi[k])));
            }
            printf("\n");
        }
    }
    return result;
    
usage:
    Abc_Print( -2, "usage: lsv_print_sopunate [options]\n" );
    Abc_Print( -2, "\t              prints the unate information for each node\n" );
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;   
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{ 
    Cmd_CommandAdd( pAbc, "AlCom", "lsv_print_sopunate", sopunate_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t sopunate_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct sopunate_registrar
{
    sopunate_registrar() 
    {
        Abc_FrameAddInitializer(&sopunate_frame_initializer);
    }
} sopunate_registrar_;

ABC_NAMESPACE_IMPL_END