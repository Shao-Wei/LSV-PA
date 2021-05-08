import numpy as np
from sklearn.tree import DecisionTreeClassifier, _tree
from sklearn.ensemble import RandomForestClassifier

def getNodeName(idx):
    return 'n_' + str(idx)

def skTreeExtract(dtree, featNames):
    def recurse(nId, nLst):
        if dtree.feature[nId] != _tree.TREE_UNDEFINED: # decision node
            recurse(dtree.children_left[nId], nLst)    # 0-branch
            recurse(dtree.children_right[nId], nLst)   # 1-branch
            nNd = getNodeName(nId)
            nC0 = getNodeName(dtree.children_left[nId])
            nC1 = getNodeName(dtree.children_right[nId])
            nCtrl = featNames[dtree.feature[nId]]
            names = ' '.join([nC0, nC1, nCtrl, nNd]) + '\n'
            vals = '-11 1\n1-0 1\n'
            nLst.append(names + vals)  
        else: # leaf node
            name = getNodeName(nId) + '\n'
            val = np.argmax(dtree.value[nId])
            val = '1\n' if (val == 1) else ''
            nLst.append(name + val)
    nLst = []
    recurse(0, nLst)
    return nLst
    
def skTree2Blif(nPi, piNameList, poName, dtree, outBlif):
    fp = open(outBlif, 'w')
    fp.write('.model skTree\n')
    fp.write('.inputs ')
    fp.write(' '.join(piNameList))
    fp.write('\n.outputs ' + poName + '\n')
    nLst = skTreeExtract(dtree, piNameList)
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
        