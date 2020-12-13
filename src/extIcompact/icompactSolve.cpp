#include "icompact.h"
#include "sat/cnf/cnf.h"
#include "base/abc/abc.h"  // Abc_NtkStrash
#include <stdlib.h>        // rand()
#include <errno.h>

ABC_NAMESPACE_IMPL_START

int miniUnsatCore(sat_solver* s, int * pLits, int nLits)
{
    int result = 0;

    veci conflict = s->conf_final;
    assert(conflict.size);
    for(int i=0; i<conflict.size; i++)
        printf("%i ", veci_begin(&conflict)[i]);
    printf("\n");
    bool assumed[nLits] = { false };
    int pLits_tmp[nLits];
    int idx = 0;

    // move assumption to the front
    for(int i=0; i<conflict.size; i++)
    {
        for(int k=0; k<nLits; k++)
            if(Abc_Lit2Var(veci_begin(&conflict)[i]) == Abc_Lit2Var(pLits[k]))
            {
                assumed[k] = true;
                pLits_tmp[idx] = pLits[k];
                idx++;
                break;
            }
    }
    result = idx;

    // append the rest
    for(int k=0; k<nLits; k++)
        if(!assumed[k])
        {
            pLits_tmp[idx] = pLits[k];
            idx++;
        }
    for(int k=0; k<nLits; k++)
        pLits[k] = pLits_tmp[k];

    return result;
}

int sat_solver_get_minimized_assumptions(sat_solver* s, int * pLits, int nLits, int nConfLimit)
{
    int i, k, status, tmp_result;
    if ( nLits == 1 )
    {
        // since the problem is UNSAT, we will try to solve it without assuming the last literal
        // if the result is UNSAT, the last literal can be dropped; otherwise, it is needed
        if ( nConfLimit ) s->nConfLimit = s->stats.conflicts + nConfLimit;
        status = sat_solver_solve_internal( s );
        return (int)(status != l_False); // return 1 if the problem is not UNSAT
    }
    assert( nLits >= 2 );
    // assume all lits
    for (i = 0; i < nLits; i++ )
        if ( !sat_solver_push(s, pLits[i]) )
        {
            if(i != nLits - 1)
            {
                for (k = i; k >= 0; k-- )
                    sat_solver_pop(s);
                return sat_solver_get_minimized_assumptions( s, pLits, i+1, nConfLimit );   
            }
            else // if we only failed to assume the last assumption variable, try to solve without it.
            { 
                printf("Last:%i\n", nLits);
                for (k = i; k >= 0; k-- )
                    sat_solver_pop(s);
                tmp_result = sat_solver_get_minimized_assumptions( s, pLits, i, nConfLimit );
                printf("tmpresult = %i\n", tmp_result);
                if(tmp_result) // if UNSAT, return; else return current nLits
                    return tmp_result;
                else
                    return nLits;    
            }
        }
            
    // solve with these assumptions
    if ( nConfLimit ) s->nConfLimit = s->stats.conflicts + nConfLimit;
    status = sat_solver_solve_internal( s );
    if ( status == l_False ) // these are enough
    {
        for (i = 0; i < nLits; i++)
            sat_solver_pop(s);
        return miniUnsatCore(s, pLits, nLits);
    }

    return 0; // wrong formulation
}



ABC_NAMESPACE_IMPL_END