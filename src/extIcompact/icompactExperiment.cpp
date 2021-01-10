#include "icompact.h"

ABC_NAMESPACE_IMPL_START

// Experiment: support restricted icompact heuristic
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
    pNtk1 = constructNtk(minMaskList1);
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
    pNtk2 = constructNtk(minMaskList2);
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

ABC_NAMESPACE_IMPL_END