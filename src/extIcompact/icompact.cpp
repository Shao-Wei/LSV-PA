#include "icompact.h"
#include "sat/cnf/cnf.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include "base/io/ioAbc.h" // Io_ReadBlif
#include "bdd/extrab/extraBdd.h"
#include <stdlib.h>        // rand()
#include <errno.h>
#include <vector>
#include <map>
#include <algorithm> // random_shuffle
#include "string.h"

ABC_NAMESPACE_IMPL_START
/////////////////////////////////////////////////////////
// Public Functions
/////////////////////////////////////////////////////////

IcompactMgr::IcompactMgr(Abc_Frame_t * pAbc, char *caresetFileName, char *baseFileName, char *funcFileName, char *resultlogFileName)
{
    // set arguements
    _pAbc = pAbc; // used for executing minimization commands.
    _caresetFileName = caresetFileName;
    _baseFileName = baseFileName;
    _funcFileName = funcFileName;
    _resultlogFileName = resultlogFileName;
    // set flags
    _fMgr = NOERROR;
    _fIcompact = 0;
    _fOcompact = 0;
    _fSupportFunc = 0;
    _fSupportPatt = 0;
    // set file names
    sprintf(_samplesplaFileName,      "%s.samples.pla",   baseFileName);
    sprintf(_reducedplaFileName,      "%s.reduced.pla",   baseFileName);
    sprintf(_outputreencodedFileName, "%s.oreencode.pla", baseFileName);
    sprintf(_outputmappingFileName,   "%s.omap.pla",      baseFileName);
    sprintf(_inputreencodedFileName,  "%s.ireencode.pla", baseFileName);
    sprintf(_inputmappingFileName,    "%s.imap.pla",      baseFileName);
    sprintf(_forqesFileName,          "%s.full.dimacs",   baseFileName);
    sprintf(_forqesCareFileName,      "%s.care.dimacs",   baseFileName);
    sprintf(_MUSFileName,             "%s.gcnf",          baseFileName);
    // set others to NULL
    _pNtk_samples = NULL;
    _pNtk_careset = NULL;
    _pNtk_tmp     = NULL;
    _pNtk_imap    = NULL;
    _pNtk_core    = NULL;
    _pNtk_omap    = NULL;
    
    _supportInfo_func = NULL;
    _supportInfo_patt = NULL;

    _piNames = NULL;
    _poNames = NULL;
    _rpiNames = NULL;
    _rpoNames = NULL;
    ///////////////////////////////////////////////////
    // set basics
    getInfoFromSamples();
    if(!mgrStatus())
    {
        _litPi = new bool[_nPi];
        _litPo = new bool[_nPo];
        _nRPi = _nPi; // _litRPi built later
        _nRPo = _nPo; // _litRPo built later
        strncpy(_workingFileName, _samplesplaFileName, 500);    
    }
    _pNtk_func = getNtk_func();
}

IcompactMgr::~IcompactMgr()
{

}

// returns _nRPo. fOutput = 1 naive, = 0 reencode
int IcompactMgr::ocompact(int fOutput, int fNewVar)
{
    printf("[Info] Start output compaction\n");
    if(mgrStatus()) { return -1; }
    if(_fOcompact) { return 0; }
    if(fOutput == 0) { return _nPo; }

    int * recordPo = new int[_nPo]; // used in reencode_heuristic, see later
    
    _step_time = Abc_Clock();
    if(fOutput == 1)
    {
        printf("  naive binary encoded\n");
        _nRPo = reencode_naive(_outputreencodedFileName, _outputmappingFileName);
    }
        
    else
    {
        printf("  reencode - circuit breaks down into two\n");
        _nRPo = reencode_heuristic(_outputreencodedFileName, _outputmappingFileName, 0, fNewVar, recordPo);
    
    }
        _end_time = Abc_Clock();

    _time_oreencode = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
    printf("reencode computation time: %9.2f\n", _time_oreencode);
    printf("Output compacted to %i / %i\n", _nRPo, _nPo);

    // neccessary 
    strncpy(_workingFileName, _outputreencodedFileName, 500);
    _fOcompact = 1;
    _litRPo = new bool[_nRPo];

    // cleanup
    delete [] recordPo;
    return _nRPo;
}

int IcompactMgr::icompact(SolvingType fSolving, double fRatio, int fNewVar, int fCollapse, int fMinimize, int fBatch, int fIter, int fSupport)
{
    printf("[Info] Start input compaction\n");
    if(mgrStatus()) { return -1; }
    if(_fIcompact) { return 0; }

    int result = 0;
    assert(!check_pla_pipo(_workingFileName, getWorkingPiNum(), getWorkingPoNum()));

    if(fSolving == HEURISTIC)
    {
        printf("  heristic%s- %i iteration\n", (fSupport)?" - support given ": " ", fIter);
        resetWorkingLitPi();
        resetWorkingLitPo();
        _step_time = Abc_Clock();
        result = icompact_heuristic(fIter, fRatio, fSupport);
        if(result == -1) { _fMgr = ICOMPACT_FAIL; }
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName);
    }
    else if(fSolving == HEURISTIC_EACH)
    {
        printf("  heristic%s- %i iteration - on each PO\n", (fSupport)?" - support given ": " ", fIter);
        resetWorkingLitPi();
        resetWorkingLitPo();
        _step_time = Abc_Clock();
        result = icompact_heuristic_each(fIter, fRatio, fSupport);
        if(result == -1) { _fMgr = ICOMPACT_FAIL; }
        _end_time = Abc_Clock();

        // report specially handled
        
        /*
        int nPo = getWorkingPoNum();
        bool * litPo = getWorkingLitPo();
        _step_time = Abc_Clock();
        for(int i=0; i<nPo; i++)
        {
            for(int k=0; k<nPo; k++)
                litPo[k] = 0;
            litPo[i] = 1;
            resetWorkingLitPi();
            result = icompact_heuristic(1, fRatio);
            if(result == -1) { _fMgr = ICOMPACT_FAIL; }
            assert(result > 0);
            writeCompactpla(_tmpFileName);
            _pNtk_tmp = Io_Read(_tmpFileName, Io_ReadFileType(_tmpFileName), 1, 0);
            _pNtk_tmp = Abc_NtkToLogic(_pNtk_tmp);
            _pNtk_tmp = Abc_NtkStrash(_pNtk_tmp, 0, 0, 0);
            _pNtk_tmp = ntkMinimize(_pNtk_tmp, BASIC, 0);
            if(_pNtk_core == NULL)
                _pNtk_core = Abc_NtkDup(_pNtk_tmp);
            else
                Abc_NtkAppend(_pNtk_core, _pNtk_tmp, 1);
        }
        _end_time = Abc_Clock();
        _pNtk_core = ntkMinimize(_pNtk_core, BASIC, 0);
        _time_core = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        _gate_core = Abc_NtkNodeNum(_pNtk_core);
        printf("time: %9.2f; gate: %i\n", _time_core, _gate_core);
        */
    }
    else if(fSolving == LEXSAT_CLASSIC)
    {
        printf("  classic sat-based\n");
        getNtk_samples(fMinimize, fCollapse);
        getNtk_careset(fMinimize, fCollapse, fBatch);
        resetWorkingLitPi();
        resetWorkingLitPo();
        _step_time = Abc_Clock();
        result = icompact_main();
        if(result == -1) { _fMgr = ICOMPACT_FAIL; }
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName);
    }
    else if(fSolving == LEXSAT_ENCODE_C)
    {        
        printf("  classic sat-based - c encoded\n");
        getNtk_careset(fMinimize, fCollapse, fBatch);
        resetWorkingLitPi();
        resetWorkingLitPo();
        _step_time = Abc_Clock();
        result = icompact_direct_encode_with_c();
        if(result == -1) { _fMgr = ICOMPACT_FAIL; }
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName);
    }
    else if(fSolving == REENCODE)
    {
        // add modification on working files
        printf("  reencode - circuit breaks down into two\n");
        int * recordPi = new int[_nPi];
        resetWorkingLitPi();
        resetWorkingLitPo();
        _step_time = Abc_Clock();
        result = reencode_heuristic(_inputmappingFileName, _inputreencodedFileName, 1, fNewVar, recordPi);
        _end_time = Abc_Clock();
        _time_ireencode = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        printf("reencode computation time: %9.2f\n", _time_ireencode);
        printf("Input compacted to %i / %i\n", result, _nPi);
    }
    
    if(result != -1 && fSolving != REENCODE && fSolving != HEURISTIC_EACH)
    {
        int nPi = getWorkingPiNum();
        bool * litPi = getWorkingLitPi();
        _time_icompact = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        printf("icompact computation time: %9.2f\n", _time_icompact);
        printf("icompact result: %i / %i\n", result, nPi);
        for(int i=0; i<nPi; i++)
            if(litPi[i]) { printf("%i ", i); }
        printf("\n");
    }

    // neccessary
    if(mgrStatus()) { return -1; } // avoid alloc -1
    _fIcompact = 1;
    _nRPi = result;
    _litRPi = new bool[_nRPi];

    // cleanup
    return _nRPi;
}

Abc_Ntk_t * IcompactMgr::getNtkImap()
{
    if(mgrStatus()) 
        return NULL;
    return NULL;
}

Abc_Ntk_t * IcompactMgr::getNtkCore()
{
    if(mgrStatus()) 
        return NULL;
    return NULL;
}

Abc_Ntk_t * IcompactMgr::getNtkOmap()
{
    if(mgrStatus()) 
        return NULL;
    return NULL;
}

void IcompactMgr::getSupportInfoFunc()
{
    if(mgrStatus()) 
        return;
    supportInfo_Func();
    printf("Report support info func\n");
}

void IcompactMgr::getSupportInfoPatt()
{
    if(mgrStatus()) 
        return;
    supportInfo_Patt();
    printf("Report support info patt\n");
}

void IcompactMgr::getEachConeFunc()
{
    if(mgrStatus()) 
        return;
    Abc_Ntk_t* pCone;
    Abc_Obj_t* pPo;
    int i;
    Abc_NtkForEachCi(_pNtk_func, pPo, i)
    {
        pCone = Abc_NtkCreateCone(_pNtk_func, pPo, Abc_ObjName(pPo), 0);
    }
}

void IcompactMgr::getEachConePatt()
{
    if(mgrStatus()) 
        return;
    Abc_Ntk_t* pPutOnTop = Abc_NtkPutOnTop(_pNtk_core, _pNtk_omap);
    Abc_Ntk_t* pCone;
    Abc_Obj_t* pPo;
    int i;
    Abc_NtkForEachCi(pPutOnTop, pPo, i)
    {
        pCone = Abc_NtkCreateCone(pPutOnTop, pPo, Abc_ObjName(pPo), 0);
    }
}

/////////////////////////////////////////////////////////
// Aux functions
/////////////////////////////////////////////////////////
// returns true if something is wrong
bool IcompactMgr::mgrStatus()
{
    if(_fMgr == BADSAMPLES)
        printf("Mgr lockdown. Error: Bad samples file \n");
    else if(_fMgr == BADEVAL)
        printf("Mgr lockdown. Error: Bad evaluation file \n");
    else if(_fMgr == ICOMPACT_FAIL)
        printf("Mgr lockdown. Error: unexpected failure in input compaction \n");
    return (_fMgr != NOERROR);
}

// all set to zero
void IcompactMgr::resetWorkingLitPi()
{
    if(_fIcompact)
    {
        for(int i=0; i<_nRPi; i++)
            _litRPi[i] = 0;
    }
    else
    {
        for(int i=0; i<_nPi; i++)
            _litPi[i] = 0;
    }
}

// all set to one
void IcompactMgr::resetWorkingLitPo()
{
    if(_fOcompact)
    {
        for(int i=0; i<_nRPo; i++)
            _litRPo[i] = 1;
    }
    else
    {
        for(int i=0; i<_nPo; i++)
            _litPo[i] = 1;
    }
}

// check if working litPi has at least one 1
bool IcompactMgr::validWorkingLitPi()
{
    int count = 0;
    int nPi = (_fIcompact)? _nRPi: _nPi;
    bool * litPi = (_fIcompact)? _litRPi: _litPi;

    for(int i=0; i<nPi; i++)
        if(litPi[i]) { count++; }

    return (count > 0);
}

// check if working litPo has at least one 1
bool IcompactMgr::validWorkingLitPo()
{
    int count = 0;
    int nPo = (_fOcompact)? _nRPo: _nPo;
    bool * litPo = (_fOcompact)? _litRPo: _litPo;

    for(int i=0; i<nPo; i++)
        if(litPo[i]) { count++; }

    return (count > 0);
}

/////////////////////////////////////////////////////////
// Support Set
/////////////////////////////////////////////////////////

// two get_support functions has different po order, sort ori to match pla order.
// check get_support_pat, support sizes are greater than expected 
void IcompactMgr::supportInfo_Func()
{
    // build _supportInfo_func
    if(!_fEval)
    {
        printf("No evaluation file.\n");
        _fMgr = BADEVAL;
        return;
    }

    _fSupportFunc = 1;
    _supportInfo_func = new int*[_nPo];
    for(int i=0; i<_nPo; i++)
        _supportInfo_func[i] = new int[_nPi](); // init to zero
    
    Abc_Ntk_t* pNtk = _pNtk_func;

    Abc_Obj_t* pPo;
    Vec_Int_t* vecSup;
    int i;
    Abc_NtkForEachPo(pNtk, pPo, i)
    {
        vecSup = Abc_NtkNodeSupportInt(pNtk, i);
        for(int j=0, n=Vec_IntSize(vecSup); j<n; j++)
            _supportInfo_func[i][Vec_IntEntry(vecSup, j)] = 1;
    }
}

void IcompactMgr::supportInfo_Patt()
{
    if(_fIcompact || _fOcompact) // post-icompact/ocompact affects icompact_heuristic
    {
        printf("Does not support post-icompact/ocompact. Aborted.\n");
        return;
    }

    _fSupportPatt = 1;
    int result;
    int nPi = getWorkingPiNum();
    int nPo = getWorkingPoNum();
    bool* litPi = getWorkingLitPi();
    bool* litPo = getWorkingLitPo();

    // build _supportInfo_patt
    _supportInfo_patt = new int*[nPo];
    for(int i=0; i<nPo; i++)
        _supportInfo_patt[i] = new int[nPi](); // init to zero

    for(int i=0; i<nPo; i++)
    {
        for(int j=0; j<nPo; j++)
            litPo[j] = 0;
        litPo[i] = 1;
        result = icompact_heuristic(8, 0, 0);
        for(int j=0; j<nPi; j++)
            _supportInfo_patt[i][j] = (litPi[j])? 1: 0;
    }
}

/////////////////////////////////////////////////////////
// Ntk Functions
/////////////////////////////////////////////////////////
Abc_Ntk_t * IcompactMgr::getNtk_func()
{
    if(_funcFileName != NULL)
    {
        _pNtk_func = Io_ReadBlif(_funcFileName, 1);
        _pNtk_func = Abc_NtkToLogic(_pNtk_func);
        _pNtk_func = Abc_NtkStrash(_pNtk_func, 0, 0, 0);
        // check consistency w/ samples
        if(Abc_NtkPiNum(_pNtk_func) != _nPi || Abc_NtkPoNum(_pNtk_func) != _nPo)
        { 
            printf("Evaluation file %s is not consistent with sampling file. File dropped.\n", _funcFileName); 
            return NULL;
        }
        _fEval = 1;
        return _pNtk_func;
    }
    return NULL;
}

void IcompactMgr::getInfoFromSamples()
{
    Abc_Ntk_t *pNtk = Io_Read(_samplesplaFileName, Io_ReadFileType(_samplesplaFileName), 1, 0);
    if(pNtk == NULL)
    {
        printf("Bad samples file %s.\n", _samplesplaFileName);
        _fMgr = BADSAMPLES;
        return;
    }

    Abc_Obj_t *pObj;
    int i;
    // set _nPi, _nPo
    _nPi = Abc_NtkPiNum(pNtk);
    _nPo = Abc_NtkPoNum(pNtk);
    // set names
    _piNames = new char*[_nPi];
    Abc_NtkForEachPi(pNtk, pObj, i)
        _piNames[i] = Abc_ObjName(pObj);
    _poNames = new char*[_nPo];
    Abc_NtkForEachPo(pNtk, pObj, i)
        _poNames[i] = Abc_ObjName(pObj);
}

// Caution: may result in huge Ntk. Batching added later.
Abc_Ntk_t * IcompactMgr::getNtk_samples(int fMinimize, int fCollapse)
{
    if(_pNtk_samples != NULL)
        return _pNtk_samples;
    _pNtk_samples = Io_Read(_samplesplaFileName, Io_ReadFileType(_samplesplaFileName), 1, 0);
    // _pNtk_samples = Abc_NtkToLogic(_pNtk_samples);
    _pNtk_samples = Abc_NtkStrash(_pNtk_samples, 0, 0, 0);
    _pNtk_samples = ntkMinimize(_pNtk_samples, fMinimize, fCollapse);
    return _pNtk_samples;
}

// Caution: may result in huge Ntk. Batching added later.
// .type fr treat unseen patterns as don't care
Abc_Ntk_t * IcompactMgr::getNtk_careset(int fMinimize, int fCollapse, int fBatch)
{
    if(_pNtk_careset != NULL)
        return _pNtk_careset;
    _pNtk_careset = Io_Read(_caresetFileName, Io_ReadFileType(_caresetFileName), 1, 0);
    // _pNtk_careset = Abc_NtkToLogic(_pNtk_careset);
    _pNtk_careset = Abc_NtkStrash(_pNtk_careset, 0, 0, 0);
    _pNtk_careset = ntkMinimize(_pNtk_careset, fMinimize, fCollapse);
    return _pNtk_careset;
}

// Currently uses _pAbc to execute minimization commands
// .type fd treat unseen patterns as offset
Abc_Ntk_t * IcompactMgr::ntkMinimize(Abc_Ntk_t * pNtk, int fMinimize, int fCollapse)
{
    // minimization commands
    char strongCommand[500]   = "if -K 6 -m; mfs2 -W 100 -F 100 -D 100 -L 100 -C 1000000 -e";
    char collapseCommand[500] = "collapse";
    char minimizeCommand[500] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";
    Abc_FrameReplaceCurrentNetwork(_pAbc, pNtk);
    if(fMinimize == STRONG)
    {
        if(Cmd_CommandExecute(_pAbc,strongCommand))
            return NULL;
    }
    if(fCollapse)
    {
        if(Cmd_CommandExecute(_pAbc,collapseCommand))
            return NULL;
    }
    if(fMinimize != NOMIN)
    {
        if(Cmd_CommandExecute(_pAbc,minimizeCommand))
            return NULL;
    }
    pNtk = Abc_NtkDup(Abc_FrameReadNtk(_pAbc));
    return pNtk;
}

// // ntkBatch() not used
// // fMode = 1 for samples, 0 for careset // 
// Abc_Ntk_t * IcompactMgr::ntkBatch(int fMode, int fBatch)
// {
//     char Command[2000];
//     if(fMode)
//     {
//         _step_time = Abc_Clock();
//         sprintf(Command, "readplabatch %i %s", fBatch, _samplesplaFileName);
//         if(Cmd_CommandExecute(_pAbc,Command))
//             return NULL;
//         _pNtk_samples = Abc_NtkDup(Abc_FrameReadNtk(_pAbc));
//         _end_time = Abc_Clock();
//         _gate_batch = Abc_NtkNodeNum(_pNtk_samples);
//         _time_batch = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
//         return _pNtk_samples;
//     }
//     else
//     {
//         _step_time = Abc_Clock();
//         sprintf(Command, "readplabatch %i %s", fBatch, _caresetFileName);
//         if(Cmd_CommandExecute(_pAbc,Command))
//             return NULL;
//         _pNtk_careset = Abc_NtkDup(Abc_FrameReadNtk(_pAbc));
//         _end_time = Abc_Clock();
//         _gate_batch = Abc_NtkNodeNum(_pNtk_careset);
//         _time_batch = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
//         return _pNtk_careset;
//     }      
// }

void IcompactMgr::writeCompactpla(char* outputplaFileName)
{
    if(!validWorkingLitPi()) // handle constant case later
    {
        printf("No compact pla written.\n");
        return;
    }
    int nPi = getWorkingPiNum();
    int nPo = getWorkingPoNum();
    bool * litPi = getWorkingLitPi();
    bool * litPo = getWorkingLitPo();
    char * plaFile = _workingFileName;

    FILE* fcompactpla = fopen(outputplaFileName, "w");
    FILE* fpattern = fopen(plaFile, "r");
    char buff[102400];
    char * unused __attribute__((unused)); // get rid of fget warnings
    int lenPi, lenPo;

    int count = 0;
    for(int i=0; i<nPi; i++)
        if(litPi[i])
            count++;
    lenPi = count;
    count = 0;
    for(int i=0; i<nPo; i++)
        if(litPo[i])
            count++;
    lenPo = count;
    
    char* one_line = new char[lenPi + lenPo + 2];
    one_line[lenPi] = ' ';
    one_line[lenPi + lenPo + 1] = '\0';

    unused = fgets(buff, 102400, fpattern);
    unused = fgets(buff, 102400, fpattern);
    unused = fgets(buff, 102400, fpattern);
    fprintf(fcompactpla, ".i %i\n", lenPi);
    fprintf(fcompactpla, ".o %i\n", lenPo);
    fprintf(fcompactpla, ".type fr\n");
    while(fgets(buff, 102400, fpattern))
    {
        int local_count = 0;
        for(int i=0; i<nPi; i++)
            if(litPi[i])
            {
                one_line[local_count] = buff[i];
                local_count++;
            }
        local_count = 0;
        for(int i=0; i<nPo; i++)
            if(litPo[i])
            {
                one_line[lenPi + 1 + local_count] = buff[nPi + 1 + i];
                local_count++;
            }    
        fprintf(fcompactpla, "%s\n", one_line);
    }

    fclose(fcompactpla);
    fclose(fpattern);
    delete [] one_line;
}

/////////////////////////////////////////////////////////
// Input compact methods
/////////////////////////////////////////////////////////

// Slightly modified. sat/bsat/satSolver.h
int sat_solver_conditional_unequal(sat_solver * pSat, int iVar, int iVar2, int iVarCond )
{
    lit Lits[3];
    int Cid;
    assert( iVar >= 0 );
    assert( iVar2 >= 0 );
    assert( iVarCond >= 0 );

    Lits[0] = toLitCond( iVarCond, 0 );
    Lits[1] = toLitCond( iVar, 1 );
    Lits[2] = toLitCond( iVar2, 0 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    
    Lits[0] = toLitCond( iVarCond, 0 );
    Lits[1] = toLitCond( iVar, 0 );
    Lits[2] = toLitCond( iVar2, 1 );
    Cid = sat_solver_addclause( pSat, Lits, Lits + 3 );
    assert( Cid );
    
    return 2;
}

int sat_solver_iff_unequal(sat_solver *pSat, int iVar, int iVar2, int iVarCond)
{
    // (x!=x' <> cond)
    // (x=x' + cond)^(x!=x + ~cond)
    lit Lits[3];
    int Cid;
    assert(iVar >= 0);
    assert(iVar2 >= 0);
    assert(iVarCond >= 0);

    // (x=x' + cond)
    Lits[0] = toLitCond(iVarCond, 0);
    Lits[1] = toLitCond(iVar, 1);
    Lits[2] = toLitCond(iVar2, 0);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    assert(Cid);

    Lits[0] = toLitCond(iVarCond, 0);
    Lits[1] = toLitCond(iVar, 0);
    Lits[2] = toLitCond(iVar2, 1);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    assert(Cid);

    // (x!=x + ~cond)
    Lits[0] = toLitCond(iVarCond, 1);
    Lits[1] = toLitCond(iVar, 0);
    Lits[2] = toLitCond(iVar2, 0);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    assert(Cid);

    Lits[0] = toLitCond(iVarCond, 1);
    Lits[1] = toLitCond(iVar, 1);
    Lits[2] = toLitCond(iVar2, 1);
    Cid = sat_solver_addclause(pSat, Lits, Lits + 3);
    assert(Cid);

    return 4;
}

int sat_solver_addclause_from( sat_solver* pSat, Cnf_Dat_t * pCnf )
{
	for (int i = 0; i < pCnf->nClauses; i++ )
	{
		if (!sat_solver_addclause( pSat, pCnf->pClauses[i], pCnf->pClauses[i+1] ))
            return false;
	}
    return true;
}

void sat_solver_print( sat_solver* pSat, int fDimacs )
{
    Sat_Mem_t * pMem = &pSat->Mem;
    clause * c;
    int i, k, nUnits;

    // count the number of unit clauses
    nUnits = 0;
    for ( i = 0; i < pSat->size; i++ )
        if ( pSat->levels[i] == 0 && pSat->assigns[i] != 3 )
            nUnits++;

//    fprintf( pFile, "c CNF generated by ABC on %s\n", Extra_TimeStamp() );
    printf( "p cnf %d %d\n", pSat->size, Sat_MemEntryNum(&pSat->Mem, 0)-1+Sat_MemEntryNum(&pSat->Mem, 1)+nUnits );

    // write the original clauses
    Sat_MemForEachClause( pMem, c, i, k )
    {
        int i;
        for ( i = 0; i < (int)c->size; i++ )
            printf( "%s%d ", (lit_sign(c->lits[i])? "-": ""),  lit_var(c->lits[i]) + (fDimacs>0) );
        if ( fDimacs )
            printf( "0" );
        printf( "\n" );
    }

    // write the learned clauses
//    Sat_MemForEachLearned( pMem, c, i, k )
//        Sat_SolverClauseWriteDimacs( pFile, c, fDimacs );

    // write zero-level assertions
    for ( i = 0; i < pSat->size; i++ )
        if ( pSat->levels[i] == 0 && pSat->assigns[i] != 3 ) // varX
            printf( "%s%d%s\n",
                     (pSat->assigns[i] == 1)? "-": "",    // var0
                     i + (int)(fDimacs>0),
                     (fDimacs) ? " 0" : "");
    printf( "\n" );
}

int IcompactMgr::icompact_heuristic(int iterNum, double fRatio, int fSupport)
{
    int result = -1;
    if(fSupport)
    {
        supportInfo_Func(); // get real support
        if(_fOcompact)
        {
            printf("icompact_heuristic() with support info is not supported post output compaction.\n");
            return -1;
        }
    }
    if(mgrStatus()) { return -1; }

    bool * minMask;
    
    ICompactHeuristicMgr* mgr = new ICompactHeuristicMgr(_workingFileName);
    mgr->lockEntry(fRatio);
    if(fSupport)
        mgr->supportInfo(_supportInfo_func);
    minMask = mgr->compact_drop(iterNum, 1);
    if(minMask == NULL) { return -1; }
    
    // report
    int nPi = getWorkingPiNum();
    bool * litPi = getWorkingLitPi();
    for(size_t i=0; i<nPi; i++)
        litPi[i] = 0;
    for(size_t i=0; i<nPi; i++)
    {
        if(minMask[i] == 1)
        {
            litPi[i] = 1;
            result++;
        }
    }

    return result;
}

int IcompactMgr::icompact_heuristic_each(int iterNum, double fRatio, int fSupport)
{
    int result = -1;
    if(fSupport)
    {
        supportInfo_Func(); // get real support
        if(_fOcompact)
        {
            printf("icompact_heuristic_each() with support info is not supported post output compaction.\n");
            return -1;
        }
    }
    if(mgrStatus()) { return -1; }

    bool ** minMaskList;
    
    ICompactHeuristicMgr* mgr = new ICompactHeuristicMgr(_workingFileName);
    mgr->lockEntry(fRatio);
    if(fSupport)
        mgr->supportInfo(_supportInfo_func);
    minMaskList = mgr->compact_drop_each(iterNum);
    if(minMaskList == NULL) { return -1; }
    
    // report specially handled

    return result;
}

// precondition: _pNtk_func, _pNtk_careset
int IcompactMgr::icompact_main()
{
    int result = -1;
    if(mgrStatus()) { return -1; }
    if(!validWorkingLitPo()) { return -1; }

    int nPi = getWorkingPiNum();
    int nPo = getWorkingPoNum();
    bool * litPi = getWorkingLitPi();
    bool * litPo = getWorkingLitPo();
    Abc_Ntk_t * pNtk_func = _pNtk_func;
    Abc_Ntk_t * pNtk_careset = _pNtk_careset;
    int pLits[nPi]; // satSolver assumption
    int nLitPo = 0;
    for(int i=0; i<nPo; i++)
        if(litPo[i]) { nLitPo++; }    

    ///////////////////////////////////////////////////////////////////////
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    Abc_Ntk_t *pNtkOri = Abc_NtkDup(pNtk_careset);
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtkOri);

    Vec_Int_t *pOriPi = Vec_IntAlloc(nPi);
    {
        Aig_Man_t *pAigOri = Abc_NtkToDar(pNtkOri, 0, 0);
        Cnf_Dat_t *pCnfOri = Cnf_Derive(pAigOri, Abc_NtkCoNum(pNtkOri));
        Cnf_DataLift(pCnfOri, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOri);
        Aig_Obj_t * pAigObj = Aig_ManCo(pCnfOri->pMan, 0);
        int var = pCnfOri->pVarNums[Aig_ObjId(pAigObj)];
        assert(var > 0);
        cid = sat_solver_add_const(pSolver, var, 0);
        sat_solver_print(pSolver, 1);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjOri = Aig_ManCi(pCnfOri->pMan, i);
            int var = pCnfOri->pVarNums[Aig_ObjId(pAigObjOri)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOriPi, var);
        }
        // memory free
        Cnf_DataFree(pCnfOri);
        Aig_ManStop(pAigOri);
    }

    Vec_Int_t *pDupPi = Vec_IntAlloc(nPi);
    {
        Aig_Man_t *pAigDup = Abc_NtkToDar(pNtkDup, 0, 0);
        Cnf_Dat_t *pCnfDup = Cnf_Derive(pAigDup, Abc_NtkCoNum(pNtkDup));
        Cnf_DataLift(pCnfDup, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfDup);
        Aig_Obj_t * pAigObj = Aig_ManCo(pCnfDup->pMan, 0);
        int var = pCnfDup->pVarNums[Aig_ObjId(pAigObj)];
        assert(var > 0);
        cid = sat_solver_add_const(pSolver, var, 0);
        sat_solver_print(pSolver, 1);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjDup = Aig_ManCi(pCnfDup->pMan, i);
            int var = pCnfDup->pVarNums[Aig_ObjId(pAigObjDup)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pDupPi, var);
        }
        // memory free
        Cnf_DataFree(pCnfDup);
        Aig_ManStop(pAigDup);
    }
    // printf("careset clauses / variables: %i / %i\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver));
    Abc_Ntk_t *pNtkOut1 = Abc_NtkDup(pNtk_func);
    Abc_Ntk_t *pNtkOut2 = Abc_NtkDup(pNtkOut1);

    Vec_Int_t *pOut1Pi = Vec_IntAlloc(nPi);
    Vec_Int_t *pOut1Po = Vec_IntAlloc(nPo);
    {
        Aig_Man_t *pAigOut1 = Abc_NtkToDar(pNtkOut1, 0, 0);
        Cnf_Dat_t *pCnfOut1 = Cnf_Derive(pAigOut1, Abc_NtkCoNum(pNtkOut1));
        Cnf_DataLift(pCnfOut1, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut1);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCi(pCnfOut1->pMan, i);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjOut1)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut1Pi, var);
        }
        // store var of po
        for (int i=0; i<nPo; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCo(pCnfOut1->pMan, i);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjOut1)];
            // Abc_Print(1, "var of po %d: %d\n", i, var);
            Vec_IntPush(pOut1Po, var);
        }
        // memory free
        Cnf_DataFree(pCnfOut1);
        Aig_ManStop(pAigOut1);
    }

    for (int i=0; i<nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOri = Vec_IntEntry(pOriPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut1, varOri, 0);
        assert(cid);
    }

    Vec_Int_t *pOut2Pi = Vec_IntAlloc(nPi);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(nPo);
    {
        Aig_Man_t *pAigOut2 = Abc_NtkToDar(pNtkOut2, 0, 0);
        Cnf_Dat_t *pCnfOut2 = Cnf_Derive(pAigOut2, Abc_NtkCoNum(pNtkOut2));
        Cnf_DataLift(pCnfOut2, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut2);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCi(pCnfOut2->pMan, i);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjOut2)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut2Pi, var);
        }
        // store var of po
        for (int i=0; i<nPo; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCo(pCnfOut2->pMan, i);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjOut2)];
            // Abc_Print(1, "var of po %d: %d\n", i, var);
            Vec_IntPush(pOut2Po, var);
        }
        // memory free
        Cnf_DataFree(pCnfOut2);
        Aig_ManStop(pAigOut2);
    }

    for (int i=0; i<nPi; i++)
    {
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut2, varDup, 0);
        assert(cid);
    }
    // printf("function clauses / variables: %i / %i\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver));
    // Alpha : (x=x' + alpha)
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(nPi);
    for (int i=0; i<nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_conditional_unequal(pSolver, varOri, varDup, varAlphaControl);
        assert(cid);
    }

    // Beta : (x!=x' <> beta)
    Vec_Int_t *pBetaControls = Vec_IntAlloc(nPi);
    for (int i=0; i<nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varBetaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pBetaControls, varBetaControl);
        cid = sat_solver_iff_unequal(pSolver, varOri, varDup, varBetaControl);
        assert(cid);
    }

    // V(Alpha ^ Beta)
    Vec_Int_t *pAndAlphaBeta = Vec_IntAlloc(nPi);
    for (int i = 0; i < nPi; i++)
    {
        int varAlpha = Vec_IntEntry(pAlphaControls, i);
        int varBeta = Vec_IntEntry(pBetaControls, i);
        int varAnd = sat_solver_addvar(pSolver);
        Vec_IntPush(pAndAlphaBeta, varAnd);
        cid = sat_solver_add_and(pSolver, varAnd, varAlpha, varBeta, 0, 0, 0);
        assert(cid);
    }

    lit Lits_AlphaBeta[nPi];
    for (int i=0; i<nPi; i++)
    {
        int varAnd = Vec_IntEntry(pAndAlphaBeta, i);
        Lits_AlphaBeta[i] = toLitCond(varAnd, 0);
    }   
    cid = sat_solver_addclause(pSolver, Lits_AlphaBeta, Lits_AlphaBeta + nPi);
    assert(cid);

    // Gamma : (o != o' <> gamma)
    Vec_Int_t *pGammaControls = Vec_IntAlloc(nPo);
    for(int i=0; i<nPo; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Po, i);
        int varOut2 = Vec_IntEntry(pOut2Po, i);
        int varGammaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pGammaControls, varGammaControl);
        cid = sat_solver_iff_unequal(pSolver, varOut1, varOut2, varGammaControl);
        assert(cid);
    }

    // V(Gamma)
    lit Lits_Gamma[nLitPo];
    int count = 0;
    for(int i=0; i<nPo; i++)
    {
        if(litPo[i])
        {
            int var = Vec_IntEntry(pGammaControls, i);
            Lits_Gamma[count] = toLitCond(var, 0);
            count++;
        }
    }
    cid = sat_solver_addclause(pSolver, Lits_Gamma, Lits_Gamma + nLitPo);
    assert(cid);

    ///////////////////////////////////////////////////////////////////////
    // printf("Total clauses / variables: %i / %i\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver));
    // set assumption order
    for (int i=0; i<nPi; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pAlphaControls, i),1);
    result = sat_solver_minimize_assumptions2(pSolver, pLits, nPi, 0);

    // update litPi
    for(int i=0; i<nPi; i++)
        litPi[i] = 0;
    for(int i=0; i<result; i++)
    {
        int var = Abc_Lit2Var(pLits[i]) - Vec_IntEntry(pAlphaControls, 0);
        assert(var >= 0 && var < nPi);
        litPi[var] = 1;
    }

    // clean up
    sat_solver_delete(pSolver);
    Abc_NtkDelete(pNtkOri);
    Abc_NtkDelete(pNtkDup);
    Vec_IntFree(pOriPi);
    Vec_IntFree(pDupPi);
    Vec_IntFree(pAlphaControls);
    Vec_IntFree(pBetaControls);
    Vec_IntFree(pAndAlphaBeta);
    Vec_IntFree(pGammaControls);

    return result;
}

// precondition: _workingFileName, pNtk_careset
int IcompactMgr::icompact_direct_encode_with_c()
{
    int result = -1;
    if(mgrStatus()) { return -1; }
    if(!validWorkingLitPo()) { return -1; }

    int nPi = getWorkingPiNum();
    int nPo = getWorkingPoNum();
    bool * litPi = getWorkingLitPi();
    bool * litPo = getWorkingLitPo();
    char* plaFile = _workingFileName;
    Abc_Ntk_t* pNtk_careset = _pNtk_careset;
    int pLits[nPi];
    char buff[102400];
    char * unused __attribute__((unused)); // get rid of fget warnings
    FILE *ff, *fm, *fPla; // file Forqes, file Muser2, file working pla  
    int nLitPo = 0;
    for(int i=0; i<nPo; i++)
        if(litPo[i]) { nLitPo++; }
    
    fPla = fopen(plaFile, "r");
    unused = fgets(buff, 102400, fPla);
    unused = fgets(buff, 102400, fPla);
    unused = fgets(buff, 102400, fPla);
    ///////////////////////////////////////////////////////////////////////
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    Abc_Ntk_t *pNtkOri = Abc_NtkDup(pNtk_careset);
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtkOri);

    Vec_Int_t *pOriPi = Vec_IntAlloc(nPi);
    {
        Aig_Man_t *pAigOri = Abc_NtkToDar(pNtkOri, 0, 0);
        Cnf_Dat_t *pCnfOri = Cnf_Derive(pAigOri, Abc_NtkCoNum(pNtkOri));
        Cnf_DataLift(pCnfOri, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOri);
        Aig_Obj_t * pAigObj = Aig_ManCo(pCnfOri->pMan, 0);
        int var = pCnfOri->pVarNums[Aig_ObjId(pAigObj)];
        assert(var > 0);
        cid = sat_solver_add_const(pSolver, var, 0);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjOri = Aig_ManCi(pCnfOri->pMan, i);
            int var = pCnfOri->pVarNums[Aig_ObjId(pAigObjOri)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOriPi, var);
        }
        // memory free
        Cnf_DataFree(pCnfOri);
        Aig_ManStop(pAigOri);
    }

    Vec_Int_t *pDupPi = Vec_IntAlloc(nPi);
    {
        Aig_Man_t *pAigDup = Abc_NtkToDar(pNtkDup, 0, 0);
        Cnf_Dat_t *pCnfDup = Cnf_Derive(pAigDup, Abc_NtkCoNum(pNtkDup));
        Cnf_DataLift(pCnfDup, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfDup);
        Aig_Obj_t * pAigObj = Aig_ManCo(pCnfDup->pMan, 0);
        int var = pCnfDup->pVarNums[Aig_ObjId(pAigObj)];
        assert(var > 0);
        cid = sat_solver_add_const(pSolver, var, 0);
        // store var of pi
        for (int i=0; i<nPi; i++)
        {
            Aig_Obj_t *pAigObjDup = Aig_ManCi(pCnfDup->pMan, i);
            int var = pCnfDup->pVarNums[Aig_ObjId(pAigObjDup)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pDupPi, var);
        }
        // memory free
        Cnf_DataFree(pCnfDup);
        Aig_ManStop(pAigDup);
    }
    Vec_Int_t *pOut1Pi = Vec_IntAlloc(nPi);
    Vec_Int_t *pOut2Pi = Vec_IntAlloc(nPi);
    {
        for(int i=0; i<nPi; i++)
        {
            int newVar1 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut1Pi, newVar1);
            int newVar2 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut2Pi, newVar2);
        }
    }
    Vec_Int_t *pOut1Po = Vec_IntAlloc(nPo);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(nPo);
    {
        for(int i=0; i<nPo; i++)
        {
            int newVar1 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut1Po, newVar1);
            int newVar2 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut2Po, newVar2);
        }
    }
    
    // i0 i1 i2 o0 o1 o2
    // -i0 -i1 -i2 c
    // -c o0
    // -c o1
    // -c o2
    while(fgets(buff, 102400, fPla))
    {
        lit Lits_Pattern1[nPi+1];
        int c1 = sat_solver_addvar(pSolver);
        for(int i=0; i<nPi; i++)
        {
            int phase = (buff[i] == '1')? 1: 0;
            int varIn = Vec_IntGetEntry(pOut1Pi, i);
            Lits_Pattern1[i] = toLitCond(varIn, phase);
        }
        Lits_Pattern1[nPi] = toLitCond(c1, 0);
        cid = sat_solver_addclause(pSolver, Lits_Pattern1, Lits_Pattern1 + nPi + 1);
        assert(cid);

        for(int i=0; i<nPo; i++)
        {
            lit Lits_Out1[2];
            int phase = (buff[nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut1Po, i);
            Lits_Out1[0] = toLitCond(c1, 1);
            Lits_Out1[1] = toLitCond(varOut, phase);
            cid = sat_solver_addclause(pSolver, Lits_Out1, Lits_Out1 + 2);
            assert(cid);
        }

        lit Lits_Pattern2[nPi+1];
        int c2 = sat_solver_addvar(pSolver);
        for(int i=0; i<nPi; i++)
        {
            int phase = (buff[i] == '1')? 1: 0;
            int varIn = Vec_IntGetEntry(pOut2Pi, i);
            Lits_Pattern2[i] = toLitCond(varIn, phase);
        }
        Lits_Pattern2[nPi] = toLitCond(c2, 0);
        cid = sat_solver_addclause(pSolver, Lits_Pattern2, Lits_Pattern2 + nPi + 1);
        assert(cid);

        for(int i=0; i<nPo; i++)
        {
            lit Lits_Out2[2];
            int phase = (buff[nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut2Po, i);
            Lits_Out2[0] = toLitCond(c2, 1);
            Lits_Out2[1] = toLitCond(varOut, phase);
            cid = sat_solver_addclause(pSolver, Lits_Out2, Lits_Out2 + 2);
            assert(cid);
        }  
    }
    fclose(fPla);

    for (int i=0; i<nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOri = Vec_IntEntry(pOriPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut1, varOri, 0);
        assert(cid);
    }
    for (int i=0; i<nPi; i++)
    {
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut2, varDup, 0);
        assert(cid);
    }
    // Alpha : (x=x' + alpha)
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(nPi);
    for (int i=0; i<nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_conditional_unequal(pSolver, varOri, varDup, varAlphaControl);
        assert(cid);
    }

    // Beta : (x!=x' <> beta)
    Vec_Int_t *pBetaControls = Vec_IntAlloc(nPi);
    for (int i=0; i<nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varBetaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pBetaControls, varBetaControl);
        cid = sat_solver_iff_unequal(pSolver, varOri, varDup, varBetaControl);
        assert(cid);
    }

    // V(Alpha ^ Beta)
    Vec_Int_t *pAndAlphaBeta = Vec_IntAlloc(nPi);
    for (int i=0; i<nPi; i++)
    {
        int varAlpha = Vec_IntEntry(pAlphaControls, i);
        int varBeta = Vec_IntEntry(pBetaControls, i);
        int varAnd = sat_solver_addvar(pSolver);
        Vec_IntPush(pAndAlphaBeta, varAnd);
        cid = sat_solver_add_and(pSolver, varAnd, varAlpha, varBeta, 0, 0, 0);
        assert(cid);
    }

    lit Lits_AlphaBeta[nPi];
    for (int i=0; i<nPi; i++)
    {
        int varAnd = Vec_IntEntry(pAndAlphaBeta, i);
        Lits_AlphaBeta[i] = toLitCond(varAnd, 0);
    }   
    cid = sat_solver_addclause(pSolver, Lits_AlphaBeta, Lits_AlphaBeta + nPi);
    assert(cid);

    // Gamma : (o != o' <> gamma)
    Vec_Int_t *pGammaControls = Vec_IntAlloc(nPo);
    for(int i=0; i<nPo; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Po, i);
        int varOut2 = Vec_IntEntry(pOut2Po, i);
        int varGammaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pGammaControls, varGammaControl);
        cid = sat_solver_iff_unequal(pSolver, varOut1, varOut2, varGammaControl);
        assert(cid);
    }

    // V(Gamma)
    lit Lits_Gamma[nLitPo];
    int count = 0;
    for(int i=0; i<nPo; i++)
    {
        if(litPo[i])
        {
            int var = Vec_IntEntry(pGammaControls, i);
            Lits_Gamma[count] = toLitCond(var, 0);
            count++;
        }
    }
    cid = sat_solver_addclause(pSolver, Lits_Gamma, Lits_Gamma + nLitPo);
    assert(cid);

    ///////////////////////////////////////////////////////////////////////
    // write files for external use - need further check
    if(_forqesFileName != NULL && _forqesCareFileName != NULL && _MUSFileName != NULL)
    {
        Sat_SolverWriteDimacs(pSolver, _forqesFileName, 0, 0, 1);

        ff = fopen(_forqesCareFileName, "w"); 
        fprintf(ff, "p cnf %i %i\n", sat_solver_nvars(pSolver), nPi);   
        for (int i = 0; i < nPi; i++)
            fprintf(ff, "-%i 0\n", Vec_IntEntry(pAlphaControls, i));
        fclose(ff);

        fm = fopen(_MUSFileName, "w");
        fprintf(fm, "p gcnf %i %i %i\n", sat_solver_nvars(pSolver), sat_solver_nclauses(pSolver)+nPi, nPi);
        ff = fopen(_forqesFileName, "r");
        unused = fgets(buff, 102400, ff);
        while(fgets(buff, 102400, ff))
            fprintf(fm, "{0} %s", buff);
        for (int i=0; i<nPi; i++)
            fprintf(fm, "{%i} -%i 0\n", i+1, Vec_IntEntry(pAlphaControls, i));
        fclose(fm);  
    }
    
    ///////////////////////////////////////////////////////////////////////
    // printf("Total clauses / variables / lits: %i / %i / %ld\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver), pSolver->stats.clauses_literals);
    
    // set assumption order
    for (int i=0; i<nPi; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pAlphaControls, i),1);
    
    result = sat_solver_minimize_assumptions2(pSolver, pLits, nPi, 0);
    
    // update litPi
    for(int i=0; i<nPi; i++)
    {
        litPi[i] = 0;
    }
    for(int i=0; i<result; i++)
    {
        int var = Abc_Lit2Var(pLits[i]) - Vec_IntEntry(pAlphaControls, 0);
        assert(var >= 0 && var < nPi);
        litPi[var] = 1;
    }
    
    // clean up
    sat_solver_delete(pSolver);
    Abc_NtkDelete(pNtkOri);
    Abc_NtkDelete(pNtkDup);
    Vec_IntFree(pOriPi);
    Vec_IntFree(pDupPi);
    Vec_IntFree(pAlphaControls);
    Vec_IntFree(pBetaControls);
    Vec_IntFree(pAndAlphaBeta);
    Vec_IntFree(pGammaControls);

    return result;
}

/////////////////////////////////////////////////////////
// Reencode Methods
/////////////////////////////////////////////////////////

// add fMode later.
int IcompactMgr::reencode_naive(char* reencodeplaFile, char* mapping)
{
    char* plaFile = _workingFileName;
    int nRPo;
    FILE* ff, *fReencode, *fMapping;
    char buff[102400];
    char* t;
    char * unused __attribute__((unused)); // get rid of fget warnings
    int count = 0;
    std::map<std::string, int> mapVar;

    ff = fopen(plaFile, "r"); // .i .o .type fr
    unused = fgets(buff, 102400, ff);
    unused = fgets(buff, 102400, ff);
    unused = fgets(buff, 102400, ff);
    while(fgets(buff, 102400, ff))
    {
        t = strtok(buff, " \n");
        t = strtok(NULL, " \n");
        std::string s(t);
        if(mapVar.count(s) == 0)
        {
            mapVar[s] = count;
            count++;
        }        
    }
    fclose(ff);

    nRPo = 64 - __builtin_clzll(count);
    char* one_line = new char[nRPo+1];
    one_line[nRPo] = '\0';
    int encoding;

    // header
    fReencode = fopen(reencodeplaFile, "w");
    fprintf(fReencode, ".i %i\n", _nPi);
    fprintf(fReencode, ".o %i\n", nRPo);
    fprintf(fReencode, ".type fr");

    fMapping  = fopen(mapping, "w");
    fprintf(fMapping, ".i %i\n", nRPo);
    fprintf(fMapping, ".o %i\n", _nPo);
    fprintf(fMapping, ".type fr");

    ff = fopen(plaFile, "r");
    unused = fgets(buff, 102400, ff);
    unused = fgets(buff, 102400, ff);
    unused = fgets(buff, 102400, ff);
    while(fgets(buff, 102400, ff))
    {
        t = strtok(buff, " \n");
        fprintf(fReencode, "%s ", t);
        t = strtok(NULL, " \n");
        std::string s(t);
        encoding = mapVar[s];
        for(int i=0; i<nRPo; i++)
        {
            int rbit = encoding%2;
            one_line[i] = (rbit)? '1': '0';
            encoding = encoding >> 1;
        }
        fprintf(fReencode, "%s\n", one_line);
        fprintf(fMapping, "%s %s\n", one_line, t);
    }

    // clean up
    fclose(ff);
    fclose(fReencode);
    fclose(fMapping);
    return nRPo;
}

// fMode = 1 input reencode, = 0 output reencode
int IcompactMgr::reencode_heuristic(char* reencodeplaFile, char* mapping, bool fMode, int newVar, int* record)
{
    char* plaFile = _workingFileName;
    int nRPo, lineNum;
    FILE* ff        = fopen(plaFile, "r");         // .i .o .type fr
    FILE* fReencode = fopen(reencodeplaFile, "w"); // .i .o .type fr
    FILE* fMapping  = fopen(mapping, "w");         // .i .o .type fr
    char buff[102400];
    char *t, *OCResult;
    char * unused __attribute__((unused)); // get rid of fget warnings

    ReencodeHeuristicMgr* mgr = new ReencodeHeuristicMgr();
    mgr->readFile(plaFile, fMode);
    nRPo = mgr->getEncoding(newVar);
    mgr->getMapping(fMode, record);  
    
    unused = fgets(buff, 102400, ff);
    fprintf(fReencode, "%s", buff);
    fprintf(fMapping, ".i %i\n", nRPo);
    unused = fgets(buff, 102400, ff);
    fprintf(fReencode, ".o %i\n", nRPo);
    fprintf(fMapping, "%s", buff);
    unused = fgets(buff, 102400, ff);
    fprintf(fReencode, "%s", buff);
    fprintf(fMapping, "%s", buff);
    lineNum = 0;
    while(fgets(buff, 102400, ff))
    {
        t = strtok(buff, " \n");
        fprintf(fReencode, "%s ", t);
        t = strtok(NULL, " \n");
        OCResult = mgr->getOCResult(lineNum);
        fprintf(fReencode, "%s\n", OCResult);
        fprintf(fMapping, "%s %s\n", OCResult, t);
        lineNum++;
    }

    // clean up
    fclose(ff);
    fclose(fReencode);
    fclose(fMapping);
    return nRPo;
}

ABC_NAMESPACE_IMPL_END