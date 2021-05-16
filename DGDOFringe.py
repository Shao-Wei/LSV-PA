import numpy as np
import pickle
from DGDO import DGDO

class DGDOFringe():
    def __init__(self, mergeCriteria="noMerge", mergeSampleMethod="pred", threshold=0.9, max_nFeats=1500, patience=1, verbose=True):
        self.verbose = verbose
        self.patience = patience

        self.mergeCriteria=mergeCriteria#three types: (1) conf<n, (2) iden>n (3) idenGConf or noMerge
        self.mergeSampleMethod=mergeSampleMethod#two types: (1)"pred":run predict from the node and see if the acc>threshold. (2)"data": compare the numConf and numIden of data on the two nodes
        self.threshold=threshold
        #DG stat
        self.nodeCount=0
    
        # fringe constraints
        self.max_nFeats = max_nFeats
        self.fringeFeats = dict()
        self.fringeFeatsBest = None
        
        # dtree
        self.dtree = None
        self.dtreeBest = None
        self.nFeats = None
        
    def __trainInt__(self, data, labels,iter):
        assert len(data) == len(labels)
        if iter>0:
            self.threshold+=0.005
            if self.threshold>1: self.threshold=1
            print("update threshold to:",self.threshold)
        self.dtree = DGDO(data.shape[1],mergeCriteria=self.mergeCriteria,mergeSampleMethod=self.mergeSampleMethod,threshold=self.threshold)
        self.dtree.fit(data, labels)
    
    def train(self, trainData, trainLabels, validData, validLabels):        
        self.nFeats = len(trainData[0])
        assert len(validData[0]) == self.nFeats
        
        trainAccBest, valAccBest = None, -1.0
        iter = 0
        while True:
            self.__trainInt__(trainData, trainLabels,iter)
            _, trainAcc = self.predict(trainData, trainLabels, False, False)
            _, valAcc = self.predict(validData, validLabels, False, False)
            if self.verbose:
                print('iter {}: {} / {}'.format(str(iter), str(trainAcc), str(valAcc)))
            if valAcc > valAccBest:
                self.dtreeBest = self.dtree
                self.fringeFeatsBest = self.fringeFeats.copy()
                valAccBest = valAcc
                trainAccBest = trainAcc
            if self.nFeats + len(self.fringeFeats) > self.max_nFeats: break
            
            initSize = len(self.fringeFeats)
            featSet = self.__fringeDetect__()
            for feat in featSet:
                if feat in self.fringeFeats: continue
                id1, id2, op = feat
                trainData = self.__dataAug__(trainData, id1, id2, op)
                validData = self.__dataAug__(validData, id1, id2, op)
                self.fringeFeats[feat] = self.nFeats + len(self.fringeFeats)            
            print("len(self.fringeFeats):",len(self.fringeFeats))
            if len(self.fringeFeats) == initSize: break
            iter += 1
        return trainAccBest, valAccBest
    
    # def __trainInt2__(self, trainData, trainLabels, validData, validLabels):
    #     self.dtree = None
    #     assert len(trainData) == len(trainLabels)
    #     assert len(validData) == len(validLabels)
        
    #     def eval(dt, data, labels):
    #         preds = dt.predict(data)
    #         return np.sum(np.array(preds)==np.array(labels)) / len(labels)
        
    #     dt0 = DGDO(trainData.shape[1],mergeCriteria=self.mergeCriteria,mergeSampleMethod=self.mergeSampleMethod,threshold=self.threshold)
        
    #     # ccp_alphas = dt0.cost_complexity_pruning_path(trainData, trainLabels).ccp_alphas
    #     # ccp_alphas = np.array_split(ccp_alphas, min(10, len(ccp_alphas)))
    #     # ccp_alphas = [x.mean() for x in ccp_alphas]
        
    #     vaccBest = -1.0
    #     for ccp_alpha in ccp_alphas:
    #         dt = DecisionTreeClassifier(random_state=self.random_state, criterion=self.criterion, max_depth=self.max_depth, ccp_alpha=ccp_alpha)
    #         dt.fit(trainData, trainLabels)
    #         vacc = eval(dt, validData, validLabels)
    #         if vacc > vaccBest:
    #             self.dtree = dt
    #             self.ccp_alpha = ccp_alpha
    #             vaccBest = vacc
    #     assert self.dtree is not None

    # def train2(self, trainData, trainLabels, validData, validLabels):
    #     self.nFeats = len(trainData[0])
    #     assert len(validData[0]) == self.nFeats
        
    #     trainAccBest, valAccBest = None, -1.0
    #     iter = 0
    #     patience = self.patience
    #     while True:
    #         self.__trainInt__(trainData, trainLabels, validData, validLabels)
    #         _, trainAcc = self.predict(trainData, trainLabels, False, False)
    #         _, valAcc = self.predict(validData, validLabels, False, False)
    #         if self.verbose:
    #             print('iter {}: {} / {}'.format(str(iter), str(trainAcc), str(valAcc)))
    #         if valAcc > valAccBest:
    #             self.dtreeBest = self.dtree
    #             self.fringeFeatsBest = self.fringeFeats.copy()
    #             valAccBest = valAcc
    #             trainAccBest = trainAcc
    #             patience = self.patience
    #         if self.nFeats + len(self.fringeFeats) > self.max_nFeats: break
            
    #         initSize = len(self.fringeFeats)
    #         featSet = self.__fringeDetect__()
    #         for feat in featSet:
    #             if feat in self.fringeFeats: continue
    #             id1, id2, op = feat
    #             trainData = self.__dataAug__(trainData, id1, id2, op)
    #             validData = self.__dataAug__(validData, id1, id2, op)
    #             self.fringeFeats[feat] = self.nFeats + len(self.fringeFeats)            
            
    #         if len(self.fringeFeats) == initSize: break
    #         iter += 1
    #         patience -= 1
    #         if patience < 0: break
    #     return trainAccBest, valAccBest
        
        
    def predict(self, data, labels=None, aug=True, useBest=True):
        getAcc = lambda preds, labels: np.sum(np.array(preds)==np.array(labels)) / len(labels)
        
        if aug: data = self.transformData(data)
        
        dtree = self.dtreeBest if useBest else self.dtree
        assert len(data[0]) == dtree.nVar
        preds = dtree.predict(data)
        
        if labels is None: return preds
        acc = getAcc(preds, labels)
        return preds, acc
        
    def transformData(self, data, useBest=True):
        assert len(data[0]) == self.nFeats
        feats = self.__getFringeFeats__(useBest)
        for id1, id2, op in feats:
            data = self.__dataAug__(data, id1, id2, op)
        return data
        
    def dumpFeats(self, fn, useBest=True):
        feats = self.__getFringeFeats__(useBest)
        with open(fn, 'w') as fp:
            fp.write('id1,id2,op\n')
            for id1, id2, op in feats:
                fp.write('{},{},{}\n'.format(str(id1), str(id2), op))
            
        
    def __getFringeFeats__(self, useBest=True):
        feats = self.fringeFeatsBest if useBest else self.fringeFeats
        feats = list(feats.items())
        feats.sort(key = lambda k: k[1])
        return [x[0] for x in feats]
    
        
    def __fringeDetect__(self):
        dtree_ = self.dtree
        
        isLeaf = lambda nId: dtree_.var[nId] == -1
        getLeftChild = lambda nId: dtree_.leftChild[nId]
        getRightChild = lambda nId: dtree_.rightChild[nId]
        
        def getChild(nId, br):
            if (nId is None) or isLeaf(nId):
                return None
            if br == 0:
                return getLeftChild(nId)
            else:
                return getRightChild(nId)
            
        def isLeaf1(nId):
            if (nId is None) or not(isLeaf(nId)):
                return False
            return dtree_.pred[nId] == 1
            
        def isLeaf0(nId):
            if (nId is None) or not(isLeaf(nId)):
                return False
            return dtree_.pred[nId] == 0
            
            
        def nodeDetect(nId):
            if (nId is None) or isLeaf(nId):
                return None
                
            if isLeaf0(getChild(getChild(nId, 0), 1)) and isLeaf0(getChild(getChild(nId, 1), 0)) and (getChild(nId, 0) == getChild(nId, 1)):
                return getChild(nId, 0), 'x=y'
            
            if isLeaf0(getChild(getChild(nId, 0), 0)) and isLeaf0(getChild(getChild(nId, 1), 1)) and (getChild(nId, 0) == getChild(nId, 1)):
                return getChild(nId, 0), 'x^y'
            
            if isLeaf1(getChild(getChild(nId, 0), 0)):
                if isLeaf1(getChild(nId, 1)) and isLeaf0(getChild(getChild(nId, 0), 1)):
                    op = 'x|~y'
                elif isLeaf1(getChild(getChild(nId, 1), 1)) and (getChild(nId, 0) == getChild(nId, 1)):
                    op = 'x=y'
                else:
                    op = '~x&~y'
                return getChild(nId, 0), op
            
            if isLeaf1(getChild(getChild(nId, 0), 1)):
                if isLeaf1(getChild(nId, 1)) and isLeaf0(getChild(getChild(nId, 0), 0)):
                    op = 'x|y'
                elif isLeaf1(getChild(getChild(nId, 1), 0)) and (getChild(nId, 0) == getChild(nId, 1)):
                    op = 'x^y'
                else:
                    op = '~x&y'
                return getChild(nId, 0), op
                
            if isLeaf1(getChild(getChild(nId, 1), 0)):
                if isLeaf1(getChild(nId, 0)) and isLeaf0(getChild(getChild(nId, 1), 1)):
                    op = '~x|~y'
                else:
                    op = 'x&~y'
                return getChild(nId, 1), op
                
            if isLeaf1(getChild(getChild(nId, 1), 1)):
                if isLeaf1(getChild(nId, 0)) and isLeaf0(getChild(getChild(nId, 1), 0)):
                    op = '~x|y'
                else:
                    op = 'x&y'
                return getChild(nId, 1), op
            
            return None
            
        ret = set()
        for nId in range(dtree_.nodeCount):
            feat_ = nodeDetect(nId)
            if feat_ is None: continue
            x = dtree_.var[nId]
            y = dtree_.var[feat_[0]]
            op = feat_[1]
            feat = (x, y, op)
            ret.add(feat)
        return ret
        
        
    def __dataAug__(self, data, id1, id2, op):
        d1 = data.transpose()[id1]
        d2 = data.transpose()[id2]
        d1=d1.astype(int)
        d2=d2.astype(int)
        #print("type of d1:",type(d1))
        #AND = lambda x, y: x & y
        def AND(x,y):
            #print("x:",x,x.dtype)
            #print("y:",y,y.dtype)
            return x&y
        #OR  = lambda x, y: x | y
        def OR(x,y):
            #print("x:",x,x.dtype)
            #print("y:",y.dtype)
            return x|y
        #XOR = lambda x, y: x ^ y
        def XOR(x,y):
            #print("x:",x,x.dtype)
            #print("y:",y,y.dtype)
            return x^y
        #NOT = lambda x: 1 ^ x
        def NOT(x):
            #print("x:",x,x.dtype)
            return x^1
        
        if op == 'x^y':
            res = XOR(d1, d2)
        elif op == 'x=y':
            res = NOT(XOR(d1, d2))
        else:
            assert ('&' in op) or ('|' in op)
            if '~x' in op: d1 = NOT(d1)
            if '~y' in op: d2 = NOT(d2)
            if '&' in op: res = AND(d1, d2)
            else: res = OR(d1, d2)
            
        res = np.array([res])
        data = np.concatenate((data, res.T), axis=1)
        return data
        
    def toBlif(self, fn, useBest=True):
        getFeatName = lambda x: 'x_' + str(x)
        getNodeName = lambda x: 'n_' + str(x)
        
        def fringeFeatsExtract(fLst):
            feats = self.__getFringeFeats__(useBest)
            n = self.nFeats
            for id1, id2, op in feats:
                names = [getFeatName(id1), getFeatName(id2), getFeatName(n)]
                if op == 'x^y':
                    pats = ['10 1', '01 1']
                elif op == 'x=y':
                    pats = ['00 1', '11 1']
                elif '&' in op:
                    b0 = '0' if ('~x' in op) else '1'
                    b1 = '0' if ('~y' in op) else '1'
                    pats = [b0 + b1 + ' 1']
                else:
                    assert '|' in op
                    b0 = '1' if ('~x' in op) else '0'
                    b1 = '1' if ('~y' in op) else '0'
                    pats = [b0 + b1 + ' 0']
                fLst.append((names, pats))
                n += 1
            
        def skTreeExtract(dtree_, nId, nLst,constructedNodes):
            if dtree_.var[nId] != -1: # decision node
                if nId not in constructedNodes:
                    constructedNodes.add(nId)
                else: return
                skTreeExtract(dtree_, dtree_.leftChild[nId], nLst,constructedNodes)    # 0-branch
                skTreeExtract(dtree_, dtree_.rightChild[nId], nLst,constructedNodes)   # 1-branch
                nNd = getNodeName(nId)
                nC0 = getNodeName(dtree_.leftChild[nId])
                nC1 = getNodeName(dtree_.rightChild[nId])
                nCtrl = getFeatName(dtree_.var[nId])
                names = [nC0, nC1, nCtrl, nNd]
                pats = ['-11 1', '1-0 1']
            else: # leaf node
                if nId not in constructedNodes:
                    constructedNodes.add(nId)
                else: return
                names = [getNodeName(nId)]
                val = dtree_.pred[nId]
                pats = ['1'] if (val == 1) else []
            nLst.append((names, pats))  
            
        dtree = self.dtreeBest if useBest else self.dtree
        dtree_ = dtree
        
        nLst = []
        constructedNodes=set()
        fringeFeatsExtract(nLst)
        skTreeExtract(dtree_, 0, nLst,constructedNodes)
        
        fp = open(fn, 'w')
        fp.write('.model FringeTree\n')
        fp.write('.inputs ')
        fp.write(' '.join([getFeatName(i) for i in range(self.nFeats)]))
        fp.write('\n.outputs y\n')
        for names, pats in nLst:
            fp.write('.names {}\n'.format(' '.join(names)))
            for pat in pats:
                fp.write(pat + '\n')
        fp.write('.names {} y\n1 1\n.end\n'.format(getNodeName(0)))
        fp.close()

    def saveWOData(self,filename):
        self.dtree.data=None
        self.dtree.label=None
        self.dtreeBest.data=None
        self.dtreeBest.label=None
        with open(filename,'wb') as f:
            pickle.dump(self,f)