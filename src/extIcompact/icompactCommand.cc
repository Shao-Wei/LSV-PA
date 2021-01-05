#include "icompact.h"
#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include "base/io/ioAbc.h"
#include <vector>

ABC_NAMESPACE_IMPL_START

static Abc_Ntk_t * DummyFunction( Abc_Ntk_t * pNtk )
{
    Abc_Print( -1, "Please rewrite DummyFunction() in file \"icompactCommand.cc\".\n" );
    return NULL;
}

static int gencareset_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;
    int fCube        = 0;
    int fNcube       = 0;
    int fNpattern    = 0;

    Abc_Ntk_t* pNtk;
    char *blifFileName, *caresetFileName, *samplesFileName;
    int nPi, nPo;
    
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "cnvh" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'c':
                fCube ^= 1;
                fNcube = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'n':
                fNpattern = atoi(argv[globalUtilOptind]);
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
    if ( argc != globalUtilOptind + 3 )
    {
        printf("Missing arguements..\n");
        goto usage;
    }
    blifFileName = argv[globalUtilOptind];
    globalUtilOptind++;
    caresetFileName = argv[globalUtilOptind];
    globalUtilOptind++;
    samplesFileName = argv[globalUtilOptind];
    globalUtilOptind++;

    printf("[Info] Start generated careset\n");
    // get nPi
    pNtk = Io_ReadBlif(blifFileName, 1);
    pNtk = Abc_NtkToLogic(pNtk);
    pNtk = Abc_NtkStrash(pNtk, 0, 0, 0);
    nPi = Abc_NtkPiNum(pNtk);
    nPo = Abc_NtkPoNum(pNtk);
    // BUG - reg count non-zero while no latch exists - fails assertion in Fra_SmlSimulateCombGiven()
    // printf("reg = %i\n", Aig_ManRegNum((Aig_Man_t*)pNtk->pManFunc));

    // gen careset
    printf("Generate careset file\n");
    if(!fCube)
        n_gen_Random(fNpattern, nPi, nPo, caresetFileName);
    else
        n_gen_Cube(fNpattern, fNcube, nPi, nPo, caresetFileName);

    // gen sampling
    printf("Generate sampling file\n");
    if(careset2patterns(samplesFileName, caresetFileName, nPi, nPo))
    {
        fprintf( stdout, "Careset incorrect or too large for pattern simulation. Aborted\n");
        return 1;
    }
    if (smlSimulateCombGiven( pNtk, samplesFileName))
    {
        fprintf( stdout, "failed at simulation\n");
        return 1;
    }

    return result;
    
usage:
    Abc_Print( -2, "usage: gencareset [options] <benchmark.blif> <careset.pla> <samples_header>\n" );
    Abc_Print( -2, "\t               generate random careset in pla format type fr\n" );
    Abc_Print( -2, "\t-c cube_num    : set number of cubes [default = %d]\n", fNcube );
    Abc_Print( -2, "\t-n minterm_num : set number of minterms to generate [default = %d]\n", fNpattern );    
    Abc_Print( -2, "\t-v             : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h             : print the command usage\n" );
    return 1;   
}

static int compact_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    /*
    Print out full mode info for clear doc.
    Test batch functionality.
    Report exact minterm count. (thru input reencode unique pat | BDD)
    */
    // == Options ================
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;

    enum SolvingType fSolving = LEXSAT_ENCODE_C;
    double fRatio            = 0;
    int fNewVar              = 4;
    int fCollapse            = 0;
    enum MinimizeType fMinimize  = BASIC;
    int fOutput              = 0;
    int fLog                 = 0;
    int fBatch               = 1;
    int fSupport             = 0;
    int fEval                = 0;

    // == Overall declaration ================
    char *caresetFileName   = NULL;
    char *baseFileName      = NULL;
    char *funcFileName      = NULL;
    char *resultlogFileName = NULL;
    
    IcompactMgr *mgr;
    // == Parse command ======================
    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "eosmlnycrpvh" ) ) != EOF )
    {
        switch ( c )
        {            
            case 'e': // for experiment only
                fEval ^= 1;
                funcFileName = argv[globalUtilOptind];
                globalUtilOptind++;
                break;
            case 'o':
                fOutput = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 's':
                fSolving = (SolvingType)atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'r':
                fRatio = atof(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'n':
                fNewVar = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'm':
                fMinimize = (MinimizeType)atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'b':
                fBatch = atoi(argv[globalUtilOptind]);
                globalUtilOptind++;
                break;
            case 'c':
                fCollapse ^= 1;
                break;
            case 'l':
                fLog ^= 1;
                resultlogFileName = argv[globalUtilOptind];
                globalUtilOptind++;
                break;
            case 'p':
                fSupport ^= 1;
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
    if ( argc != globalUtilOptind + 2 )
    {
        printf("Missing arguements.. Please specify careset file as well as base name for intermediate files.\n");
        goto usage;
    }
    caresetFileName = argv[globalUtilOptind];
    globalUtilOptind++;
    baseFileName = argv[globalUtilOptind];
    globalUtilOptind++;

    /////////////////////////////////////////////////////////
    // Start Main
    /////////////////////////////////////////////////////////
    printf("[Info] IcompactMgr start\n");
    mgr = new IcompactMgr(pAbc, caresetFileName, baseFileName, funcFileName, resultlogFileName);
    mgr->ocompact(fOutput, fNewVar);
    mgr->icompact(fSolving, fRatio, fNewVar, fCollapse, fMinimize, fBatch);

    /////////////////////////////////////////////////////////
    // Print Evaluation
    /////////////////////////////////////////////////////////
    printf("[Info]Print output files & evaluation\n");

    ////////////////////////////////////////////////////////
    return result;
    
usage:
    Abc_Print( -2, "Input / Output Compaction on High Dimensional Boolean Space\n" );
    Abc_Print( -2, "Author(s): Shao-Wei Chu [email: r08943098@ntu.edu.tw]\n" );
    Abc_Print( -2, "\n" );
    Abc_Print( -2, "usage: icompact_cube [options] <careset.pla> <base>\n" );
    Abc_Print( -2, "\t                         compact input variables based on specified careset\n" );
    Abc_Print( -2, "\t<careset.pla>          : careset specified in pla\n" );
    Abc_Print( -2, "\t<base>                 : neccessary intermidiate files are dumped here\n" );
    Abc_Print( -2, "\t-e <file.blif>         : original function, used for evaluation\n" );
    Abc_Print( -2, "\t-l <result.log>        : interal signal / statistics log\n");
    Abc_Print( -2, "\t-o                     : output compaction type: 0(none), 1(naive), 2(select) [default = %d]\n", fOutput);
    Abc_Print( -2, "\t-s <num>               : input compaction type: 0(HEURISTIC_SINGLE), 1(HEURISTIC_8), 2(LEXSAT_CLASSIC), 3(LEXSAT_ENCODE_C), 4(REENCODE) [default = %d]\n", fSolving);
    Abc_Print( -2, "\t-r <num>               : set ratio for input compaction lock entry [default = %f]\n", fRatio);
    Abc_Print( -2, "\t-m <num>               : minimize type:  0(NOMIN), 1(BASIC), 2(STRONG) [default = %d]\n", fMinimize);
    Abc_Print( -2, "\t-n <num>               : set number of newly introduced variable for reencoding - effects solving type REENCODE & output compaction type select [default = %d]\n", fNewVar);
    Abc_Print( -2, "\t-b <num>               : set batch size (have effects only when solving type being set to LEXSAT_CLASSIC) [default = %d]\n", fBatch);
    Abc_Print( -2, "\t-c                     : toggle using collapse during minimization [default = %d]\n", fCollapse );
    Abc_Print( -2, "\t-v                     : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-p                     : print selected support info(for debug only)\n");
    Abc_Print( -2, "\t-h                     : print the command usage\n" );
    return 1;   
}

static int checkesp_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;

    abctime step_time, end_time;
    char* fileName;
    char *resultName;
    char * extension = ".esp.pla";
    int gate_count, input_count;
    double runtime;
    int unused_i __attribute__((unused)); // get rid of system warnings

    char Command[1000];
    // char strongCommand[1000]   = "if -K 6 -m; mfs2 -W 100 -F 100 -D 100 -L 100 -C 1000000 -e";
    // char collapseCommand[1000] = "collapse";
    char minimizeCommand[1000] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";

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

    if ( argc != globalUtilOptind + 1) 
        goto usage;
    // get the input file name
    fileName = argv[globalUtilOptind];

    resultName = ABC_ALLOC(char, strlen(fileName)+1+8);
    strcpy(resultName, fileName);
    strcat(resultName, extension);

    step_time = Abc_Clock();
    sprintf( Command, "./espresso %s", fileName);
    unused_i = system( Command );
    sprintf( Command, "read %s", resultName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", resultName);
        return 1;
    }
    if(Cmd_CommandExecute(pAbc,minimizeCommand))
    {
        printf("min %s. Failed.\n", resultName);
        return 1;
    }
    end_time = Abc_Clock();
    input_count = espresso_input_count(resultName); 
    gate_count = Abc_NtkNodeNum(Abc_FrameReadNtk(pAbc));
    runtime = 1.0*((double)(end_time - step_time))/((double)CLOCKS_PER_SEC);

    ////////////////////////////////////////////////////////
    // filename, input_count, runtime, gate_count
    printf("%s, %i, %.2f, %i\n", fileName, input_count, runtime, gate_count);
    ////////////////////////////////////////////////////////
    return result;
    
usage:
    Abc_Print( -2, "usage: checkesp sample.pla\n" );
    Abc_Print( -2, "\t              perform espresso and print circuit size and runtime\n" );
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;   
}

static int readplabatch_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    // == Overall declaration ================
    int result = 0;
    char Command[2000];
    char minimizeCommand[1000] = "if -K 6 -m; mfs2 -W 100 -F 100 -D 100 -L 100 -C 1000000 -e; \
                                  strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";
    char buff[102400];
    char * unused_c __attribute__((unused)); // get rid of fget warnings
    int    unused_i __attribute__((unused)); // get rid of system warnings

    char* funcFileName;
    char espFileName[1000];
    char espResultFileName[1000];
    char resultBlifFileName[1000];
    FILE* fRead, *fWrite;
    int batchvar;
    int batchnum;
    Abc_Ntk_t** allNtks;
    
    // == Parse command ======================
    if ( argc != globalUtilOptind + 2 )
    {
        Abc_Print( -2, "usage: readplabatch <n> <file.pla>\n" );
        Abc_Print( -2, "\t              read in pla as batches\n" );
        Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", 0);
        Abc_Print( -2, "\t-h            : print the command usage\n" );
        return 1;   
    }
    batchvar = atoi(argv[globalUtilOptind]);
    globalUtilOptind++;
    funcFileName = argv[globalUtilOptind];

    // =======================================
    printf("[Info] start read pla in batch\n");

    sprintf(espFileName, "%s.tmp.pla", funcFileName);
    sprintf(espResultFileName, "%s.tmp.pla.esp.pla", funcFileName);
    sprintf(resultBlifFileName, "%s.batch%i.blif", funcFileName, batchvar);
    batchnum = (int)pow(2, batchvar);
    allNtks = ABC_ALLOC(Abc_Ntk_t*, batchnum);

    for(int i=0; i<batchnum; i++)
    {
        // sub-pla
        fRead = fopen(funcFileName, "r");
        fWrite = fopen(espFileName, "w");
        unused_c = fgets(buff, 102400, fRead);
        fprintf(fWrite, "%s\n", buff);
        unused_c = fgets(buff, 102400, fRead);
        fprintf(fWrite, "%s\n", buff);
        unused_c = fgets(buff, 102400, fRead);
        fprintf(fWrite, "%s\n", buff);
        while(fgets(buff, 102400, fRead))
        {     
            bool nomatch = 0; 
            for(int s=0; s<batchvar; s++)
            {
                char eachbit = ((i>>s)%2)? '1': '0'; 
                if(buff[s] != eachbit)
                    nomatch = 1;
            }
            if(!nomatch)
                fprintf(fWrite, "%s\n", buff);
        }
        fclose(fRead);
        fclose(fWrite);

        // espresso
        sprintf(Command, "./espresso %s", espFileName);
        unused_i = system(Command);

        // get subNtk
        sprintf(Command, "r %s; strash", espResultFileName);
        if(Cmd_CommandExecute(pAbc,Command))
        {
            printf("Cannot read %s\n", espResultFileName);
            return 1;
        }
        if(Cmd_CommandExecute(pAbc,minimizeCommand))
        {
            printf("Error when minimizing esp result network\n");
            return 1;
        }
        Abc_Ntk_t* subNtk = Abc_NtkDup(Abc_FrameReadNtk(pAbc));
        assert(Abc_NtkIsStrash(subNtk) && Abc_NtkCheck(subNtk));
        allNtks[i] = subNtk;

        printf("Sub-circuit %i constructed and minimized\n", i);
    }

    int nPi = Abc_NtkPiNum(allNtks[0]);
    int nPo = Abc_NtkPoNum(allNtks[0]);

    Abc_Ntk_t* pNtkMux = Abc_NtkAlloc( ABC_NTK_STRASH, ABC_FUNC_AIG, 1 );
    Abc_Obj_t* pObj;
    int i, j, k;
    // create pi
    for( i=0; i<nPi; i++)
        Abc_NtkCreatePi(pNtkMux);
    
    // Dup the original
    for( i=0; i<batchnum; i++)
    {
        Abc_NtkCleanCopy(allNtks[i]);
        Abc_AigConst1(allNtks[i])->pCopy = Abc_AigConst1(pNtkMux);
        Abc_NtkForEachPi(allNtks[i], pObj, j)
            pObj->pCopy = Abc_NtkPi(pNtkMux, j);
        Abc_AigForEachAnd(allNtks[i], pObj, j)
            pObj->pCopy = Abc_AigAnd( (Abc_Aig_t *)pNtkMux->pManFunc, Abc_ObjChild0Copy(pObj), Abc_ObjChild1Copy(pObj) );     
    }
    
    // Append mux tree
    for(i=0; i<nPo; i++)
    {
        Vec_Ptr_t* vec_nodes = Vec_PtrAlloc(0);
        Vec_Ptr_t* new_nodes = Vec_PtrAlloc(0);
        for(j=0; j<batchnum; j++)
            Vec_PtrPush(vec_nodes, Abc_ObjChild0Copy(Abc_NtkPo(allNtks[j], i)));
     
        for(j=0; j<batchvar; j++)
        {
            Vec_PtrZero(new_nodes);
            for( k=0; k<Vec_PtrSize(vec_nodes); k+=2)
            {
                pObj = Abc_AigMux((Abc_Aig_t *)pNtkMux->pManFunc, Abc_NtkPi(pNtkMux, j), (Abc_Obj_t*)Vec_PtrGetEntry(vec_nodes, k+1), (Abc_Obj_t*)Vec_PtrGetEntry(vec_nodes, k));
                Vec_PtrPush(new_nodes, pObj);
            }
            Vec_PtrZero(vec_nodes);
            vec_nodes = Vec_PtrDup(new_nodes); 
        }
        Abc_Obj_t* pObjPo = Abc_NtkCreatePo(pNtkMux);
        Abc_ObjAddFanin(pObjPo, (Abc_Obj_t*)Vec_PtrGetEntry(vec_nodes, 0));
        
    }
    Abc_FrameReplaceCurrentNetwork(pAbc, pNtkMux);
    

    //clean up
    sprintf(Command, "rm %s", espFileName);
    unused_i = system(Command);
    sprintf(Command, "rm %s", espResultFileName);
    unused_i = system(Command);
    for( i=0; i<batchnum; i++)
        Abc_NtkDelete(allNtks[i]);
    ABC_FREE(allNtks);

    return result;
}

static int bddminimize_Command( Abc_Frame_t_ * pAbc, int argc, char ** argv )
{
    int result       = 0;
    int c            = 0;
    int fVerbose     = 0;

    int fReorder     = 0;
    int fReverse     = 0;
    int fBddSizeMax  = ABC_INFINITY;

    char* funcFileName;
    char* caresetFileName;
    char * outputFileName;
    FILE * fDump, *fLog;
    char dumpFileName[800];
    Abc_Ntk_t* pNtk_func;
    Abc_Ntk_t* pNtk_careset; 
    Abc_Ntk_t * pNtkNew;
    Abc_Obj_t * pObj;
    int k; 
    int countbdd = 0;
    DdManager * dd;
    DdNode** poNodes; 
    abctime step_time, end_time;
    int cid;
    int gate_c, gate;

    char Command[1000];
    // char strongCommand[1000]   = "if -K 6 -m; mfs2 -W 100 -F 100 -D 100 -L 100 -C 1000000 -e";
    char collapseCommand[1000] = "collapse";
    char minimizeCommand[1000] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";
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

    if ( argc != globalUtilOptind + 3 )
        goto usage;
    // get the input file name
    funcFileName = argv[globalUtilOptind];
    globalUtilOptind++;
    caresetFileName = argv[globalUtilOptind];
    globalUtilOptind++;
    outputFileName = argv[globalUtilOptind];
    sprintf(dumpFileName, "%s.bdd.blif", funcFileName);

    printf("[Info] bdd Li compaction start\n");
    // get pNtk_func
    sprintf( Command, "read %s; strash", funcFileName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", funcFileName);
        return 1;
    }
    pNtk_func = Abc_NtkDup(Abc_FrameReadNtk(pAbc));

    // get pNtk_careset
    sprintf( Command, "read %s", caresetFileName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", caresetFileName);
        return 1;
    }
    if(Cmd_CommandExecute(pAbc,minimizeCommand))
    {
        printf("min %s. Failed.\n", caresetFileName);
        fLog = fopen(outputFileName, "a");
        fprintf( fLog, "%s,%s\n", funcFileName, "error when minimize");
        fclose(fLog);
        return 1;
    }
    pNtk_careset = Abc_NtkDup(Abc_FrameReadNtk(pAbc));

    step_time = Abc_Clock();
    // printf( "BDD construction time = %9.2f sec", 1.0*((double)(Abc_Clock() - step_time))/((double)CLOCKS_PER_SEC) );

    Abc_NtkAppend(pNtk_func, pNtk_careset, 1); // add careset as last output
    if ( Abc_NtkBuildGlobalBdds(pNtk_func, fBddSizeMax, 1, fReorder, fReverse, fVerbose) == NULL )
        printf("pNtk_func convert to bdd failed.\n");
    dd = (DdManager *)Abc_NtkGlobalBddMan( pNtk_func );
    printf( "Shared BDD size = %ld nodes.  \n", Cudd_ReadNodeCount(dd));

    for(int i=0; i<Abc_NtkCoNum(pNtk_func)-1; i++)
        Cudd_bddMinimize(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, i)),(DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, Abc_NtkCoNum(pNtk_func)-1)));
        // Cudd_bddLICompaction(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, i)),(DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, Abc_NtkCoNum(pNtk_func)-1)));
    printf( "Shared BDD size = %ld nodes.  \n", Cudd_ReadNodeCount(dd));
    // printf("path : %lf\n", Cudd_CountPathsToNonZero((DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, Abc_NtkCoNum(pNtk_func)-1))));

    // cuddP(dd, (DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, Abc_NtkCoNum(pNtk_func)-1)));
    Cudd_RecursiveDeref(dd,(DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, Abc_NtkCoNum(pNtk_func)-1))); // deref careset ntk
    printf( "Shared BDD size = %ld nodes.  \n", Cudd_ReadNodeCount(dd));

    poNodes = new DdNode*[Abc_NtkCoNum(pNtk_func)-1];
    fDump = fopen(dumpFileName, "w");
    for(int i=0; i<Abc_NtkCoNum(pNtk_func)-1; i++)
        poNodes[i] = (DdNode*)Abc_ObjGlobalBdd(Abc_NtkCo(pNtk_func, i));
    cid = Cudd_DumpBlif(dd, Abc_NtkCoNum(pNtk_func)-1, poNodes, NULL, NULL, NULL, fDump, 0);
    assert(cid);
    fclose(fDump);
    end_time = Abc_Clock();

    sprintf( Command, "read %s", dumpFileName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", dumpFileName);
        return 1;
    }
    if(Cmd_CommandExecute(pAbc,collapseCommand))
    {
        printf("collapse %s. Failed.\n", caresetFileName);
        fLog = fopen(outputFileName, "a");
        fprintf( fLog, "%s,%s\n", funcFileName, "error when collapse");
        fclose(fLog);
        return 1;
    }
    if(Cmd_CommandExecute(pAbc,minimizeCommand))
    {
        printf("min %s. Failed.\n", caresetFileName);
        fLog = fopen(outputFileName, "a");
        fprintf( fLog, "%s,%s\n", funcFileName, "error when minimize");
        fclose(fLog);
        return 1;
    }
    gate_c = Abc_NtkNodeNum(Abc_FrameReadNtk(pAbc));
    sprintf( Command, "read %s", dumpFileName);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", dumpFileName);
        return 1;
    }
    if(Cmd_CommandExecute(pAbc,minimizeCommand))
    {
        printf("min %s. Failed.\n", caresetFileName);
        fLog = fopen(outputFileName, "a");
        fprintf( fLog, "%s,%s\n", funcFileName, "error when minimize");
        fclose(fLog);
        return 1;
    }
    gate = Abc_NtkNodeNum(Abc_FrameReadNtk(pAbc));

    pNtkNew = Abc_FrameReadNtk(pAbc);
    Abc_NtkForEachCi(pNtkNew, pObj, k)
    {
        // printf("%i; ", Abc_ObjFanoutNum(pObj));
        if(Abc_ObjFanoutNum(pObj))
            countbdd++;
    }

    fLog = fopen(outputFileName, "a");
    fprintf( fLog, "%s,%i,%.2f,%i,%i\n", funcFileName, countbdd, 1.0*((double)(end_time - step_time))/((double)CLOCKS_PER_SEC), gate_c, gate);
    fclose(fLog);

    Abc_NtkFreeGlobalBdds( pNtk_func, 1 );
    return result;

usage:
    Abc_Print( -2, "usage: bddminimize file.blif careset.pla result.log\n" );
    Abc_Print( -2, "\t              perform LiCompaction on file.blif and careset.pla\n" );
    Abc_Print( -2, "\t-v            : verbosity [default = %d]\n", fVerbose );
    Abc_Print( -2, "\t-h            : print the command usage\n" );
    return 1;   
}

// called during ABC startup
static void init(Abc_Frame_t* pAbc)
{ 
    Cmd_CommandAdd( pAbc, "AlCom", "compact", compact_Command, 1);
    Cmd_CommandAdd( pAbc, "AlCom", "gencareset", gencareset_Command, 1);
    Cmd_CommandAdd( pAbc, "AlCom", "checkesp", checkesp_Command, 1);
    Cmd_CommandAdd( pAbc, "ALCom", "bddminimize", bddminimize_Command, 1);
    Cmd_CommandAdd( pAbc, "AlCom", "readplabatch", readplabatch_Command, 1); // helper
}

// called during ABC termination
static void destroy(Abc_Frame_t* pAbc)
{
}

// this object should not be modified after the call to Abc_FrameAddInitializer
static Abc_FrameInitializer_t icompact_frame_initializer = { init, destroy };

// register the initializer a constructor of a global object
// called before main (and ABC startup)
struct icompact_registrar
{
    icompact_registrar() 
    {
        Abc_FrameAddInitializer(&icompact_frame_initializer);
    }
} icompact_registrar_;

ABC_NAMESPACE_IMPL_END