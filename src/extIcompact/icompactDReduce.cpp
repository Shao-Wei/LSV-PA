#include "icompactDReduce.h"
#include <string.h>

IcompactDReduceMgr::IcompactDReduceMgr(char* fileName)
{
    char buff[10000];
    char* t, * tTmp;
    char* unused __attribute__((unused));
    FILE* fPla = fopen(fileName, "r");
    _fileName = fileName;

    // variables init to NULL
    _nTargetCount = 0;
    _targetMatrix = NULL;
    _reverse = 0;
    _pivot = NULL;
    _canonicalVar = NULL;
    _cFunc = NULL;

    // get essentials from header
    unused = fgets(buff, 10000, fPla); // .i n
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    _nPi = atoi(t);
    unused = fgets(buff, 10000, fPla); // .o n
    unused = fgets(buff, 10000, fPla); // .ilb
    _piNames = new char*[_nPi];
    t = strtok(buff, " \n");
    for(int i=0; i<_nPi; i++)
    {
        t = strtok(NULL, " \n");
        tTmp = new char[strlen(t)+1];
        strcpy(tTmp, t);
        _piNames[i] = tTmp;
    }
    unused = fgets(buff, 10000, fPla); // .ob
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    tTmp = new char[strlen(t)+1];
    strcpy(tTmp, t);
    _poName = tTmp;
    unused = fgets(buff, 10000, fPla); // .type fr
    unused = fgets(buff, 10000, fPla); // .p n
    t = strtok(buff, " \n"); t = strtok(NULL, " \n");
    _nPat = atoi(t);

    // construct matrix
    _fullMatrix = new bool*[_nPat];
    for(int i=0; i<_nPat; i++)
        _fullMatrix[i] = new bool[_nPi]();
    _fullOutput = new bool[_nPat]();

    for(int p=0; p<_nPat; p++)
    {
        unused = fgets(buff, 10000, fPla);
        t = strtok(buff, " \n");
        for(int i=0; i<_nPi; i++)
            _fullMatrix[p][i] = (t[i]=='1');
        t = strtok(NULL, " \n");
        _fullOutput[p] = (t[0]=='1');
    }
    fclose(fPla);
}

IcompactDReduceMgr::~IcompactDReduceMgr()
{
    for(int i=0; i<_nPi; i++)
        delete [] _piNames[i];
    delete [] _piNames;
    delete [] _poName;
    for(int i=0; i<_nPat; i++)
        delete [] _fullMatrix[i];
    delete [] _fullMatrix;
    delete [] _fullOutput;

    if(_pivot != NULL)
    {
        for(int i=0; i<_nTargetCount; i++)
            delete [] _targetMatrix[i];
        delete [] _targetMatrix;
        delete [] _pivot;
    }

    if(_canonicalVar != NULL)
    {
        delete [] _canonicalVar;
        for(int i=0; i<_nPi; i++)
            delete [] _cFunc[i];
        delete [] _cFunc;
    }
}

int IcompactDReduceMgr::dReducePerform(char* oPlaName, int fVerbose)
{
    printf("[Info] Performing D-reduce..\n");
    int canonicalVarCount;

    initAffine();
    GJE(_targetMatrix, _nPi, _nTargetCount);
    // printMatrix(_targetMatrix, _nPi, _nTargetCount, _nPi);
    canonicalVarCount = getCFunc();
    if(canonicalVarCount < _nPi)
        writeCompactPla(oPlaName);

    if(fVerbose)
    {
        printf("Canonical variables: %i / %i\n", canonicalVarCount, _nPi);
        if(canonicalVarCount < _nPi)
        {
            for(int i=0; i<_nPi; i++)
                if(_canonicalVar[i])
                    printf(" %s", _piNames[i]);
            printf("\n");
        } 
    }

    return (canonicalVarCount == _nPi);
}

int IcompactDReduceMgr::writeFullBlif(char* iBlifName, char* oBlifName)
{
    char buff[102400];
    char* unused __attribute__((unused));
    
    FILE* fPla = fopen(iBlifName, "r");
    if(!fPla) return 1;
    FILE* fOut = fopen(oBlifName, "w");
    if(!fPla) return 1;

    // write header
    fprintf(fOut, ".model DR_ckt\n");
    fprintf(fOut, ".inputs");
    for(int i=0; i<_nPi; i++)
        fprintf(fOut, " %s", _piNames[i]);
    fprintf(fOut, "\n.outputs %s\n", _poName);

    // copy from iBlif
    unused = fgets(buff, 102400, fPla);
    unused = fgets(buff, 102400, fPla);
    unused = fgets(buff, 102400, fPla);
    while(fgets(buff, 102400, fPla))
    {
        if(buff[0]=='.' && buff[1]=='e' && buff[2]=='n' && buff[3]=='d')
            continue;
        fprintf(fOut, "%s", buff);
    }

    // add characteristic function _cFunc, c_
    // sktree uses n_ for node, f_ for fringe, y for output
    int cNodeIdx = 0; // next c_ idx to be assigned
    int prevNodeIdx = 0; // c_ idx for the output of prev constraint 
    fprintf(fOut, "\n.names y c_%i\n1 1\n", cNodeIdx);
    prevNodeIdx = cNodeIdx;
    cNodeIdx++;
    for(int v=0; v<_nPi; v++)
    {
        if(!_canonicalVar[v])
        {
            fprintf(fOut, ".names %s c_%i\n%s\n", _piNames[v], cNodeIdx, (_pivot[v])? "1 1": "0 1");                
            cNodeIdx++;
            for(int i=0; i<_nPi; i++)
            {
                if(_cFunc[v][i])
                {                
                    fprintf(fOut, ".names c_%i %s c_%i\n01 1\n10 1\n", cNodeIdx-1, _piNames[i], cNodeIdx);
                    cNodeIdx++;
                }
            }
            fprintf(fOut, ".names c_%i c_%i c_%i\n11 1\n", cNodeIdx-1, prevNodeIdx, cNodeIdx);
            prevNodeIdx = cNodeIdx;
            cNodeIdx++;
        }
    }
    fprintf(fOut, ".names c_%i %s\n%s\n", prevNodeIdx, _poName, (_reverse)? "0 1": "1 1");
    fprintf(fOut, ".end\n");
    fclose(fPla);
    fclose(fOut);
    return 0;
}

void IcompactDReduceMgr::initAffine()
{
    // get target matrix (onset or offset which are smaller)
    _nTargetCount = 0;
    for(int i=0; i<_nPat; i++)
        if(_fullOutput[i])
            _nTargetCount++;
    if( _nTargetCount > (_nPat-_nTargetCount) ) // onset > offset
    {
        _reverse = 1;
        _nTargetCount = _nPat - _nTargetCount;
    }
    // printf("_nTargetCount = %i\n", _nTargetCount);
    _targetMatrix = new bool*[_nTargetCount];
    for(int i=0; i<_nTargetCount; i++)
        _targetMatrix[i] = new bool[_nPi]();
    
    int lineCount = 0;
    for(int p=0; p<_nPat; p++)
    {
        if(_fullOutput[p] == !_reverse)
        {
            for(int i=0; i<_nPi; i++)
                _targetMatrix[lineCount][i] = _fullMatrix[p][i];
            lineCount++;    
        }   
    }

    // set pivot, transfer to vector space
    _pivot = new bool[_nPi]();
    for(int i=0; i<_nPi; i++)
        _pivot[i] = _targetMatrix[0][i];
    for(int p=0; p<_nTargetCount; p++)
        for(int i=0; i<_nPi; i++)
            _targetMatrix[p][i] = (_targetMatrix[p][i] != _pivot[i]);
}

void swapRow(bool **m, int nCol, int r1, int r2)
{
    bool tmp;
    for(int i=0; i<nCol; i++)
    {
        tmp = m[r1][i];
        m[r1][i] = m[r2][i];
        m[r2][i] = tmp;
    }
}

void IcompactDReduceMgr::GJE(bool **m, int nCol, int nRow)
{
    int rowIdx = 0, tmp;
    for(int col=0; col<nCol; col++)
    {
        if( rowIdx >= nRow) break;
        if(m[rowIdx][col] == 0)
        {
            tmp = 1;
            while( (rowIdx+tmp) < nRow && m[rowIdx+tmp][col] == 0 )
                tmp++;
            if( (rowIdx+tmp) == nRow) continue;
            swapRow(m, nCol, rowIdx, rowIdx+tmp);
        }

        for(int row=0; row<nRow; row++)
        {
            if(row != rowIdx && m[row][col])
            {
                for(int k=col; k<nCol; k++)
                    m[row][k] = (m[rowIdx][k])? !m[row][k]: m[row][k];
            }
        }
        rowIdx++;
    }
}

int IcompactDReduceMgr::getCFunc()
{
    _canonicalVar = new bool[_nPi]();
    _cFunc = new bool*[_nPi];
    for(int i=0; i<_nPi; i++)
        _cFunc[i] = new bool[_nPi]();

    int colIdx = 0, canonicalVarCount = 0;
    for(int r=0; r<_nTargetCount; r++)
    {
        while(colIdx < _nPi)
        {
            if(!_targetMatrix[r][colIdx])
                colIdx++;
            else
                break;
        }
        if(colIdx == _nPi)
            break;
        _canonicalVar[colIdx] = 1;

        // _targetMatrix[r][colIdx] is the leading 1
        for(int i=colIdx+1; i<_nPi; i++)
            if(_targetMatrix[r][i])
                _cFunc[i][colIdx] = 1;
    }
    
    // get number of canonical var
    for(int i=0; i<_nPi; i++)
        if(_canonicalVar[i])
            canonicalVarCount++;
    return canonicalVarCount;
}

void IcompactDReduceMgr::printMatrix(bool **m, int w, int l, int firstRows)
{
    printf("Matrix:\n");
    if(firstRows<l && firstRows != -1)
        l = firstRows;
    for(int i=0; i<l; i++)
    {
        for(int j=0; j<w; j++)
            printf(" %i", (m[i][j])? 1: 0);
        printf("\n");
    }
}

void IcompactDReduceMgr::writeCompactPla(char* oPlaName)
{
    // get number of canonical var
    int canonicalVarCount = 0;
    for(int i=0; i<_nPi; i++)
        if(_canonicalVar[i])
            canonicalVarCount++;

    // get preserved minterms
    bool preserved[_nPat] = {0};
    int nPreserved = 0;
    bool fAffine, fVal;
    for(int p=0; p<_nPat; p++)
    {
        fAffine = 1;
        // check if pat is in affine space
        if(_fullOutput[p] == _reverse)
        {
            for(int i=0; i<_nPi; i++) // for each noncanonical var
            {
                if(!_canonicalVar[i])
                {
                    fVal = (_pivot[i])? _fullMatrix[p][i]: !_fullMatrix[p][i];
                    for(int j=0; j<_nPi; j++)
                        if(_cFunc[i][j])
                            fVal = (fVal != _fullMatrix[p][j]);
                    if(!fVal)
                        fAffine = 0;
                }
                if(!fAffine)
                    break;                
            }
        }

        if(fAffine)
        {
            preserved[p] = 1;
            nPreserved++;
        }
    }
    
    // start writing
    char oneline[canonicalVarCount+1];
    oneline[canonicalVarCount] = '\0';
    int colIdx;

    FILE* fPla = fopen(oPlaName, "w");
    fprintf(fPla, ".i %i\n", canonicalVarCount);
    fprintf(fPla, ".o 1\n");
    fprintf(fPla, ".ilb");
    for(int i=0; i<_nPi; i++)
        if(_canonicalVar[i])
            fprintf(fPla, " %s", _piNames[i]);
    fprintf(fPla, "\n.ob y\n");
    fprintf(fPla, ".type fr\n");
    fprintf(fPla, ".p %i\n", nPreserved);
    for(int p=0; p<_nPat; p++)
    {
        if(preserved[p])
        {
            colIdx = 0;
            for(int i=0; i<_nPi; i++)
            {
                if(_canonicalVar[i])
                oneline[colIdx] = (_fullMatrix[p][i])? '1': '0';
                colIdx++;
            }
            fprintf(fPla, "%s %s\n", oneline, (_fullOutput[p])? "1": "0");
        }
    }

    fclose(fPla);
}