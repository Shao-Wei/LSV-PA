#ifndef ABC__ext__icompact_h
#define ABC__ext__icompact_h
#include "icompactHeuristic.h"
#include "reencodeHeuristic.h"
#include "base/main/main.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include "base/io/ioAbc.h" // Io_Read
#include "map/mio/mio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <ctime>
#include <errno.h>

ABC_NAMESPACE_HEADER_START
///////////////////////////
// Commit log
///////////////////////////
// STF bug fix (case buffer node, reverse topo cand)
// STF intergrated into compact

///////////////////////////
// Todo
///////////////////////////
// (icompactGencareset) n_gen_Random checkpattern removal
// pi/po names handling after compaction
// support handling after compaction
// reencode methods should return -1 when fail
// icompact heuristic scheme add
// check unvalid char when reading pla 
// handle constant po where compaction result returns 0
// _patternMgr may have wrong working file when used multiple times
// icompact heuristic does not reset patter between each iter
// ocompact heuristic may have '-' in encoding, which is not valid for icompact pla format
// ocompact heuristic use po signals with small supports
// omap construction checking fail at pntk to dar

// base/abci/abcStrash.c
extern "C" { Abc_Ntk_t * Abc_NtkPutOnTop( Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtk2 ); }

/*== base/abci/abcDar.c ==*/
extern Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);
extern Abc_Ntk_t *Abc_NtkFromDar( Abc_Ntk_t * pNtkOld, Aig_Man_t * pMan );

enum SolvingType {
    HEURISTIC = 0,
    HEURISTIC_EACH = 1, // eachPo mode, while others are fullPo mode
    LEXSAT_CLASSIC = 2,
    LEXSAT_ENCODE_C = 3,
    REENCODE = 4,
    NONE = 5  
};

enum MinimizeType {
    NOMIN = 0,
    BASIC = 1,
    STRONG = 2
};

enum ERRORTYPE {
    NOERROR = 0,
    BADSAMPLES = 1, // bad samples file
    BADEVAL = 2, // bad func file
    ICOMPACT_FAIL = 3,  // icompact failure
    FUNC_NAME_NOT_CONSISTANT = 4, // pi/po names not consistant between func & samples
    CONSTRUCT_NTK_FAIL = 5,
    NTK_FAIL_SIMULATION = 6
};

class IcompactMgr
{
public:
    IcompactMgr(Abc_Frame_t * pAbc, char *caresetFileName, char *baseFileName, char *funcFileName, char *resultlogFileName);
    ~IcompactMgr();

    // main functions
    int performExp(int fExperiment);
    int ocompact(int fOutput, int fNewVar);
    int icompact(SolvingType fSolving, double fRatio, int fNewVar, int fCollapse, int fMinimize, int fBatch, int fIter, int fSupport);

    // get ntk functions
    Abc_Ntk_t * getNtkImap(); // later
    Abc_Ntk_t * getNtkCore(); // later
    Abc_Ntk_t * getNtkOmap(); // later

    // support functions
    void getSupportInfoFunc(); // print function. later
    void getSupportInfoPatt(); // print function. later

    // each cone functions
    void getEachConeFunc(); // later
    void getEachConePatt(); // later

private:
    // flags
    enum ERRORTYPE _fMgr;
    bool _fIcompact, _fOcompact; // flag set true after input/output compaction
    bool _fEval; // flag set true if _pNtk_func is built for eval
    bool _fSupportFunc, _fSupportPatt; // flag set true after support set is built
    
    ICompactHeuristicMgr * _patternMgr;

    // working
    char _workingFileName[500];
    bool *_litWorkingPi, *_litWorkingPo;
    int _workingPi, _workingPo; 

    // basic info
    int _nPi, _nPo;
    char **_piNames, **_poNames; // get from samples file
    bool *_litPi, *_litPo;
    int _nRPi, _nRPo; // Output Compaction & Input Reencoding results
    char **_rpiNames, **_rpoNames;
    bool *_litRPi, *_litRPo;
    int _oriPatCount, _uniquePatCount;

    // timing
    abctime _step_time, _end_time;

    // support set
    int ** _supportInfo_func; // [i][j] if po i has support pi j
    int ** _supportInfo_patt; // [i][j] if po i has support pi j
    void supportInfo_Func();
    void supportInfo_Patt();

    // arguements
    Abc_Frame_t * _pAbc;
    char *_funcFileName;
    char *_caresetFileName;
    char *_baseFileName; // base file name for multiple intermediate files
    char *_resultlogFileName;
    char* _tmpFileName = "tmp.pla";

    // intermediate files
    char _samplesplaFileName[500]; // samples of simulation
    char _reducedplaFileName[500]; // redundent inputs removed
    char _outputreencodedFileName[500];
    char _outputmappingFileName[500]; // omap
    char _inputreencodedFileName[500];
    char _inputmappingFileName[500]; // imap

    // intermediate pNtks
    Abc_Ntk_t* _pNtk_func; // only used for evaluation
    Abc_Ntk_t* _pNtk_careset;
    Abc_Ntk_t* _pNtk_samples;
    Abc_Ntk_t* _pNtk_imap;
    Abc_Ntk_t* _pNtk_core;
    Abc_Ntk_t* _pNtk_omap;

    // external tool use
    char _forqesFileName[500];
    char _forqesCareFileName[500];
    char _MUSFileName[500];
    
    // sizing & timing  
    double _time_icompact, _time_ireencode, _time_oreencode; // runtime for each operation 
    int _gate_imap, _gate_core, _gate_omap, _gate_batch; // aig node count for each sub-circuit
    double _time_imap, _time_core, _time_omap, _time_batch; // minimization time for each sub-circuit

    // aux functions
    bool mgrStatus();
    void resetWorkingLitPi();
    void resetWorkingLitPo();
    bool validWorkingLitPi(); 
    bool validWorkingLitPo();
    void getInfoFromSamples();
    void orderPiPo(Abc_Ntk_t * pNtk);

    // ntk functions
    Abc_Ntk_t * getNtk_func();
    Abc_Ntk_t * getNtk_samples(int fMinimize, int fCollapse);
    Abc_Ntk_t * getNtk_careset(int fMinimize, int fCollapse, int fBatch);
    Abc_Ntk_t * ntkMinimize(Abc_Ntk_t * pNtk, int fMinimize, int fCollapse);
    Abc_Ntk_t * ntkMfs(Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkCare);
    Abc_Ntk_t * ntkFraig(Abc_Ntk_t * pNtk, Abc_Ntk_t * pNtkExdc);
//    Abc_Ntk_t * ntkBatch(int fMode, int fBatch); 
    void writeCompactpla(char* outputplaFileName);
    void writeCaresetpla(char* outputplaFileName);
    Abc_Ntk_t * constructNtkEach(bool **minMaskList, int fMfs, int fFraig, int fSTF);
    Abc_Ntk_t * constructNtkOmap(int * recordPo, int fMfs, int fFraig, int fSTF);

    // icompact methods - forqes / Muser2 file dump is supported in icompact_cube_direct_encode_with_c()
    int icompact_heuristic(int iterNum, double fRatio, int fSupport);
    bool** icompact_heuristic_each(int iterNum, double fRatio, int fSupport); // non-single result, compaction matrix returned 
    int icompact_main();
    int icompact_direct_encode_with_c();

    // reencode methods
    int reencode_naive(char* reencodeplaFile, char* mapping);
    int reencode_heuristic(char* reencodeplaFile, char* mapping, bool type, int newVar, int* record);

    // experiments
    int exp_support_icompact_heuristic();
    int exp_support_icompact_heuristic_options();
    int exp_omap_construction();
};

// icompactSolve.cpp
extern int sat_solver_get_minimized_assumptions(sat_solver* s, int * pLits, int nLits, int nConfLimit);

// icompactGencareset.cpp
int smlSimulateCombGiven( Abc_Ntk_t *pNtk, char * pFileName);
int smlVerifyCombGiven( Aig_Man_t * pAig, char * pFileName, int * pCount, int fVerbose);
int smlSTFaultCandidate( Aig_Man_t * pAig, char * pFileName, vector< pair<int, int> >& vCandidate);
int careset2patterns(char* patternsFileName, char* caresetFilename, int nPi, int nPo);
void n_gen_AP(int nPat, int nPi, int nPo, char* filename);
void n_gen_Random(int nPat, int nPi, int nPo, char* filename);
void n_gen_Cube(int nPat, int nCube, int nPi, int nPo, char* filename);

// icompactUtil.cpp
extern int espresso_input_count(char* filename);
extern int check_pla_pipo(char *pFileName, int nPi, int nPo);
void orderPiPo(Abc_Ntk_t * pNtk, int nPi, int nPo);
int ntkVerifySamples(Abc_Ntk_t* pNtk, char *pFile, int fVerbose);
int ntkAppend( Abc_Ntk_t * pNtk1, Abc_Ntk_t * pNtk2);
char ** setDummyNames(int len, char * baseStr);
bool singleSupportComplement(char * pFileName, int piIdx, int poIdx); // check if two bits are complement
Abc_Ntk_t * ntkSTFault(Abc_Ntk_t * pNtk, char * simFileName); // modifies src/aig/aig/aigTable.c Aig_TableLookUp() to avoid assertion fail

ABC_NAMESPACE_HEADER_END
#endif