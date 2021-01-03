#ifndef ABC__ext__icompact_h
#define ABC__ext__icompact_h
#include "icompactHeuristic.h"
#include "reencodeHeuristic.h"
#include "base/main/main.h"
#include "sat/bsat/satSolver.h"
#include "sat/cnf/cnf.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include "base/io/ioAbc.h" // Io_ReadBlif
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

enum SolvingType {
    HEURISTIC_SINGLE = 0,
    HEURISTIC_8 = 1,
    HEURISTIC_EACH = 6, // eachPo mode, while others are fullPo mode
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

class IcompactMgr
{
public:
    IcompactMgr(Abc_Frame_t * pAbc, char *funcFileName, char *caresetFileName, char *baseFileName, char *resultlogFileName);
    ~IcompactMgr() {}
    void ocompact(int fOutput, int fNewVar);
    int icompact(SolvingType fSolving, double fRatio, int fNewVar, int fCollapse, int fMinimize, int fBatch);

private:
    bool _fIcompact, _fOcompact;
    int _oriPi, _oriPo;
    // TODO: Clarify usage of _nPi, _nPo;
    int _nPi, _nPo; // No not touch _nPi. compacted Pi/ Po. Output Compaction & Input Reencoding results are stored here
    bool *_litPi, *_litPo;
    abctime _step_time, _end_time;

    Abc_Frame_t * _pAbc;
    char *_funcFileName;
    char *_caresetFileName;
    char *_baseFileName; // base file name for multiple intermediate files
    char *_resultlogFileName;
    char* _tmpFileName = "tmp.pla";

    // intermediate files
    char _workingFileName[500];
    char _samplesplaFileName[500]; // samples of simulation
    char _reducedplaFileName[500]; // redundent inputs removed
    char _outputreencodedFileName[500];
    char _outputmappingFileName[500]; // omap
    char _inputreencodedFileName[500];
    char _inputmappingFileName[500]; // imap

    // intermediate pNtks
    Abc_Ntk_t* _pNtk_func;
    Abc_Ntk_t* _pNtk_careset;
    Abc_Ntk_t* _pNtk_samples;
    Abc_Ntk_t* _pNtk_tmp;
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
    int check_pla_pipo(char *pFileName);

    // ntk functions
    void getNtk_func(); // used in constructor
    Abc_Ntk_t * getNtk_samples(int fMinimize, int fCollapse);
    Abc_Ntk_t * getNtk_careset(int fMinimize, int fCollapse, int fBatch);
    Abc_Ntk_t * ntkMinimize(Abc_Ntk_t * pNtk, int fMinimize, int fCollapse);
//    Abc_Ntk_t * ntkBatch(int fMode, int fBatch); 

    // icompact methods - forqes / Muser2 file dump is supported in icompact_cube_direct_encode_with_c()
    int icompact_cube_heuristic(char* plaFile, int iterNum, double ratio);
    int icompact_cube_main(Abc_Ntk_t * pNtk_func, Abc_Ntk_t* pNtk_careset);
    int icompact_cube_direct_encode_with_c(char* plaFile, Abc_Ntk_t* pNtk_careset, char* forqesFileName, char* forqesCareFileName, char* MUSFileName);
    int icompact_cube_direct_encode_without_c(char* plaFile, Abc_Ntk_t* pNtk_careset);
    
    // reencode methods
    int reencode_naive(char* plaFile, char* reencodeplaFile, char* outputmapping);
    int reencode_heuristic(char* plaFile, char* reencodeplaFile, char* outputmapping, bool type, int newVar, int* record);
};

extern int careset2patterns(char* patternsFileName, char* caresetFilename, int nPi, int nPo);
extern int careset2patterns_r(char* patternsFileName, char* caresetFilename, int nPi);
extern void writeCompactpla(char* outputplaFileName, char* patternFileName, int nPi, bool* litPi, int nPo, bool* litPo, int idx);
extern int espresso_input_count(char* filename);
extern Abc_Ntk_t* get_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time);
extern Abc_Ntk_t* get_part_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time, int nPi, int nPo, int *litPo);

// two get_support functions has different po order, sort ori to match pla order.
// check get_support_pat, support sizes are greater than expexted 
extern void get_support_ori(Abc_Ntk_t* pNtk, int nPi, int nPo, int** supportInfo);
extern void get_support_pat(char* plaFile, int nPi, int nPo, int** supportInfo);

// icompactSolve.cpp
extern int sat_solver_get_minimized_assumptions(sat_solver* s, int * pLits, int nLits, int nConfLimit);

// icompactGencareset.cpp
int smlSimulateCombGiven( Abc_Ntk_t *pNtk, char * pFileName);
extern void n_gen_AP(int nPat, int nPi, int nPo, char* filename);
extern void n_gen_Random(int nPat, int nPi, int nPo, char* filename);
extern void n_gen_Cube(int nPat, int nCube, int nPi, int nPo, char* filename);

ABC_NAMESPACE_HEADER_END
#endif