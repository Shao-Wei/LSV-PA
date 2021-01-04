#include "icompact.h"

ABC_NAMESPACE_IMPL_START

// returns the number of input variables used in the cubes of the pla
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

// checks if pi/po is consistant in pFileName
int check_pla_pipo(char *pFileName, int nPi, int nPo)
{
    char buff[102400];
    char* t;
    FILE* fPla = fopen(pFileName, "r");

    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPi) { fclose(fPla); return 1; }
    fgets(buff, 102400, fPla);
    t = strtok(buff, " \n");
    t = strtok(NULL, " \n");
    if(atoi(t) != nPo) { fclose(fPla); return 1; }
    
    fclose(fPla);
    return 0;
}

ABC_NAMESPACE_IMPL_END