#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import numpy as np
from sklearn import tree
from sklearn.model_selection import ParameterGrid
import matplotlib
import pickle
from sk2Blif import skTreeList2Blif
from DGDOFringe import DGDOFringe


parameters={
    'random_state':[1],
    'min_samples_split':[2,4],
    'min_samples_leaf':[2,3],
    #'max_features':[None,"sqrt"],
    'max_depth': [None, 4, 16]
    }

def getPlaData(poNum,target):
    file=open("{}/{}.pla".format(target,poNum), 'r')
    resultData=[]
    resultLabel=[]
    # header: .i .o .ilb .ob .type .p
    for n,line in enumerate(file.readlines()):
        if line.startswith(".e"):
            break
        if n==0:
            nVar=int(line.split(' ')[1])
        elif n==1:
            continue
        elif n==2:
            piNameList = line.split(' ')[1:]
            piNameList[-1] = piNameList[-1].rstrip()
        elif n==3:
            poName = line.split(' ')[1]
            poName = poName.rstrip()
        elif n==4:
            continue
        elif n==5:
            nSample=int(line.split(' ')[1])
        else:
            #print(line)
            data=line.split(' ')[0]
            label=int(line.split(' ')[1])
            data=[int(c) for c in data]
            resultData.append(data)
            resultLabel.append(label)
    file.close()
    return np.array(resultData),np.array(resultLabel),nVar,nSample, piNameList, poName
    
def constructRecur(trainData, trainLabel, baseType):
    def constructAux(trainData, trainLabel, nNodesLimit, level, baseType):
        candDtList = []
        candNNodes = []

        ## base case
        if level >= 1: # assert to 100% acc
            if baseType == "fringe":
                dt=DGDOFringe(mergeCriteria="merge",mergeSampleMethod="pred",threshold=1.0,max_nFeats=100)
                dt.train(trainData,trainLabel,trainData,trainLabel)
                return [dt]
            else:
                dt = tree.DecisionTreeClassifier(random_state=1, min_samples_split=2, min_samples_leaf=1)
                dt.fit(trainData,trainLabel)
                return [dt]
            
        ## update
        # worst case preserved
        dt = tree.DecisionTreeClassifier(random_state=1, min_samples_split=2, min_samples_leaf=1)
        dt.fit(trainData,trainLabel)
        nNodes = dt.tree_.node_count
        if(level == 0):
            print("Level 0 cand: ")
            print(str(nNodes) + " : " + str(1.0))
        candDtList.append([dt])
        # grid search
        parameterGrids = ParameterGrid(parameters)
        for setting in parameterGrids:
            dt = tree.DecisionTreeClassifier()
            dt.set_params(**setting)
            dt.fit(trainData,trainLabel)
            nNodes = dt.tree_.node_count
            labelMisclassified = dt.predict(trainData) != trainLabel
            trainAcc = 1 - sum(labelMisclassified)/len(trainLabel)
            if(level == 0):
                print(str(nNodes) + " : " + str(trainAcc))
            if trainAcc >= 1.0:
                candDtList.append([dt])
            if trainAcc < 1.0 and trainAcc > 0.7 and nNodes < 0.9 * candDtList[0][0].tree_.node_count:
                candDtList.append([dt] + constructAux(trainData, labelMisclassified, 5 * nNodes, level+1, baseType))
        for idx in range(len(candDtList)):
            nodeCount = 0
            for d in candDtList[idx]:
                if type(d) == DGDOFringe:
                    nodeCount = nodeCount + d.nodeCount
                else:
                    nodeCount = nodeCount + d.tree_.node_count
                candNNodes.append(nodeCount)
        if level == 0:
            print("Size report: ")
            for n in candNNodes:
                print(n)
        
        return candDtList[candNNodes.index(min(candNNodes))]
    
    return constructAux(trainData, trainLabel, 100000, 0, baseType)
    
if __name__ == '__main__':
    if len(sys.argv) < 4:
        print('Missing argument. Usage: ./dtRecur.py <target> <poNum> <baseType>')
        sys.exit()
    target = sys.argv[1]     # samples/c1908.3000
    poNum = int(sys.argv[2]) # 0
    baseType = sys.argv[3] # dt | fringe

    # log files
    logFile=open("{}/dtRecur.csv".format(target),'w')
    logFile.write("poNum, train_acc\n")
    logFile.write(str(poNum)+',')
    trainAccRec=[]

    # construct circuit
    trainData,trainLabel,nVar,nSampleTrain, piNameList, poName = getPlaData(poNum, target)
    dtList = constructRecur(trainData, trainLabel, baseType)
    skTreeList2Blif(nVar, piNameList, poName, dtList, '{}/{}'.format(target,poNum))

    logFile.close()
