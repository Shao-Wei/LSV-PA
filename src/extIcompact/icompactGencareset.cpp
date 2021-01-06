#include "icompact.h"
#include <stdlib.h> // rand()
#include <sstream> // int to string
#include "proof/fra/fra.h"

ABC_NAMESPACE_IMPL_START

/////////////////////////////////////////////////////////
// Simulation 
// Modified from fraSim.c to support 1. simulation results written to file, 2. simulation accuracy report
/////////////////////////////////////////////////////////
/*== base/abci/abcDar.c ==*/
extern "C" { Aig_Man_t *Abc_NtkToDar(Abc_Ntk_t *pNtk, int fExors, int fRegisters); }
/*== proof/fra/fraSim.c ==*/
extern "C" { Vec_Str_t * Fra_SmlSimulateReadFile( char * pFileName ); }
extern "C" { void Fra_SmlSimulateOne( Fra_Sml_t * p ); }

int smlWriteGolden( Fra_Sml_t * p, Vec_Str_t * vSimInfo, int nPart, char* pfilename)
{
    int k, i;
    Aig_Obj_t * pObj;
    Aig_Man_t * pAig = p->pAig;
    int patLen = Aig_ManCiNum(pAig) + Aig_ManCoNum(pAig);

    FILE * fsample = fopen(pfilename, "a");
    if(!fsample)
        return 1;

    for ( k = 0; k < nPart; k++ )
    {
        for (int o=k*patLen; o<(k+1)*patLen - Vec_PtrSize(pAig->vCos); o++)
            fprintf(fsample, "%d", Vec_StrEntry(vSimInfo, o));
        fprintf(fsample, " ");
        Aig_ManForEachCo( pAig, pObj, i )
            fprintf(fsample, "%d", Abc_InfoHasBit( Fra_ObjSim( p, pObj->Id ), k ) );
        fprintf(fsample, "\n");
    }

    fclose(fsample);
    return 0;
}

int smlCompareGolden( Fra_Sml_t * p, Vec_Str_t * vSimInfo, int nPart)
{
    int k, i;
    char goldenBit;
    Aig_Obj_t * pObj;
    
    Aig_Man_t * pAig = p->pAig;
    bool fCorrect;
    int correctCount = 0;

    for(k=0; k<nPart; k++)
    {
        fCorrect = 1;
        Aig_ManForEachCo(pAig, pObj, i)
        {
            goldenBit = Vec_StrEntry(vSimInfo, (k+1)*Aig_ManCiNum(pAig)+ k*Aig_ManCoNum(pAig) + i);
            if(Abc_InfoHasBit(Fra_ObjSim( p, pObj->Id ), k) != goldenBit) 
            {
                fCorrect = 0;
                break;
            } 
        }
        if(fCorrect) { correctCount++; }
    }

    return correctCount;
}

// modified from src/proof/fra/fraSim.c Fra_SmlSimulateReadFile
Vec_Str_t * smlSimulateReadFile( char * pFileName, int nIgnore)
{
    Vec_Str_t * vRes;
    FILE * pFile;
    int c;
    char buff[102400];
    char * unused __attribute__((unused)); // get rid of fget warnings
    pFile = fopen( pFileName, "rb" );
    if ( pFile == NULL )
    {
        printf( "Cannot open file \"%s\" with simulation patterns.\n", pFileName );
        return NULL;
    }
    for(int i=0; i<nIgnore; i++) { unused = fgets(buff, 102400, pFile); } // skip lines
    vRes = Vec_StrAlloc( 1000 );
    while ( (c = fgetc(pFile)) != EOF )
    {
        if ( c == '0' || c == '1' )
            Vec_StrPush( vRes, (char)(c - '0') );
        else if ( c != ' ' && c != '\r' && c != '\n' && c != '\t' )
        {
            printf( "File \"%s\" contains symbol (%c) other than \'0\' or \'1\'.\n", pFileName, (char)c );
            Vec_StrFreeP( &vRes );
            break;
        }
    }
    fclose( pFile );
    return vRes;
}

// modified from src/proof/fra/fraSim.c Fra_SmlInitializeGiven
void smlInitializeGiven( Fra_Sml_t * p, Vec_Str_t * vSimInfo )
{
    Aig_Obj_t * pObj;
    unsigned * pSims;
    int i, k, nPats = Vec_StrSize(vSimInfo) / (Aig_ManCiNum(p->pAig)+Aig_ManCoNum(p->pAig));
    int nPatsPadded = p->nWordsTotal * 32;
    assert( Aig_ManRegNum(p->pAig) == 0 );
    assert( Vec_StrSize(vSimInfo) % (Aig_ManCiNum(p->pAig)+Aig_ManCoNum(p->pAig)) == 0 );
    assert( nPats <= nPatsPadded );
    Aig_ManForEachCi( p->pAig, pObj, i )
    {
        pSims = Fra_ObjSim( p, pObj->Id );
        // clean data
        for ( k = 0; k < p->nWordsTotal; k++ )
            pSims[k] = 0;
        // load patterns
        for ( k = 0; k < nPats; k++ )
            if ( Vec_StrEntry(vSimInfo, k * (Aig_ManCiNum(p->pAig)+Aig_ManCoNum(p->pAig)) + i) )
                Abc_InfoSetBit( pSims, k );
        /*
        // pad the remaining bits with the value of the last pattern
        for ( ; k < nPatsPadded; k++ )
            if ( Vec_StrEntry(vSimInfo, (nPats-1) * Aig_ManCiNum(p->pAig) + i) )
                Abc_InfoSetBit( pSims, k );
        */
    }
}

// modified from src/proof/fra/fraSim.c Fra_SmlSimulateCombGiven()
int smlSimulateCombGiven( Abc_Ntk_t* pNtk, char * pFileName)
{
    Vec_Str_t * vSimInfo, * vSimPart;
    Fra_Sml_t * p = NULL;
    char pFileName2[200];
    FILE* fSamples;
    int nPatterns, nPart, nPatPerSim;
    int patLen;
    int cFlag, i;
    Abc_Obj_t * pObj;
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);

    // read comb patterns from file
    vSimInfo = smlSimulateReadFile( pFileName, 0 );
    if ( vSimInfo == NULL )
        return 1;

    patLen = Aig_ManCiNum(pAig)+Aig_ManCoNum(pAig);
    if ( Vec_StrSize(vSimInfo) % patLen != 0 )
    {
        printf( "File \"%s\": The number of binary digits (%d) is not divisible by the number of pi (%d) + po (%d).\n", 
            pFileName, Vec_StrSize(vSimInfo), Aig_ManCiNum(pAig), Aig_ManCoNum(pAig) );
        Vec_StrFree( vSimInfo );
        return 1;
    }

    // avoid seg fault: divide pattern to 128 frames per simulation
    nPatPerSim = 4096;
    nPatterns = Vec_StrSize(vSimInfo) / patLen;
    sprintf(pFileName2, "%s.samples.pla", pFileName);
    printf("[Info] Simulate using %d patterns. Results written in %s\n", nPatterns, pFileName2);
    
    fSamples = fopen(pFileName2, "w");  // write header
    fprintf(fSamples, ".i %i\n", Aig_ManCiNum(pAig));
    fprintf(fSamples, ".o %i\n", Aig_ManCoNum(pAig));
    fprintf(fSamples, ".ilb");
    Abc_NtkForEachCi(pNtk, pObj, i)
        fprintf(fSamples, " %s", Abc_ObjName(pObj));
    fprintf(fSamples, "\n.ob");
    Abc_NtkForEachCo(pNtk, pObj, i)
        fprintf(fSamples, " %s", Abc_ObjName(pObj));
    fprintf(fSamples, "\n.type fr\n");
    fclose(fSamples);

    for (int n=0; n<=(nPatterns/nPatPerSim); n++)
    {
        nPart = (n==(nPatterns/nPatPerSim))? (nPatterns%nPatPerSim): nPatPerSim;
        if (nPart == 0) { break; }
        p = Fra_SmlStart( pAig, 0, 1, Abc_BitWordNum(nPart) );

        vSimPart = Vec_StrAlloc(0);
        for (int l=n*patLen*nPart; l<(n+1)*patLen*nPart; l++)
            Vec_StrPush(vSimPart, Vec_StrEntry(vSimInfo, l));
        
        // start simulation
        smlInitializeGiven( p, vSimPart );
        Fra_SmlSimulateOne( p );
        cFlag = smlWriteGolden( p, vSimPart, nPart, pFileName2);
        if(cFlag)
            return 1;

        Vec_StrFree( vSimPart );
        Fra_SmlStop( p );
    }

    Vec_StrFree( vSimInfo );
    Aig_ManStop(pAig);
    return 0;
}

// modified from src/proof/fra/fraSim.c Fra_SmlSimulateCombGiven()
int smlVerifyCombGiven( Abc_Ntk_t* pNtk, char * pFileName)
{
    Vec_Str_t * vSimInfo, * vSimPart;
    Fra_Sml_t * p = NULL;
    int nPatterns, nPart, nPatPerSim;
    int patLen;
    int correctCount, totalCount = 0;
    Aig_Man_t * pAig = Abc_NtkToDar(pNtk, 0, 0);

    // read comb patterns from file
    vSimInfo = smlSimulateReadFile( pFileName, 5 ); // skip header
    if ( vSimInfo == NULL )
        return 1;

    patLen = Aig_ManCiNum(pAig)+Aig_ManCoNum(pAig);
    if ( Vec_StrSize(vSimInfo) % patLen != 0 )
    {
        printf( "File \"%s\": The number of binary digits (%d) is not divisible by the number of pi (%d) + po (%d).\n", 
            pFileName, Vec_StrSize(vSimInfo), Aig_ManCiNum(pAig), Aig_ManCoNum(pAig) );
        Vec_StrFree( vSimInfo );
        return 1;
    }

    // avoid seg fault: divide pattern to 128 frames per simulation
    nPatPerSim = 4096;
    nPatterns = Vec_StrSize(vSimInfo) / patLen;

    printf("Verifying circuit:");
    for (int n=0; n<=(nPatterns/nPatPerSim); n++)
    {
        nPart = (n==(nPatterns/nPatPerSim))? (nPatterns%nPatPerSim): nPatPerSim;
        if (nPart == 0) { break; }
        p = Fra_SmlStart( pAig, 0, 1, Abc_BitWordNum(nPart) );

        vSimPart = Vec_StrAlloc(0);
        for (int l=n*patLen*nPart; l<(n+1)*patLen*nPart; l++)
            Vec_StrPush(vSimPart, Vec_StrEntry(vSimInfo, l));
        
        // start simulation
        smlInitializeGiven( p, vSimPart );
        Fra_SmlSimulateOne( p );
        correctCount = smlCompareGolden(p, vSimPart, nPart);
        totalCount += correctCount;

        Vec_StrFree( vSimPart );
        Fra_SmlStop( p );
    }
    printf(" : %i / %i (correct/total) (%f%%)\n", totalCount, nPatterns, 100*totalCount/(double)nPatterns);
    Vec_StrFree( vSimInfo );
    Aig_ManStop(pAig);
    return (totalCount == nPatterns)? 1: 0;
}

int careset2patterns(char* patternsFileName, char* caresetFilename, int nPi, int nPo)
{
    FILE *fcareset = fopen(caresetFilename, "r");
    FILE *fpatterns = fopen(patternsFileName, "w");
    char buff[102400];
    char * unused __attribute__((unused));
    int minterm_total;
    int enum_count;
    int local_count;
    char* literals = new char[nPi];
    char* one_line = new char[nPi + nPo + 2];
    one_line[nPi+nPo+1] = '\0';
    one_line[nPi] = ' ';
    for(int i=nPi+1; i<nPi+nPo+1; i++)
        one_line[i] = '0';

    unused = fgets(buff, 102400, fcareset); // no header needed for simulation file
    unused = fgets(buff, 102400, fcareset);
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

/////////////////////////////////////////////////////////
// Random String Generation
/////////////////////////////////////////////////////////
void random_1D_array_bool(bool *array, int length, int n, int seed)
{
    srand(seed);
    int n1 = 0; //number of 1s
    int index[length];

    for (int i = 0; i < length; i++)
        array[i] = 0;

    for (int i = 0; i < length; i++)
        index[i] = i;

    while (n1 != n)
    {
        int ind = rand() % (length - n1);
        array[index[ind]] = 1;
        for (int i = ind; i < length - (n1 + 1); i++)
            index[i] = index[i + 1];
        n1 = n1 + 1;
    }
}

void random_1D_array_int(int *array, int length, int n, int seed)
{
    srand(seed);
    int n1 = 0; //number of 1s
    int index[length];

    for (int i = 0; i < length; i++)
        array[i] = 0;

    for (int i = 0; i < length; i++)
        index[i] = i;

    while (n1 != n)
    {
        int ind = rand() % (length - n1);
        array[index[ind]] = 1;
        for (int i = ind; i < length - (n1 + 1); i++)
            index[i] = index[i + 1];
        n1 = n1 + 1;
    }
}

void n_gen_AP(int nPat, int nPi, int nPo, char* filename)
{
	srand(time(0)); // seed of rand

	FILE * f = fopen(filename, "w");
    fprintf(f, ".i %i\n.o %i\n", nPi, 1);

	double one_1_ratio = 0.5;
    int one_1_num = (int)nPi * one_1_ratio; 
    if (one_1_num == 0)
        one_1_num = 1;
    int avg_npat = (int)nPat / (nPi - 1);
    int *npat_num_1 = new int[nPi - 1];
    int npat_now = 0;

    if (avg_npat == 0)
    {
        random_1D_array_int(npat_num_1, nPi - 1, nPat, rand());
    }
    else if (avg_npat <= one_1_num)
    {
        for (int i = 0; i < nPi - 1; i++)
            npat_num_1[i] = avg_npat;
    }
    else //avg_npat > one_1_num
    {
        int sn = nPat / 2;
        int n = (((nPi - 1) & 1) == 1) ? (nPi - 1) / 2 + 1 : (nPi - 1) / 2;
        int a1 = one_1_num;
        int an = 2 * sn / n - a1;
        double d = (double) (an - a1) / (n - 1);
        for (int i = 0; i < n; i++)
        {
            npat_num_1[i] = a1 + d * i;
            npat_num_1[nPi - 2 - i] = npat_num_1[i];
        }
    }


    bool *basepat = new bool[nPi];
    char *onepat = new char[nPi +1];// for outputting one pattern
    onepat[nPi  ] = '\0';

	for (int i = 1; i < nPi; i++)
    {
        int k = 0;
        while (k < npat_num_1[i - 1])
        {
            random_1D_array_bool(basepat, nPi, i, rand());
            npat_now = npat_now + 1;
            k = k + 1;
			for (int j = 0; j < nPi; j++)
            {
                if (basepat[j] == 1)
                    onepat[j] = '1';
                else
                    onepat[j] = '0';
            }
				
            fprintf(f, "%s 1\n", onepat);
        }
    }

	delete [] basepat;
	delete [] npat_num_1;

    for (int i = npat_now; i < nPat; i++)
	{
	    for (int j = 0; j < nPi; j++)
			if ((rand() & 1) == 1)
            {    onepat[j] = '1'; }
            else
            {    onepat[j] = '0'; }
		
		fprintf(f, "%s 1\n", onepat);
	}

	delete [] onepat;
    fclose(f);
}

void n_gen_Random(int nPat, int nPi, int nPo, char* filename)
{
	srand(time(0)); // seed of rand

    char *onepat = new char[nPi+1];
    onepat[nPi] = '\0';

    char *dummyoutput = new char[nPo +1];
    for(int i=0; i<nPo; i++)
        dummyoutput[i] = '0';
    dummyoutput[nPo] = '\0';

    FILE * f = fopen(filename, "w");
    fprintf(f, ".i %i\n.o %i\n", nPi, 1);

    FILE *fver = fopen("check.pattern", "w");
    fprintf(fver, "dummy\ndummy\n");

    for (int i = 0; i < nPat; i++)
	{
	    for (int j = 0; j < nPi; j++)
        {
            if ((rand() & 1) == 1)
            {    onepat[j] = '1'; }
            else
            {    onepat[j] = '0'; }
        }
		fprintf(f, "%s 1\n", onepat);
        fprintf(fver, "%s%s\n", onepat, dummyoutput);
	}

	delete [] onepat;
    fclose(f);
    fclose(fver);
}

void n_gen_Cube(int nPat, int nCube, int nPi, int nPo, char* filename)
{
    srand(time(0)); // seed of rand

    char *onepat = new char[nPi+1];
    onepat[nPi] = '\0';

    FILE * f = fopen(filename, "w");
    fprintf(f, ".i %i\n.o %i\n", nPi, 1);

    int randbit;
    int dontcarepercube = 64 - __builtin_clzll((unsigned long long)nPat/nCube);
    if((nPat & (nPat-1)) == 0)
        dontcarepercube = dontcarepercube - 1;
    printf("Don't care bits per cube: %i\n", dontcarepercube);

    for (int i=0; i<nCube; i++)
	{
	    randbit = rand() % nPi;
        for (int j=0; j<nPi; j++)
        {
            if(j>=randbit && j<randbit + dontcarepercube)
            {    onepat[j] = '-'; }
            else if ( j<randbit-nPi+dontcarepercube )
            {    onepat[j] = '-'; }
            else if ((rand() & 1) == 1)
            {    onepat[j] = '1'; }
            else
            {    onepat[j] = '0'; }
        }
		fprintf(f, "%s 1\n", onepat);
	}

	delete [] onepat;
    fclose(f);
}

ABC_NAMESPACE_IMPL_END