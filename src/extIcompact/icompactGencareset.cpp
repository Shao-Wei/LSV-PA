#include "icompact.h"
#include <stdlib.h>        // rand()
#include <errno.h>
#include <sstream> //int to string

ABC_NAMESPACE_IMPL_START

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