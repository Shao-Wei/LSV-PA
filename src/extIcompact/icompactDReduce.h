#ifndef ABC__ext__icompactDReduce_h
#define ABC__ext__icompactDReduce_h

#include <iostream>
using namespace std;

/*
This part of code implements Dimension-Reduce from 
Bernasconi, Anna, and Valentina Ciriani. "Dimension
-reducible boolean functions based on affine spaces."
 ACM Transactions on Design Automation of Electronic 
Systems (TODAES) 16.2 (2011): 1-21.

It is used by command "ntkDRconstruct".
*/

class IcompactDReduceMgr
{
public:
    IcompactDReduceMgr(char* fileName);
    ~IcompactDReduceMgr();

    int dReducePerform(char* oPlaName, int fVerbose); // 0 if reduced successfully; 1 if no noncanonical variables exist
    // int synPla2Blif(char* oPlaName, char* iBlifName); // use external method
    int writeFullBlif(char* iBlifName, char* oBlifName); // attach _cFunc to synthesized blif

private:
    char* _fileName;
    int _nPi, _nPat, _nTargetCount;
    char **_piNames, *_poName;
    bool **_fullMatrix, **_targetMatrix; // matrix for GJE representing onset / offset
    bool * _fullOutput;

    // affine space characteristics
    bool _reverse; // 1 if offset is applied, 0 if onset is applied
    bool *_pivot; // pivot vector alpha
    bool *_canonicalVar; // 1 if var is canonical
    bool **_cFunc; 

    void initAffine();
    void GJE(bool **m, int w, int l); 
    int getCFunc(); // return 1 if nCanonicalVar==_nPi; _cFunc[i][j] the ith var depends on the jth var
    void writeCompactPla(char* oPlaName);

    void printMatrix(bool **m, int w, int l, int firstRows);
    
    




};




#endif