#include "icompact.h"

ABC_NAMESPACE_IMPL_START

// Experiment 20210110: support restricted icompact heuristic
// evaluation file(-e) & log file(-l) must be specified
int IcompactMgr::exp_support_icompact_heuristic()
{
    printf("[Info] Experiment: support restricted icompact heuristic\n");
    if(_resultlogFileName == NULL) { printf("No log file. Abort.\n"); }

    int fSupport;
    int fIter = 8;
    bool **minMaskList1, **minMaskList2;
    double time_h1, time_h2, time_c1, time_c2;
    Abc_Ntk_t *pNtk1, *pNtk2, *pCone;
    Abc_Obj_t *pObj;
    FILE *fLog;
    int count;

    strncpy(_workingFileName, _samplesplaFileName, 500);
    _litWorkingPi = _litPi;
    _litWorkingPo = _litPo;
    _workingPi = _nPi;
    _workingPo = _nPo;

    fSupport = 0;
    printf("  heristic%s- %i iteration - on each PO\n", (fSupport)?" - support given ": " ", fIter);
    resetWorkingLitPi();
    resetWorkingLitPo();
    _step_time = Abc_Clock();
    minMaskList1 = icompact_heuristic_each(fIter, 0, fSupport);
    if(minMaskList1 == NULL) { _fMgr = ICOMPACT_FAIL; return 0; }
    _end_time = Abc_Clock();
    time_h1 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    _step_time = Abc_Clock();
    pNtk1 = constructNtkEach(minMaskList1, 0, 0, 0, 0);
    if(pNtk1 == NULL) { return 0; }
    _end_time = Abc_Clock();
    time_c1 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
    

    fSupport = 1;
    printf("  heristic%s- %i iteration - on each PO\n", (fSupport)?" - support given ": " ", fIter);
    resetWorkingLitPi();
    resetWorkingLitPo();
    _step_time = Abc_Clock();
    minMaskList2 = icompact_heuristic_each(fIter, 0, fSupport);
    if(minMaskList2 == NULL) { _fMgr = ICOMPACT_FAIL; return 0; }
    _end_time = Abc_Clock();
    time_h2 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    _step_time = Abc_Clock();
    pNtk2 = constructNtkEach(minMaskList2, 0, 0, 0, 0);
    if(pNtk2 == NULL) { return 0; }
    _end_time = Abc_Clock();
    time_c2 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    // start log
    printf("[Info] Compare constructed circuit w/ original circuit\n");
    fLog = fopen(_resultlogFileName, "a");

    fprintf(fLog, "\nbenchmark, nPattern, uniquePattern, portion(%%)\n");
    fprintf(fLog, "%s, %i, %i, %f\n", _funcFileName, _oriPatCount, _uniquePatCount, 100*_uniquePatCount/pow(2, _nPi));
    
    fprintf(fLog, "Timing (s), icompact, construct ntk\n");
    fprintf(fLog, "w/o support, %f, %f\n", time_h1, time_c1);
    fprintf(fLog, "w/  support, %f, %f\n", time_h2, time_c2);

    fprintf(fLog, "Overall circuit size(aig gate count)\n");
    fprintf(fLog, " original, %i\n w/o support, %i\n w/ support, %i", Abc_NtkNodeNum(_pNtk_func), Abc_NtkNodeNum(pNtk1), Abc_NtkNodeNum(pNtk2));

    fprintf(fLog, "\nSupport size on each po");
    fprintf(fLog, "\noriginal");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        count = 0;
        for(int piIdx=0; piIdx<_nPi; piIdx++)
            if(_supportInfo_func[poIdx][piIdx]) { count++; }
        fprintf(fLog, ", %i", count);        
    }
    fprintf(fLog, "\nw/o support");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        count = 0;
        for(int piIdx=0; piIdx<_nPi; piIdx++)
            if(minMaskList1[poIdx][piIdx]) { count++; }
        fprintf(fLog, ", %i", count);        
    }
    fprintf(fLog, "\nw/ support");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        count = 0;
        for(int piIdx=0; piIdx<_nPi; piIdx++)
            if(minMaskList2[poIdx][piIdx]) { count++; }
        fprintf(fLog, ", %i", count);        
    }

    fprintf(fLog, "\nCone size on each po(aig gate count)");
    fprintf(fLog, "\noriginal");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(_pNtk_func, poIdx);
        pCone = Abc_NtkCreateCone(_pNtk_func, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
    fprintf(fLog, "\nw/o support");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(pNtk1, poIdx);
        pCone = Abc_NtkCreateCone(pNtk1, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
    fprintf(fLog, "\nw/ support");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(pNtk2, poIdx);
        pCone = Abc_NtkCreateCone(pNtk2, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
        
    // clean up
    fclose(fLog);
    Abc_NtkDelete(pNtk1);
    Abc_NtkDelete(pNtk2);

    return 0;
}

// Experiment 20210202: support restricted icompact heuristic, evaluate added options(fMfs, fFraig, fSTF)
// evaluation file(-e) & log file(-l) must be specified
int IcompactMgr::exp_support_icompact_heuristic_options()
{
    printf("[Info] Experiment: support restricted icompact heuristic\n");
    if(_resultlogFileName == NULL) { printf("No log file. Abort.\n"); }

    int fSupport;
    int fIter = 8;
    bool **minMaskList;
    double time_h, time_c1, time_c2, time_c3;
    Abc_Ntk_t *pNtk1, *pNtk2, *pNtk3;
    FILE *fLog;

    strncpy(_workingFileName, _samplesplaFileName, 500);
    _litWorkingPi = _litPi;
    _litWorkingPo = _litPo;
    _workingPi = _nPi;
    _workingPo = _nPo;

    fSupport = 1;
    printf("  heristic%s- %i iteration - on each PO\n", (fSupport)?" - support given ": " ", fIter);
    resetWorkingLitPi();
    resetWorkingLitPo();
    _step_time = Abc_Clock();
    minMaskList = icompact_heuristic_each(fIter, 0, fSupport);
    if(minMaskList == NULL) { _fMgr = ICOMPACT_FAIL; return 0; }
    _end_time = Abc_Clock();
    time_h = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
/*
    _step_time = Abc_Clock();
    pNtk1 = constructNtkEach(minMaskList, 0, 0, 0, 0);
    if(pNtk1 == NULL) { return 0; }
    _end_time = Abc_Clock();
    time_c1 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    _step_time = Abc_Clock();
    pNtk2 = constructNtkEach(minMaskList, 0, 0, 1, 0);
    if(pNtk2 == NULL) { return 0; }
    _end_time = Abc_Clock();
    time_c2 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
*/
    _step_time = Abc_Clock();
    pNtk3 = constructNtkEach(minMaskList, 0, 0, 1, 1);
    if(pNtk3 == NULL) { return 0; }
    _end_time = Abc_Clock();
    time_c3 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    // start log
    printf("[Info] Compare constructed circuit w/ original circuit\n");
    fLog = fopen(_resultlogFileName, "a");

    fprintf(fLog, "\nbenchmark, nPattern, uniquePattern, portion(%%)\n");
    fprintf(fLog, "%s, %i, %i, %f\n", _funcFileName, _oriPatCount, _uniquePatCount, 100*_uniquePatCount/pow(2, _nPi));
    
    fprintf(fLog, "Timing (s), icompact, construct ntk\n");
//     fprintf(fLog, "none, %f, %f\n", time_h, time_c1);
//     fprintf(fLog, "stfault, %f, %f\n", time_h, time_c2);
    fprintf(fLog, "stfault_signal merge, %f, %f\n", time_h, time_c3);

    fprintf(fLog, "Overall circuit size(aig gate count)\n");
    fprintf(fLog, "original, %i\n", Abc_NtkNodeNum(_pNtk_func));
//     fprintf(fLog, "none, %i\n", Abc_NtkNodeNum(pNtk1));
//     fprintf(fLog, "stfault, %i\n", Abc_NtkNodeNum(pNtk2));
    fprintf(fLog, "stfault_signal merge, %i\n", Abc_NtkNodeNum(pNtk3));

    /*
    fprintf(fLog, "Support size on each po\n");
    fprintf(fLog, "original");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        count = 0;
        for(int piIdx=0; piIdx<_nPi; piIdx++)
            if(_supportInfo_func[poIdx][piIdx]) { count++; }
        fprintf(fLog, ", %i", count);        
    }
    fprintf(fLog, "\nw/ compaction");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        count = 0;
        for(int piIdx=0; piIdx<_nPi; piIdx++)
            if(minMaskList[poIdx][piIdx]) { count++; }
        fprintf(fLog, ", %i", count);        
    }

    fprintf(fLog, "\nCone size on each po(aig gate count)");
    fprintf(fLog, "\noriginal");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(_pNtk_func, poIdx);
        pCone = Abc_NtkCreateCone(_pNtk_func, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
    fprintf(fLog, "\nw/o options");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(pNtk1, poIdx);
        pCone = Abc_NtkCreateCone(pNtk1, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
    fprintf(fLog, "\nw/  options");
    for(int poIdx=0; poIdx<_nPo; poIdx++)
    {
        pObj = Abc_NtkPo(pNtk2, poIdx);
        pCone = Abc_NtkCreateCone(pNtk2, Abc_ObjFanin0(pObj), Abc_ObjName(pObj), 0);
        fprintf(fLog, ", %i", Abc_NtkNodeNum(pCone));
        Abc_NtkDelete(pCone);
    }
    fprintf(fLog, "\n");
    */
        
    // clean up
    fclose(fLog);
//     Abc_NtkDelete(pNtk1);
//     Abc_NtkDelete(pNtk2);
    Abc_NtkDelete(pNtk3);
    return 0;
}

// Experiment 20210124: output mapping circuit size (naive vs. reencode)
// evaluation file(-e) & log file(-l) must be specified
int IcompactMgr::exp_omap_construction()
{
    printf("[Info] Experiment: output compaction mapping circuit construction\n");
    if(_resultlogFileName == NULL) { printf("No log file. Abort.\n"); }

    double time_h1, time_h2, time_c1, time_c2;
    int rpo1, rpo2;
    Abc_Ntk_t *pNtk1, *pNtk2;
    FILE *fLog;

    strncpy(_workingFileName, _samplesplaFileName, 500);
    _litWorkingPi = _litPi;
    _litWorkingPo = _litPo;
    _workingPi = _nPi;
    _workingPo = _nPo;

    printf("  naive binary encoded\n");
_step_time = Abc_Clock();
    _nRPo = reencode_naive(_outputreencodedFileName, _outputmappingFileName);
_end_time = Abc_Clock();
time_h1 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
    rpo1 = _nRPo;
    _rpoNames = setDummyNames(_nRPo, "reO_");
    _litRPo = new bool[_nRPo];
_step_time = Abc_Clock();
    pNtk1 = constructNtkOmap(NULL, 0, 1, 0, 0);
_end_time = Abc_Clock();
time_c1 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    strncpy(_workingFileName, _samplesplaFileName, 500);
    _litWorkingPi = _litPi;
    _litWorkingPo = _litPo;
    _workingPi = _nPi;
    _workingPo = _nPo;

    printf("  reencode - circuit breaks down into two\n");
    int * recordPo = new int[_nPo];
_step_time = Abc_Clock();
    _nRPo = reencode_heuristic(_outputreencodedFileName, _outputmappingFileName, 0, 4, recordPo);
_end_time = Abc_Clock();
time_h2 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);
    rpo2 = _nRPo;
    _rpoNames = setDummyNames(_nRPo, "reO_");
    _litRPo = new bool[_nRPo];
_step_time = Abc_Clock();
    pNtk2 = constructNtkOmap(recordPo, 0, 1, 0, 0);
_end_time = Abc_Clock();
time_c2 = 1.0*((double)(_end_time - _step_time))/((double)CLOCKS_PER_SEC);

    // start log
    printf("[Info] Compare constructed circuit\n");
    fLog = fopen(_resultlogFileName, "a");

    fprintf(fLog, "\nbenchmark, nPi, nPo\n");
    fprintf(fLog, "%s, %i, %i\n", _funcFileName, _nPi, _nPo);
    
    fprintf(fLog, "Reencoded size\n");
    fprintf(fLog, "  naive    , %i\n", rpo1);
    fprintf(fLog, "  reencode , %i\n", rpo2);

    fprintf(fLog, "Timing (s), ocompact, construct ntk\n");
    fprintf(fLog, "  naive    , %f, %f\n", time_h1, time_c1);
    fprintf(fLog, "  reencode , %f, %f\n", time_h2, time_c2);

    fprintf(fLog, "Overall circuit size(aig gate count)\n");
    fprintf(fLog, "  original , %i\n", Abc_NtkNodeNum(_pNtk_func));
    fprintf(fLog, "  naive    , %i\n", Abc_NtkNodeNum(pNtk1));
    fprintf(fLog, "  reencode , %i\n", Abc_NtkNodeNum(pNtk2));
    // clean up
    fclose(fLog);
    Abc_NtkDelete(pNtk1);
    Abc_NtkDelete(pNtk2);

    return 0;
}
ABC_NAMESPACE_IMPL_END