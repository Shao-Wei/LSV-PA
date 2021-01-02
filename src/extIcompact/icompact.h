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

extern Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);

extern int careset2patterns(char* patternsFileName, char* caresetFilename, int nPi, int nPo);
extern int careset2patterns_r(char* patternsFileName, char* caresetFilename, int nPi);
extern void writeCompactpla(char* outputplaFileName, char* patternFileName, int nPi, bool* litPi, int nPo, bool* litPo, int idx);
extern int espresso_input_count(char* filename);
extern Abc_Ntk_t* get_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time);
extern Abc_Ntk_t* get_part_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time, int nPi, int nPo, int *litPo);

extern int icompact_cube_heuristic(char* plaFile, int iterNum, double ratio, int nPi, int nPo, bool *litPi, bool *litPo);
extern int icompact_cube_main(Abc_Ntk_t * pNtk_func, Abc_Ntk_t* pNtk_careset, int nPi, int nPo, bool *litPi, bool *litPo);
extern int icompact_cube_direct_encode_with_c(char* plaFile, Abc_Ntk_t* pNtk_careset, int nPi, int nPo, bool *litPi, bool *litPo, char* forqesFileName, char* forqesCareFileName, char* MUSFileName);
extern int icompact_cube_direct_encode_without_c(char* plaFile, Abc_Ntk_t* pNtk_careset, int nPi, int nPo, bool *litPi, bool *litPo);
extern int output_compaction_naive(char* plaFile, char* reencodeplaFile, char* outputmapping);
extern int icompact_cube_reencode(char* plaFile, char* reencodeplaFile, char* outputmapping, bool type, int newVar, int* record);

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