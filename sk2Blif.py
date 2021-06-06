import numpy as np
from sklearn.tree import DecisionTreeClassifier, _tree
from sklearn.ensemble import RandomForestClassifier
from DGDOFringe import DGDOFringe

def getNodeName(idx):
    return 'n_' + str(idx)

def skTreeExtract(dtree, featNames, nLift):
    def recurse(nId, nLst, nLift):
        if dtree.feature[nId] != _tree.TREE_UNDEFINED: # decision node
            recurse(dtree.children_left[nId], nLst, nLift)    # 0-branch
            recurse(dtree.children_right[nId], nLst, nLift)   # 1-branch
            nNd = getNodeName(nId + nLift)
            nC0 = getNodeName(dtree.children_left[nId] + nLift)
            nC1 = getNodeName(dtree.children_right[nId] + nLift)
            nCtrl = featNames[dtree.feature[nId]]
            names = ' '.join([nC0, nC1, nCtrl, nNd]) + '\n'
            vals = '-11 1\n1-0 1\n'
            nLst.append(names + vals)  
        else: # leaf node
            name = getNodeName(nId + nLift) + '\n'
            val = np.argmax(dtree.value[nId])
            val = '1\n' if (val == 1) else ''
            nLst.append(name + val)
    nLst = []
    recurse(0, nLst, nLift)
    return nLst
    
def skTree2Blif(nPi, piNameList, poName, dtree, outBlif):
    fp = open(outBlif, 'w')
    fp.write('.model skTree\n')
    fp.write('.inputs ')
    fp.write(' '.join(piNameList))
    fp.write('\n.outputs ' + poName + '\n')
    nLst = skTreeExtract(dtree, piNameList, 0)
    for n in nLst:
        fp.write('.names ' + n)
    fp.write('.names {} {}\n1 1\n.end\n'.format(getNodeName(0), poName))
    fp.close()
    
# sk2Blif: convert sklearn models into combinational circuits in BLIF format
#   - nPi: number of primary inputs
#   - piNameList: list of names of primary inputs
#   - poName: name of primary output
#   - model: the sklearn model (decision tree or random forest) 
#   - oFn: prefix of the output circuit file name
def sk2Blif(nPi, piNameList, poName, model, oFn):
    if type(model) == DecisionTreeClassifier:
        skTree2Blif(nPi, piNameList, poName, model.tree_, oFn+'.blif')
    elif type(model) == RandomForestClassifier:
        for i, dt in enumerate(model.estimators_):
            skTree2Blif(nPi, piNameList, poName, dt.tree_, oFn+str(i)+'.blif')
        
# skTreeList2Blif: convert sklearn models into combinational circuits in BLIF format
#   - nPi: number of primary inputs
#   - piNameList: list of names of primary inputs
#   - poName: name of primary output
#   - dtList: the list of decision trees(DGDO or sklearn object)
#   - oFn: prefix of the output circuit file name
def skTreeList2Blif(nPi, piNameList, poName, dtList, oFn):
    fp = open(oFn+'.blif', 'w')
    fp.write('.model skTree\n')
    fp.write('.inputs ')
    fp.write(' '.join(piNameList))
    fp.write('\n.outputs ' + poName + '\n')

    nLift = 0
    for dtree in reversed(dtList):
        print("nLift = " + str(nLift))
        if type(dtree) == DGDOFringe:
            nCount = dtree.dtreeBest.nodeCount # this is wrong
            nLst = dtree.toBlif(None, True, nLift, True, piNameList)
            for names, pats in nLst:
                fp.write('.names {}\n'.format(' '.join(names)))
                for pat in pats:
                    fp.write(pat + '\n')
        else:
            nCount = dtree.tree_.node_count
            nLst = skTreeExtract(dtree.tree_, piNameList, nLift)
            for n in nLst:
                fp.write('.names ' + n)

        nPo  = getNodeName(nLift)
        nXor = getNodeName(nLift + nCount)
        if nLift == 0:
            fp.write('.names {} {}\n1 1\n'.format(nPo, nXor))
            # print('.names {} {}\n1 1\n'.format(nPo, nXor))
        else:
            nPrev = getNodeName(nLift - 1)
            names = ' '.join([nPo, nPrev, nXor]) + '\n'
            vals = '10 1\n01 1\n'
            fp.write('.names {}{}'.format(names, vals))  
            # print('.names {}{}'.format(names, vals))   
        nLift = nLift + nCount + 1 
    fp.write('.names {} {}\n1 1\n.end\n'.format(getNodeName(nLift - 1), poName))
    # print('.names {} {}\n1 1\n.end\n'.format(getNodeName(nLift - 1), poName))
    fp.close()
