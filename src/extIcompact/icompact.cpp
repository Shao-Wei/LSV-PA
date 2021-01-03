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

int careset2patterns(char* patternsFileName, char* caresetFilename, int nPi, int nPo)
{
    FILE *fcareset = fopen(caresetFilename, "r");
    FILE *fpatterns = fopen(patternsFileName, "w");
    char buff[102400];
    int minterm_total;
    int enum_count;
    int local_count;
    char* literals = new char[nPi];
    char* one_line = new char[nPi + nPo + 2];
    one_line[nPi+nPo+1] = '\0';
    one_line[nPi] = ' ';
    for(int i=nPi+1; i<nPi+nPo+1; i++)
        one_line[i] = '0';

    fgets(buff, 102400, fcareset); // no header needed for simulation file
    fgets(buff, 102400, fcareset);
    while(fgets(buff, 102400, fcareset))
    {
        for(int i=0;i<nPi; i++)
            literals[i] = 1;
        
        enum_count = 0;
        for(int i=0; i<nPi; i++)
        {
            if(buff[i]!='1' && buff[i]!='0')
            {
                literals[i] = 0;
                enum_count++;
            }
        }
        
        // printf("%s : %i\n", buff, enum_count); 
        if(enum_count > 20)
            return 1;
        else if(enum_count != 0) 
            minterm_total = (int)pow(2, enum_count);
        else
            minterm_total = 1;

        if(enum_count == 0)
        {
            for(int i=0; i<nPi; i++)
                one_line[i] = buff[i];
            fprintf(fpatterns, "%s\n", one_line);
        }
        else
        {
            while(minterm_total > 0)
            {
                local_count = enum_count-1;
                
                minterm_total = minterm_total - 1;
                for(int i=0; i<nPi; i++)
                {
                    
                    if(literals[i])
                        one_line[i] = buff[i];
                    else
                    {
                        one_line[i] = ((minterm_total >> local_count)%2)? '1': '0';
                        local_count = local_count-1;
                    }     
                    
                }
                fprintf(fpatterns, "%s\n", one_line);
            }
        }
    }

    fclose(fcareset);
    fclose(fpatterns);
    delete [] one_line;
    return 0;
}

void writeCompactpla(char* outputplaFileName, char* patternFileName, int nPi, bool* litPi, int nPo, bool* litPo, int idx)
{
    FILE* fcompactpla = fopen(outputplaFileName, "w");
    FILE* fpattern = fopen(patternFileName, "r");
    char buff[102400];
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

    fgets(buff, 102400, fpattern);
    fgets(buff, 102400, fpattern);
    fgets(buff, 102400, fpattern);
    fprintf(fcompactpla, ".i %i\n", lenPi);
    fprintf(fcompactpla, ".o %i\n", lenPo);
    if(lenPo == 1) // modify later
        fprintf(fcompactpla, ".ob z%i\n", idx);
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

int output_compaction_naive(char* plaFile, char* reencodeplaFile, char* outputmapping)
{
    int nRPo;
    FILE* ff        = fopen(plaFile, "r");         // .i .o .type fr
    FILE* fReencode = fopen(reencodeplaFile, "w"); // .i .o .type fr
    FILE* fMapping  = fopen(outputmapping, "w");   // .i .o .type fr
    char buff[102400];
    char* t;
    int count = 0;
    std::map<std::string, int> mapping;

    fgets(buff, 102400, ff);
    fgets(buff, 102400, ff);
    fgets(buff, 102400, ff);
    while(fgets(buff, 102400, ff))
    {
        t = strtok(buff, " \n");
        t = strtok(NULL, " \n");
        std::string s(t);
        if(mapping.count(s) == 0)
        {
            mapping[s] = count;
            count++;
        }        
    }
    fclose(ff);

    nRPo = 64 - __builtin_clzll(count);
    char* one_line = new char[nRPo+1];
    one_line[nRPo] = '\0';
    int encoding;
    ff = fopen(plaFile, "r");
    fgets(buff, 102400, ff);
    fprintf(fReencode, "%s", buff);
    fprintf(fMapping, ".i %i\n", nRPo);
    fgets(buff, 102400, ff);
    fprintf(fReencode, ".o %i\n", nRPo);
    fprintf(fMapping, "%s", buff);
    fgets(buff, 102400, ff);
    fprintf(fReencode, "%s", buff);
    fprintf(fMapping, "%s", buff);
    while(fgets(buff, 102400, ff))
    {
        t = strtok(buff, " \n");
        fprintf(fReencode, "%s ", t);
        t = strtok(NULL, " \n");
        std::string s(t);
        encoding = mapping[s];
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

int aux_genReducedPla(int nPi, int poIdx, char* originalFilename, char* outputFilename)
{
    char buff[102400];
    FILE *fcare = fopen(outputFilename, "w");
    FILE *foriginal = fopen(originalFilename, "r");
    char *one_line = new char[nPi + 3];
    one_line[nPi + 2] = '\0';
    one_line[nPi    ] = '|';
    //header
    fgets(buff, 102400, foriginal);
    fgets(buff, 102400, foriginal);
    fgets(buff, 102400, foriginal);

    fprintf(fcare, ".i %i\n", nPi);
    fprintf(fcare, ".o 1\n");
    fprintf(fcare, ".ob o%i\n", poIdx);
    fprintf(fcare, ".type fdr\n");

    // body
    while(fgets(buff, 102400, foriginal))
    {
        for(int i=0; i<nPi; i++)
        {
            one_line[i] = buff[i];
        }
        one_line[nPi + 1] = buff[nPi + 1 + poIdx];
        fprintf(fcare, "%s\n", one_line);
    }

    fclose(fcare);
    fclose(foriginal);
    delete [] one_line;
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

Abc_Ntk_t* get_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time)
{
    int check;
    abctime step_time, end_time;
    Abc_Ntk_t *pNtk = NULL;
    Abc_Ntk_t *pNtk_part = NULL;
    char *tmpplaname = "tmp.pla";
    char Command[2000];
    char minimizeCommand[1000] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";

    step_time = Abc_Clock();
    sprintf( Command, "read %s; strash", plaFile);
    if(Cmd_CommandExecute(pAbc,Command))
    {
        printf("Cannot read %s\n", plaFile);
        return NULL;
    }
    if(Cmd_CommandExecute(pAbc,minimizeCommand))
    {
        printf("Minimize %s. Failed.\n", plaFile);
        return NULL;
    }
    pNtk = Abc_FrameReadNtk(pAbc);
    end_time = Abc_Clock();
    
    gate = Abc_NtkNodeNum(pNtk);
    time = 1.0*((double)(end_time - step_time))/((double)CLOCKS_PER_SEC);
    printf("%s: size = %i; time = %f\n",log, gate, time);
    return pNtk;
}

Abc_Ntk_t* get_part_ntk(Abc_Frame_t_ * pAbc, char* plaFile, char* log, int& gate, double& time, int nPi, int nPo, int *litPo)
{
    int check;
    abctime step_time, end_time;
    Abc_Ntk_t *pNtk = NULL;
    Abc_Ntk_t *pNtk_part = NULL;
    char *tmpplaname = "tmp.pla";
    char Command[2000];
    char minimizeCommand[1000] = "strash; dc2; balance -l; resub -K 6 -l; rewrite -l; \
                                  resub -K 6 -N 2 -l; refactor -l; resub -K 8 -l; balance -l; \
                                  resub -K 8 -N 2 -l; rewrite -l; resub -K 10 -l; rewrite -z -l; \
                                  resub -K 10 -N 2 -l; balance -l; resub -K 12 -l; \
                                  refactor -z -l; resub -K 12 -N 2 -l; rewrite -z -l; balance -l; strash";
    
    step_time = Abc_Clock();
    for(int i=0; i<nPo; i++)
    {
        if(litPo[i] == -1)
        {
            check = aux_genReducedPla(nPi, i, plaFile, tmpplaname); 
            assert(check == 0);
            sprintf( Command, "read %s; strash", tmpplaname);
            if(Cmd_CommandExecute(pAbc,Command))
            {
                printf("Cannot read %s\n", tmpplaname);
                return NULL;
            }
            if(Cmd_CommandExecute(pAbc,minimizeCommand))
            {
                printf("Minimize %s. Failed.\n", tmpplaname);
                return NULL;
            }
            pNtk_part = Abc_FrameReadNtk(pAbc);
            if(pNtk == NULL)
                pNtk = Abc_NtkDup(pNtk_part);
            else
                Abc_NtkAppend(pNtk, pNtk_part, 1);
        }
        else
        {
            // TODO
            // add simple input - output
            // Abc_NtkDupObj, Abc_NtkStartFrom
        }
    } 
    end_time = Abc_Clock();
    
    gate = Abc_NtkNodeNum(pNtk);
    time = 1.0*((double)(end_time - step_time))/((double)CLOCKS_PER_SEC);
    printf("%s: size = %i; time = %f\n",log, gate, time);
    return pNtk;
}

/////////////////////////////////////////////////////////
// Aux functions
/////////////////////////////////////////////////////////
void IcompactMgr::setFilenames(char *baseFileName)
{
    sprintf(_samplesplaFileName,      "%s.samples.pla",   baseFileName);
    sprintf(_reducedplaFileName,      "%s.reduced.pla",   baseFileName);
    sprintf(_outputreencodedFileName, "%s.oreencode.pla", baseFileName);
    sprintf(_outputmappingFileName,   "%s.omap.pla",      baseFileName);
    sprintf(_inputreencodedFileName,  "%s.ireencode.pla", baseFileName);
    sprintf(_inputmappingFileName,    "%s.imap.pla",      baseFileName);
    sprintf(_forqesFileName,          "%s.full.dimacs",   baseFileName);
    sprintf(_forqesCareFileName,      "%s.care.dimacs",   baseFileName);
    sprintf(_MUSFileName,             "%s.gcnf",          baseFileName);
}

void get_support_ori(Abc_Ntk_t* pNtk, int nPi, int nPo, int** supportInfo)
{
    assert(Abc_NtkCiNum(pNtk) == nPi);
    assert(Abc_NtkCoNum(pNtk) == nPo);

    Abc_Obj_t* pPo;
    Vec_Int_t* vecSup;
    int i;
    Abc_NtkForEachPo(pNtk, pPo, i)
    {
        vecSup = Abc_NtkNodeSupportInt(pNtk, i);
        for(int j=0, n = Vec_IntSize(vecSup); j<n; j++)
            supportInfo[i][Vec_IntEntry(vecSup, j)] = 1;
    }
}

void get_support_pat(char* plaFile, int nPi, int nPo, int** supportInfo)
{
    int result;
    bool* litPi = new bool[nPi];
    bool* litPo = new bool[nPo];
    for(int i=0; i<nPo; i++)
    {
        for(int j=0; j<nPo; j++)
            litPo[j] = 0;
        litPo[i] = 1;
        result = icompact_cube_heuristic(plaFile, 8, 0, nPi, nPo, litPi, litPo);
        for(int j=0; j<nPi; j++)
            supportInfo[i][j] = (litPi[j])? 1: 0;
    }
}

/////////////////////////////////////////////////////////
// Input compact methods
/////////////////////////////////////////////////////////
int IcompactMgr::icompact_cube(SolvingType fSolving, double fRatio, int fNewVar)
{
    int result;
    char buff[102400];
    char* t;

    // initial checkings
    int nLitPo = 0;
    for(int i=0; i<_nPo; i++)
        if(_litPo[i]) { nLitPo++; }
    if(nLitPo == 0) { printf("icompact_cube: Cared output size zero: *litPo must be specified\n"); return -1; }

    FILE* fPla = fopen(_workingFileName, "r");
    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    assert(atoi(t) == _nPi);
    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    assert(atoi(t) == _nPo);
    fclose(fPla);

    if(fSolving == HEURISTIC_SINGLE)
    {
        _step_time = Abc_Clock();
        result = icompact_cube_heuristic(_workingFileName, 1, fRatio);
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName, _workingFileName, _nPi, _litPi, workingPo, _litPo, 0);
    }
    else if(fSolving == HEURISTIC_8)
    {
        _step_time = Abc_Clock();
        result = icompact_cube_heuristic(_workingFileName, 8, fRatio);
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName, _workingFileName, _nPi, _litPi, workingPo, _litPo, 0);
    }
    else if(fSolving == LEXSAT_CLASSIC)
    {
        _step_time = Abc_Clock();
        result = icompact_cube_main(pNtk_sample, pNtk_careset);
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName, _workingFileName, _nPi, _litPi, workingPo, _litPo, 0);     
    }
    else if(fSolving == LEXSAT_ENCODE_C)
    {        
        _step_time = Abc_Clock();
        result = icompact_cube_direct_encode_with_c(_workingFileName, pNtk_careset, _forqesFileName, _forqesCareFileName, _MUSFileName);
        _end_time = Abc_Clock();
        writeCompactpla(_reducedplaFileName, _workingFileName, _nPi, _litPi, workingPo, _litPo, 0);
    }
    else if(fSolving == REENCODE)
    {
        _step_time = Abc_Clock();
        recordPi = new int[_nPi];
        result = icompact_cube_reencode(_workingFileName, inputmappingFileName, inputreencodedFileName, 1, fNewVar, recordPi);
        _end_time = Abc_Clock();
        _time_ireencode = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        printf("reencode computation time: %9.2f\n", _time_ireencode);
        printf("Input compacted to %i / %i\n", result, _nPi);
    }
    else if(fSolving == HEURISTIC_EACH)
    {
        _step_time = Abc_Clock();
        for(int i=0; i<workingPo; i++)
        {
            for(int k=0; k<workingPo; k++)
                _litPo[k] = 0;
            _litPo[i] = 1;

            result = icompact_cube_heuristic(_workingFileName, 1, fRatio);
            assert(result > 0);
            writeCompactpla(tmpFileName, _workingFileName, _nPi, _litPi, workingPo, _litPo, i);
            sprintf( Command, "read %s; strash", tmpFileName);
            if(Cmd_CommandExecute(pAbc,Command))
            {
                printf("Cannot read %s\n", tmpFileName);
                return NULL;
            }
            if(Cmd_CommandExecute(pAbc,minimizeCommand))
            {
                printf("Minimize %s. Failed.\n", tmpFileName);
                return NULL;
            }
            pNtk_tmp = Abc_FrameReadNtk(pAbc);
            if(pNtk_core == NULL)
                pNtk_core = Abc_NtkDup(pNtk_tmp);
            else
                Abc_NtkAppend(pNtk_core, pNtk_tmp, 1);
        }
        _end_time = Abc_Clock();
        Abc_FrameSetCurrentNetwork(pAbc, pNtk_core);
        if(Cmd_CommandExecute(pAbc,minimizeCommand))
        {
            printf("Minimize %s. Failed.\n", tmpFileName);
            return NULL;
        }
        pNtk_core = Abc_FrameReadNtk(pAbc);
        _time_core = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        _gate_core = Abc_NtkNodeNum(pNtk_core);
        printf("time: %9.2f; gate: %i\n", _time_core, _gate_core);
    }
    
    if(fSolving != (REENCODE || HEURISTIC_EACH))
    {
        _time_icompact = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
        printf("==========================\n");
        printf("icompact computation time: %9.2f\n", _time_icompact);
        printf("icompact result: %i / %i\n", result, _nPi);
        for(int i=0; i<_nPi; i++)
            if(_litPi[i])
                printf("%i ", i);
        printf("\n");
    }

    return 0;
}

/*== base/abci/abcDar.c ==*/
extern Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters);

/*== Slightly modified. sat/bsat/satSolver.h ==*/
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

int IcompactMgr::icompact_cube_main( Abc_Ntk_t * pNtk_func, Abc_Ntk_t * pNtk_careset)
{
    int result = _nPi;
    int pLits[_nPi]; // satSolver assumption
    int nLitPo = 0;
    for(int i=0; i<_nPo; i++)
        if(_litPo[i]) { nLitPo++; }    

    ///////////////////////////////////////////////////////////////////////
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    Abc_Ntk_t *pNtkOri = Abc_NtkDup(pNtk_careset);
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtkOri);

    Vec_Int_t *pOriPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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

    Vec_Int_t *pDupPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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

    Vec_Int_t *pOut1Pi = Vec_IntAlloc(_nPi);
    Vec_Int_t *pOut1Po = Vec_IntAlloc(_nPo);
    {
        Aig_Man_t *pAigOut1 = Abc_NtkToDar(pNtkOut1, 0, 0);
        Cnf_Dat_t *pCnfOut1 = Cnf_Derive(pAigOut1, Abc_NtkCoNum(pNtkOut1));
        Cnf_DataLift(pCnfOut1, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut1);
        // store var of pi
        for (int i=0; i<_nPi; i++)
        {
            Aig_Obj_t *pAigObjOut1 = Aig_ManCi(pCnfOut1->pMan, i);
            int var = pCnfOut1->pVarNums[Aig_ObjId(pAigObjOut1)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut1Pi, var);
        }
        // store var of po
        for (int i=0; i<_nPo; i++)
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

    for (int i=0; i<_nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOri = Vec_IntEntry(pOriPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut1, varOri, 0);
        assert(cid);
    }

    Vec_Int_t *pOut2Pi = Vec_IntAlloc(_nPi);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(_nPo);
    {
        Aig_Man_t *pAigOut2 = Abc_NtkToDar(pNtkOut2, 0, 0);
        Cnf_Dat_t *pCnfOut2 = Cnf_Derive(pAigOut2, Abc_NtkCoNum(pNtkOut2));
        Cnf_DataLift(pCnfOut2, sat_solver_nvars(pSolver));
        sat_solver_addclause_from(pSolver, pCnfOut2);
        // store var of pi
        for (int i=0; i<_nPi; i++)
        {
            Aig_Obj_t *pAigObjOut2 = Aig_ManCi(pCnfOut2->pMan, i);
            int var = pCnfOut2->pVarNums[Aig_ObjId(pAigObjOut2)];
            // Abc_Print(1, "var of pi %d: %d\n", i, var);
            Vec_IntPush(pOut2Pi, var);
        }
        // store var of po
        for (int i=0; i<_nPo; i++)
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

    for (int i=0; i<_nPi; i++)
    {
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut2, varDup, 0);
        assert(cid);
    }
    // printf("function clauses / variables: %i / %i\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver));
    // Alpha : (x=x' + alpha)
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_conditional_unequal(pSolver, varOri, varDup, varAlphaControl);
        assert(cid);
    }

    // Beta : (x!=x' <> beta)
    Vec_Int_t *pBetaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varBetaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pBetaControls, varBetaControl);
        cid = sat_solver_iff_unequal(pSolver, varOri, varDup, varBetaControl);
        assert(cid);
    }

    // V(Alpha ^ Beta)
    Vec_Int_t *pAndAlphaBeta = Vec_IntAlloc(_nPi);
    for (int i = 0; i < _nPi; i++)
    {
        int varAlpha = Vec_IntEntry(pAlphaControls, i);
        int varBeta = Vec_IntEntry(pBetaControls, i);
        int varAnd = sat_solver_addvar(pSolver);
        Vec_IntPush(pAndAlphaBeta, varAnd);
        cid = sat_solver_add_and(pSolver, varAnd, varAlpha, varBeta, 0, 0, 0);
        assert(cid);
    }

    lit Lits_AlphaBeta[_nPi];
    for (int i=0; i<_nPi; i++)
    {
        int varAnd = Vec_IntEntry(pAndAlphaBeta, i);
        Lits_AlphaBeta[i] = toLitCond(varAnd, 0);
    }   
    cid = sat_solver_addclause(pSolver, Lits_AlphaBeta, Lits_AlphaBeta + _nPi);
    assert(cid);

    // Gamma : (o != o' <> gamma)
    Vec_Int_t *pGammaControls = Vec_IntAlloc(_nPo);
    for(int i=0; i<_nPo; i++)
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
    for(int i=0; i<_nPo; i++)
    {
        if(_litPo[i])
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
    for (int i=0; i<_nPi; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pAlphaControls, i),1);
    result = sat_solver_minimize_assumptions2(pSolver, pLits, _nPi, 0);

    // update litPi
    for(int i=0; i<_nPi; i++)
        _litPi[i] = 0;
    for(int i=0; i<result; i++)
    {
        int var = Abc_Lit2Var(pLits[i]) - Vec_IntEntry(pAlphaControls, 0);
        assert(var >= 0 && var < _nPi);
        _litPi[var] = 1;
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

int IcompactMgr::icompact_cube_direct_encode_with_c(char* plaFile, Abc_Ntk_t* pNtk_careset, char* forqesFileName, char* forqesCareFileName, char* MUSFileName)
{
    int result = _nPi;
    int pLits[_nPi];
    char buff[102400];
    char* t;
    FILE *ff, *fm, *fPla; // file Forqes, file Muser2, file working pla  
    int nLitPo = 0;
    for(int i=0; i<_nPo; i++)
        if(_litPo[i]) { nLitPo++; }
    
    fPla = fopen(plaFile, "r");
    fgets(buff, 102400, fPla);
    fgets(buff, 102400, fPla);
    fgets(buff, 102400, fPla);
    ///////////////////////////////////////////////////////////////////////
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    Abc_Ntk_t *pNtkOri = Abc_NtkDup(pNtk_careset);
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtkOri);

    Vec_Int_t *pOriPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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

    Vec_Int_t *pDupPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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
    Vec_Int_t *pOut1Pi = Vec_IntAlloc(_nPi);
    Vec_Int_t *pOut2Pi = Vec_IntAlloc(_nPi);
    {
        for(int i=0; i<_nPi; i++)
        {
            int newVar1 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut1Pi, newVar1);
            int newVar2 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut2Pi, newVar2);
        }
    }
    Vec_Int_t *pOut1Po = Vec_IntAlloc(_nPo);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(_nPo);
    {
        for(int i=0; i<_nPo; i++)
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
        lit Lits_Pattern1[_nPi+1];
        int c1 = sat_solver_addvar(pSolver);
        for(int i=0; i<_nPi; i++)
        {
            int phase = (buff[i] == '1')? 1: 0;
            int varIn = Vec_IntGetEntry(pOut1Pi, i);
            Lits_Pattern1[i] = toLitCond(varIn, phase);
        }
        Lits_Pattern1[_nPi] = toLitCond(c1, 0);
        cid = sat_solver_addclause(pSolver, Lits_Pattern1, Lits_Pattern1 + _nPi + 1);
        assert(cid);

        for(int i=0; i<_nPo; i++)
        {
            lit Lits_Out1[2];
            int phase = (buff[_nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut1Po, i);
            Lits_Out1[0] = toLitCond(c1, 1);
            Lits_Out1[1] = toLitCond(varOut, phase);
            cid = sat_solver_addclause(pSolver, Lits_Out1, Lits_Out1 + 2);
            assert(cid);
        }

        lit Lits_Pattern2[_nPi+1];
        int c2 = sat_solver_addvar(pSolver);
        for(int i=0; i<_nPi; i++)
        {
            int phase = (buff[i] == '1')? 1: 0;
            int varIn = Vec_IntGetEntry(pOut2Pi, i);
            Lits_Pattern2[i] = toLitCond(varIn, phase);
        }
        Lits_Pattern2[_nPi] = toLitCond(c2, 0);
        cid = sat_solver_addclause(pSolver, Lits_Pattern2, Lits_Pattern2 + _nPi + 1);
        assert(cid);

        for(int i=0; i<_nPo; i++)
        {
            lit Lits_Out2[2];
            int phase = (buff[_nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut2Po, i);
            Lits_Out2[0] = toLitCond(c2, 1);
            Lits_Out2[1] = toLitCond(varOut, phase);
            cid = sat_solver_addclause(pSolver, Lits_Out2, Lits_Out2 + 2);
            assert(cid);
        }  
    }
    fclose(fPla);

    for (int i=0; i<_nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOri = Vec_IntEntry(pOriPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut1, varOri, 0);
        assert(cid);
    }
    for (int i=0; i<_nPi; i++)
    {
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut2, varDup, 0);
        assert(cid);
    }
    // Alpha : (x=x' + alpha)
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_conditional_unequal(pSolver, varOri, varDup, varAlphaControl);
        assert(cid);
    }

    // Beta : (x!=x' <> beta)
    Vec_Int_t *pBetaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varBetaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pBetaControls, varBetaControl);
        cid = sat_solver_iff_unequal(pSolver, varOri, varDup, varBetaControl);
        assert(cid);
    }

    // V(Alpha ^ Beta)
    Vec_Int_t *pAndAlphaBeta = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varAlpha = Vec_IntEntry(pAlphaControls, i);
        int varBeta = Vec_IntEntry(pBetaControls, i);
        int varAnd = sat_solver_addvar(pSolver);
        Vec_IntPush(pAndAlphaBeta, varAnd);
        cid = sat_solver_add_and(pSolver, varAnd, varAlpha, varBeta, 0, 0, 0);
        assert(cid);
    }

    lit Lits_AlphaBeta[_nPi];
    for (int i=0; i<_nPi; i++)
    {
        int varAnd = Vec_IntEntry(pAndAlphaBeta, i);
        Lits_AlphaBeta[i] = toLitCond(varAnd, 0);
    }   
    cid = sat_solver_addclause(pSolver, Lits_AlphaBeta, Lits_AlphaBeta + _nPi);
    assert(cid);

    // Gamma : (o != o' <> gamma)
    Vec_Int_t *pGammaControls = Vec_IntAlloc(_nPo);
    for(int i=0; i<_nPo; i++)
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
    for(int i=0; i<_nPo; i++)
    {
        if(_litPo[i])
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
    if(forqesFileName != NULL && forqesCareFileName != NULL && MUSFileName != NULL)
    {
        Sat_SolverWriteDimacs(pSolver, forqesFileName, 0, 0, 1);

        ff = fopen(forqesCareFileName, "w"); 
        fprintf(ff, "p cnf %i %i\n", sat_solver_nvars(pSolver), _nPi);   
        for (int i = 0; i < _nPi; i++)
            fprintf(ff, "-%i 0\n", Vec_IntEntry(pAlphaControls, i));
        fclose(ff);

        fm = fopen(MUSFileName, "w");
        fprintf(fm, "p gcnf %i %i %i\n", sat_solver_nvars(pSolver), sat_solver_nclauses(pSolver)+_nPi, _nPi);
        ff = fopen(forqesFileName, "r");
        fgets(buff, 102400, ff);
        while(fgets(buff, 102400, ff))
            fprintf(fm, "{0} %s", buff);
        for (int i=0; i<_nPi; i++)
            fprintf(fm, "{%i} -%i 0\n", i+1, Vec_IntEntry(pAlphaControls, i));
        fclose(fm);  
    }
    
    ///////////////////////////////////////////////////////////////////////
    // printf("Total clauses / variables / lits: %i / %i / %ld\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver), pSolver->stats.clauses_literals);
    
    // set assumption order
    for (int i=0; i<_nPi; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pAlphaControls, i),1);
    
    result = sat_solver_minimize_assumptions2(pSolver, pLits, _nPi, 0);
    
    // update litPi
    for(int i=0; i<_nPi; i++)
    {
        _litPi[i] = 0;
    }
    for(int i=0; i<result; i++)
    {
        int var = Abc_Lit2Var(pLits[i]) - Vec_IntEntry(pAlphaControls, 0);
        assert(var >= 0 && var < _nPi);
        _litPi[var] = 1;
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

int IcompactMgr::icompact_cube_direct_encode_without_c(char* plaFile, Abc_Ntk_t* pNtk_careset)
{
    int result = _nPi;
    int pLits[_nPi];
    char buff[102400];
    char* t;
    FILE *fPla;  // file working pla  
    int nLitPo = 0;
    for(int i=0; i<_nPo; i++)
        if(_litPo[i]) { nLitPo++; }
    
    fPla = fopen(plaFile, "r");
    fgets(buff, 102400, fPla);
    fgets(buff, 102400, fPla);
    fgets(buff, 102400, fPla);
    ///////////////////////////////////////////////////////////////////////
    // start solver
    sat_solver *pSolver = sat_solver_new();
    int cid;

    Abc_Ntk_t *pNtkOri = Abc_NtkDup(pNtk_careset);
    Abc_Ntk_t *pNtkDup = Abc_NtkDup(pNtkOri);

    Vec_Int_t *pOriPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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

    Vec_Int_t *pDupPi = Vec_IntAlloc(_nPi);
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
        for (int i=0; i<_nPi; i++)
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
    // printf("Careset clauses / variables / lits: %i / %i / %ld\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver), pSolver->stats.clauses_literals);
    Vec_Int_t *pOut1Pi = Vec_IntAlloc(_nPi);
    Vec_Int_t *pOut2Pi = Vec_IntAlloc(_nPi);
    {
        for(int i=0; i<_nPi; i++)
        {
            int newVar1 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut1Pi, newVar1);
            int newVar2 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut2Pi, newVar2);
        }
    }
    Vec_Int_t *pOut1Po = Vec_IntAlloc(_nPo);
    Vec_Int_t *pOut2Po = Vec_IntAlloc(_nPo);
    {
        for(int i=0; i<_nPo; i++)
        {
            int newVar1 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut1Po, newVar1);
            int newVar2 = sat_solver_addvar(pSolver);
            Vec_IntPush(pOut2Po, newVar2);
        }
    }
    
    // i0 i1 i2 o0 o1 o2
    // -i0 -i1 -i2 o0
    // -i0 -i1 -i2 o1
    // -i0 -i1 -i2 o2
    while(fgets(buff, 102400, fPla))
    {
        for(int i=0; i<_nPo; i++)
        {
            lit Lits_Pattern1[_nPi+1];
            for(int j=0; j<_nPi; j++)
            {
                int phaseIn = (buff[j] == '1')? 1: 0;
                int varIn = Vec_IntGetEntry(pOut1Pi, j);
                Lits_Pattern1[j] = toLitCond(varIn, phaseIn);
            }
            int phaseOut = (buff[_nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut1Po, i);
            Lits_Pattern1[_nPi] = toLitCond(varOut, phaseOut);
            cid = sat_solver_addclause(pSolver, Lits_Pattern1, Lits_Pattern1 + _nPi + 1);
            assert(cid);
        }

        for(int i=0; i<_nPo; i++)
        {
            lit Lits_Pattern2[_nPi+1];
            for(int j=0; j<_nPi; j++)
            {
                int phaseIn = (buff[j] == '1')? 1: 0;
                int varIn = Vec_IntGetEntry(pOut2Pi, j);
                Lits_Pattern2[j] = toLitCond(varIn, phaseIn);
            }
            int phaseOut = (buff[_nPi + 1 + i] == '1')? 0: 1;
            int varOut = Vec_IntGetEntry(pOut2Po, i);
            Lits_Pattern2[_nPi] = toLitCond(varOut, phaseOut);
            cid = sat_solver_addclause(pSolver, Lits_Pattern2, Lits_Pattern2 + _nPi + 1);
            assert(cid);
        }      
    }
    fclose(fPla);

    for (int i=0; i<_nPi; i++)
    {
        int varOut1 = Vec_IntEntry(pOut1Pi, i);
        int varOri = Vec_IntEntry(pOriPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut1, varOri, 0);
        assert(cid);
    }
    for (int i=0; i<_nPi; i++)
    {
        int varOut2 = Vec_IntEntry(pOut2Pi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        cid = sat_solver_add_buffer(pSolver, varOut2, varDup, 0);
        assert(cid);
    }
    // printf("Function clauses / variables / lits: %i / %i / %ld\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver), pSolver->stats.clauses_literals);
    // Alpha : (x=x' + alpha)
    Vec_Int_t *pAlphaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varAlphaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pAlphaControls, varAlphaControl);
        cid = sat_solver_conditional_unequal(pSolver, varOri, varDup, varAlphaControl);
        assert(cid);
    }

    // Beta : (x!=x' <> beta)
    Vec_Int_t *pBetaControls = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varOri = Vec_IntEntry(pOriPi, i);
        int varDup = Vec_IntEntry(pDupPi, i);
        int varBetaControl = sat_solver_addvar(pSolver);
        Vec_IntPush(pBetaControls, varBetaControl);
        cid = sat_solver_iff_unequal(pSolver, varOri, varDup, varBetaControl);
        assert(cid);
    }

    // V(Alpha ^ Beta)
    Vec_Int_t *pAndAlphaBeta = Vec_IntAlloc(_nPi);
    for (int i=0; i<_nPi; i++)
    {
        int varAlpha = Vec_IntEntry(pAlphaControls, i);
        int varBeta = Vec_IntEntry(pBetaControls, i);
        int varAnd = sat_solver_addvar(pSolver);
        Vec_IntPush(pAndAlphaBeta, varAnd);
        cid = sat_solver_add_and(pSolver, varAnd, varAlpha, varBeta, 0, 0, 0);
        assert(cid);
    }

    lit Lits_AlphaBeta[_nPi];
    for (int i=0; i<_nPi; i++)
    {
        int varAnd = Vec_IntEntry(pAndAlphaBeta, i);
        Lits_AlphaBeta[i] = toLitCond(varAnd, 0);
    }   
    cid = sat_solver_addclause(pSolver, Lits_AlphaBeta, Lits_AlphaBeta + _nPi);
    assert(cid);

    // Gamma : (o != o' <> gamma)
    Vec_Int_t *pGammaControls = Vec_IntAlloc(_nPo);
    for(int i=0; i<_nPo; i++)
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
    for(int i=0; i<_nPo; i++)
    {
        if(_litPo[i])
        {
            int var = Vec_IntEntry(pGammaControls, i);
            Lits_Gamma[count] = toLitCond(var, 0);
            count++;
        }
    }
    cid = sat_solver_addclause(pSolver, Lits_Gamma, Lits_Gamma + nLitPo);
    assert(cid);

    ///////////////////////////////////////////////////////////////////////
    // printf("Total clauses / variables / lits: %i / %i / %ld\n", sat_solver_nclauses(pSolver), sat_solver_nvars(pSolver), pSolver->stats.clauses_literals);
    // set assumption order
    for (int i=0; i<_nPi; i++)
        pLits[i] = Abc_Var2Lit(Vec_IntEntry(pAlphaControls, i),1);  
    result = sat_solver_minimize_assumptions2(pSolver, pLits, _nPi, 0);

    // update litPi
    for(int i=0; i<_nPi; i++)
        _litPi[i] = 0;
    for(int i=0; i<result; i++)
    {
        int var = Abc_Lit2Var(pLits[i]) - Vec_IntEntry(pAlphaControls, 0);
        assert(var >= 0 && var < _nPi);
        _litPi[var] = 1;
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

int IcompactMgr::icompact_cube_heuristic(char* plaFile, int iterNum, double ratio)
{
    int result;
    ICompactHeuristicMgr* mgr = new ICompactHeuristicMgr();
    mgr->readFile(plaFile);
    mgr->lockEntry(ratio);

    size_t maskCount, minMaskCount;
    char *mask, *minMask;
    vector<size_t> varOrder;

    // solve
    mask = new char[_nPi+1];
    mask[_nPi] = '\0';
    minMaskCount = _nPi;
    minMask = new char[_nPi+1];
    minMask[_nPi] = '\0';
    for(size_t i=0; i<_nPi; i++)
        varOrder.push_back(i);

    for(int iter=0; iter<iterNum; iter++)
    {
        printf("Solve heuristic - iter %i\n", iter);
        random_shuffle(varOrder.begin(), varOrder.end());
        random_shuffle(varOrder.begin(), varOrder.end());        
#ifdef DEBUG
        for(size_t i=0; i<nPi; i++)
            cout << varOrder[i] << " ";
        cout << endl;
#endif
        for(size_t i=0; i<_nPi; i++)
            mask[i] = '1';
        for(size_t i=0; i<_nPi; i++)
        {
            size_t test = varOrder[i];
            if(mgr->getLocked(test))
                continue;
            mask[test] = '0'; // drop var
            mgr->maskOne(test);
            if(mgr->findConflict(_litPo))
            {
                mask[test] = '1'; // preserve var
                mgr->undoOne();
            }        
        }
        maskCount = _nPi;
        for(size_t i=0; i<_nPi; i++)
            if(mask[i] == '0')
                maskCount--; 
        if(maskCount <= minMaskCount)
        {
            minMaskCount = maskCount;
            for(int i=0; i<_nPi; i++)
                minMask[i] = mask[i];
        }
    }

    // report
    for(size_t i=0; i<_nPi; i++)
        _litPi[i] = 0;
    result = 0;
    for(size_t i=0; i<_nPi; i++)
    {
        if(minMask[i] == '1')
        {
            _litPi[i] = 1;
            result++;
        }
    }

    // clean up
    delete [] mask;
    delete [] minMask;
    return result;
}

int IcompactMgr::icompact_cube_reencode(char* plaFile, char* reencodeplaFile, char* outputmapping, bool type, int newVar, int* record)
{
    int nRPo, lineNum;
    FILE* ff      = fopen(plaFile, "r");         // .i .o .type fr
    FILE* fReencode = fopen(reencodeplaFile, "w"); // .i .o .type fr
    FILE* fMapping  = fopen(outputmapping, "w");   // .i .o .type fr
    char buff[102400];
    char *t, *OCResult;

    ReencodeHeuristicMgr* mgr = new ReencodeHeuristicMgr();
    mgr->readFile(plaFile, type);
    nRPo = mgr->getEncoding(newVar);
    mgr->getMapping(type, record);    
    
    fgets(buff, 102400, ff);
    fprintf(fReencode, "%s", buff);
    fprintf(fMapping, ".i %i\n", nRPo);
    fgets(buff, 102400, ff);
    fprintf(fReencode, ".o %i\n", nRPo);
    fprintf(fMapping, "%s", buff);
    fgets(buff, 102400, ff);
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