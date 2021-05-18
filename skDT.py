#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
modified from joey's code
"""
import sys
import numpy as np
from sklearn import tree
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import GridSearchCV
import matplotlib
import pickle
from sk2Blif import sk2Blif
from DGDO import DGDO
from DGDOFringe import DGDOFringe

def getData(poNum,target):
    file=open("{}/{}.pla".format(target,poNum), 'r')
    nVar=0
    nSample=6400
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

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Missing argument. Usage: ./skDT.py <target> <poNum> <type>')
        sys.exit()
    target = sys.argv[1] # samples/c1908.3000
    poNum = int(sys.argv[2]) # 0
    constructType = sys.argv[3] # "dt", "dgdo", "fringe"

    logFile=open("{}/skDT.csv".format(target),'w')
    logFile.write("type: " + str(constructType) + "\n")
    logFile.write("poNum,train_acc\n")
    logFile.write(str(poNum)+',')
    trainAccRec=[]
    trainData,trainLabel,nVar,nSampleTrain, piNameList, poName=getData(poNum,target)

    if constructType == "dt":
        dg=tree.DecisionTreeClassifier(random_state=1, min_samples_split=2, min_samples_leaf=1)
        dg.fit(trainData,trainLabel)
        #count=sum(dg.predict(trainData)==trainLabel)
        #trainAcc=count/nSampleTrain
        #print('train acc:',trainAcc)
        sk2Blif(nVar, piNameList, poName, dg, '{}/{}'.format(target,poNum))
    elif constructType == "dgdo":
        dg=DGDO(nVar,mergeCriteria="merge",mergeSampleMethod="pred",threshold=1.0)
        dg.fit(trainData,trainLabel)
        #count=sum(dg.predict(trainData)==trainLabel)
        #trainAcc=count/nSampleTrain
        #print('train acc:',trainAcc)
        sk2Blif(nVar, piNameList, poName, dg, '{}/{}'.format(target,poNum))
    elif constructType == "fringe":
        dg=DGDOFringe(mergeCriteria="merge",mergeSampleMethod="pred",threshold=1.0,max_nFeats=100)
        dg.train(trainData,trainLabel,trainData,trainLabel)
        #count=sum(dg.predict(trainData)==trainLabel)
        #trainAcc=count/nSampleTrain
        #print('train acc:',trainAcc)
        dg.toBlif('{}/{}'.format(target,poNum))
    else:
        print("Warning! Cannot parse the construction type. Options: dt, dgdo, fringe")

    logFile.close()

