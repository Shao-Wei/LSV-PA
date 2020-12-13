#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "reconvergentAIG.h"

ABC_NAMESPACE_IMPL_START

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"reconvergentAIGCommand.cc\".\n" );
    return NULL;
}

static int reconvergentAIG_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;

    int fnNode = 1000;
    int fnPi = 100;
    int fMax = 100;
    char* fLog;
    int max;
    ReconvergentAIGMgr* mgr = new ReconvergentAIGMgr(pAbc);
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "vhnilm" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'n':
                fnNode = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'i':
                fnPi = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'm':
                fMax = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'l':
                fLog = argv[globalUtilOptind];
                globalUtilOptind++;
                break;
            case 'v':
                fVerbose ^= 1;
                break;
            case 'h':
                goto usage;
            default:
                goto usage;
        }
    }

    
    mgr->generateAND_1_aug(fnNode, fnPi);
    max = mgr->minimizeCurve(fMax, fLog);
    printf("( %i, %i, %i)\n", max, mgr->getNewAND(), mgr->getNewLev());

    // mgr->generateAND_1_aug(4000000, 100);
    // max = mgr->minimizeCurve(50, fLog);
    // printf("( %i, %i, %i)\n", max, mgr->getNewAND(), mgr->getNewLev());
      
    
    


    return result;
    
usage:
    Abc_Print( -2, "usage: reconvergentAIG [options]\n" );
    Abc_Print( -2, "\t              Examination on minimization of reconvergent AIGs\n" );
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{ 
    Cmd_CommandAdd( pAbc, "AlCom", "reconvergentAIG", reconvergentAIG_Command, 1);
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t reconvergentAIG_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct reconvergentAIG_registrar
{
    reconvergentAIG_registrar() 
    {
        Abc_FrameAddInitializer(&reconvergentAIG_frame_initializer);
    }
} reconvergentAIG_registrar_;

ABC_NAMESPACE_IMPL_END