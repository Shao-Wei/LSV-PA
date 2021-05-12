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

#train sklearn DT
parameters={
    'random_state':[0,2,4],
    'min_samples_split':[2],
    'min_samples_leaf':[1],
    'max_features':[None,"sqrt"],
#    'min_impurity_decrease':[0,0.02,0.05,0.1],
#    'ccp_alpha':[0,0.00035,0.0006,0.0008]
    }

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Missing argument. Usage: ./skDT.py <target> <poNum>')
        sys.exit()
    target = sys.argv[1] # samples/c1908.3000/
    poNum = int(sys.argv[2]) # 0

    logFile=open("{}/skDT.csv".format(target),'w')
    logFile.write("poNum,train_acc\n")
    logFile.write(str(poNum)+',')
    trainAccRec=[]
    trainData,trainLabel,nVar,nSampleTrain, piNameList, poName=getData(poNum,target)

    dt=tree.DecisionTreeClassifier(random_state=1)
    dt=GridSearchCV(dt,parameters)
    #print("training skDT with parameter search for target:{} poNum:{}".format(target,poNum))
    dt.fit(trainData,trainLabel)
    #print("finish training")
    #print("depth of tree:",dt.get_depth())
    pred=dt.best_estimator_.predict(trainData)
    #print('dt train acc:',np.sum(np.array(pred)==np.array(trainLabel))/nSampleTrain*100,'%')
    logFile.write("{}\n".format(np.sum(np.array(pred)==np.array(trainLabel))/nSampleTrain))

    #convert to circuit
    #print("converting and saving blif file")
    sk2Blif(nVar, piNameList, poName, dt.best_estimator_, '{}/{}'.format(target,poNum))
    #print("finish converting and saving blif file")

    logFile.close()

