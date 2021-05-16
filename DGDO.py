import numpy as np
import math
import copy
import sys
import pickle

class DGDO():
    def __init__(self,nVar,mergeCriteria,mergeSampleMethod,threshold):
        #DG spec
        self.nVar=nVar
        self.mergeCriteria=mergeCriteria#three types: (1) conf<n, (2) iden>n (3) idenGConf or noMerge
        self.mergeSampleMethod=mergeSampleMethod#two types: (1)"pred":run predict from the node and see if the acc>threshold. (2)"data": compare the numConf and numIden of data on the two nodes
        #DG stat
        self.nodeCount=0
        self.threshold=threshold

        #DG inner data
        self.var=[]
        self.cube=[]#warn! seems unused!
        self.data=[]
        self.label=[]
        self.unUsedVar=[]#a list of lists, each list contains the unused variables for the node, noting that the decision var chosen is in the list
        self.parent=[]# a list of lists, each list contains the nodeInds of its parent nodes
        self.branch=[]#records which branch from the parent node 
        self.leftChild=[]# size=# of nodes, records left child of node i, if store -2: child is 0, if -3: child is 1
        self.rightChild=[]# size=# of nodes, records right child of node i, if store -2: child is 0, if -3: child is 1
        self.pred=[]# size=# of nodes, records prediction of node i, -1 means non-leaf node. Possible values={-1,0,1}
        self.level=[]#the root is of level 0, seems unused

    def fit(self,data,label):
        def computeGain(data,label,var):
            out0=np.sum(label==0)#num of 0 in parent node
            out1=label.size-out0#num of 1 in parent node
            p0=out0/label.size
            p1=out1/label.size
            oriInfo=-1*((p0*math.log2(p0) if p0!=0 else 0)+ (p1*math.log2(p1) if p1!=0 else 0))

            #print("data:",data)
            in0out0=np.sum((data[:,var]==0)&(label==0))
            in0out1=np.sum((data[:,var]==0)&(label==1))
            in1out0=np.sum((data[:,var]==1)&(label==0))
            in1out1=np.sum((data[:,var]==1)&(label==1))
            p00=in0out0/(in0out0+in0out1+1e-8)
            p01=1-p00
            p10=in1out0/(in1out0+in1out1+1e-8)
            p11=1-p10

            n0=in0out0+in0out1
            n1=in1out0+in1out1
            nAll=n1+n0
            
            #ID3
            info0=(n0/nAll)*-1*((p00*math.log2(p00) if p00!=0 else 0) + (p01*math.log2(p01) if p01!=0 else 0))
            info1=(n1/nAll)*-1*((p10*math.log2(p10) if p10!=0 else 0) + (p11*math.log2(p11) if p11!=0 else 0))
            return oriInfo-info0-info1

            # #C4.5
            # splitInfo0=-1*(n0/nAll)*math.log2(n0/nAll) if n0!=0 else 0
            # splitInfo1=-1*(n1/nAll)*math.log2(n1/nAll) if n1!=0 else 0
            # return (oriInfo-info0-info1)/(splitInfo0+splitInfo1+1e-8)

        def findVar(data,label,nodeInd):
            maxVar=-1
            maxGain=-10000
            for v in self.unUsedVar[nodeInd]:
                gain=computeGain(data,label,v)
                if gain>maxGain:
                    maxVar=v
                    maxGain=gain
            return maxVar
        def splitData(data,label,var):
            mask0=data[:,var]==0
            mask1=data[:,var]==1
            data0=data[mask0,:]
            #data0=np.delete(data0,var,1)
            data1=data[mask1,:]
            #data1=np.delete(data1,var,1)
            label0=label[mask0]
            label1=label[mask1]
            return data0,label0,data1,label1
        def createRoot(parentInd,branch,data,label):#things abour var are not set
            self.nodeCount+=1
            if branch!=None:
                self.level.append(self.level[parentInd]+1)

                #print(parentInd)
                newUnusedVar=copy.deepcopy(self.unUsedVar[parentInd])
                #print("var to del:",self.var[parentInd])
                #newUnusedVar.remove(self.var[parentInd])
                try:
                    newUnusedVar.remove(self.var[parentInd])
                except ValueError:
                    print("parentInd:",parentInd)
                    print("var[parentInd]:",self.var[parentInd])
                    errorMsg=f"parentInd:{parentInd}\nvar[parentInd]:{self.var[parentInd]}"
                    sys.exit(errorMsg)
                self.unUsedVar.append(newUnusedVar)
                newCube=copy.deepcopy(self.cube[parentInd])
                newCube.append(self.var[parentInd]+1 if branch==1 else -(self.var[parentInd]+1))
                self.cube.append(newCube)
            else:
                self.level.append(0)
                self.unUsedVar.append([i for i in range(self.nVar)])
                self.cube.append([])
            self.data.append(data)
            self.label.append(label)
            self.parent.append([parentInd])
            self.branch.append([branch])
            self.pred.append(-1)
            #the followings haven't set to right values
            self.leftChild.append(-1)
            self.rightChild.append(-1)

        def createLeaf(parentInd,branch,leafValue):
            #print("construct leaf with nodeInd:",self.nodeCount)
            self.nodeCount+=1
            self.var.append(-1)
            self.cube.append(self.cube[parentInd]+([self.var[parentInd]+1] if branch==1 else [-(self.var[parentInd]+1)]))
            self.data.append(None)
            self.label.append(None)
            self.unUsedVar.append(None)
            self.parent.append([parentInd])
            self.branch.append([branch])
            self.leftChild.append(None)
            self.rightChild.append(None)
            self.pred.append(leafValue)
            self.level.append(self.level[parentInd]+1)

        def canMerge(data,label,unUsedVar,node2):
            print("unused variable:",unUsedVar)
            print("self.unUsedVar[node2]:",self.unUsedVar[node2])
            combUnUsedVar=set(unUsedVar) & (set(self.unUsedVar[node2]))#-{self.var[self.parent[node2][0]]})
            combUnUsedVar=list(combUnUsedVar)
            if len(combUnUsedVar)==0:
                return False
            #print("comb unused var:",combUnUsedVar)
            # print("node2:",node2)
            # print("self.data[node2]:",self.data[node2])
            # print("self.data[node2][combUnUsedVar]:",self.data[node2][:,combUnUsedVar])
            # print("data[combUnUsedVar]:",data[:,combUnUsedVar])
            # print("data shape:",data.shape)
            combData=np.concatenate((data[:,combUnUsedVar],self.data[node2][:,combUnUsedVar]))
            print("combData:",combData)
            combLabel=np.concatenate((label,self.label[node2]))
            #print("combLabel:",combLabel)
            sortedInd=np.lexsort(np.rot90(combData))
            #print("sortedInd:",sortedInd)
            numConf=0
            numIden=0
            i=0
            while i<sortedInd.shape[0]-1:
                ind=sortedInd[i]
                #print("combData[ind]:",combData[ind])
                #print("combData[ind+1]:",combData[sortedInd[i+1]])
                if (combData[ind]==combData[sortedInd[i+1]]).all() and combLabel[ind]==combLabel[sortedInd[i+1]]:
                    numIden+=1
                    i+=2
                elif (combData[ind]==combData[sortedInd[i+1]]).all() and combLabel[ind]!=combLabel[sortedInd[i+1]]:
                    numConf+=1
                    i+=2
                else:
                    i+=1
            print("numConf:",numConf)
            print("numIden",numIden)
            if self.mergeCriteria.lower()=="idengconf":
                return numIden>numConf
            elif self.mergeCriteria[:4]=="conf":
                th=int(self.mergeCriteria[5:])
                return numConf<th
            elif self.mergeCriteria[:4]=="iden":
                th=int(self.mergeCriteria[5:])
                return numIden>th
            elif self.mergeCriteria=="noMerge":
                return False
            else:
                print("Warning! Cannot parse the merge criteria. Did not perform merging!")
                return False
            #return numIden>15 and numConf<1


        def canMerge_classify(data,label,node2):
            # print("chk node2:",node2)
            # print("data:",data)
            # print("label:",label)
            pred=self.predict(data,node2)
            count=sum(pred==label)
            acc=count/label.shape[0]
            return acc>self.threshold   
        
        def findMergableNode(data,label,unUsedVar,candNodes):
            # print(f"starts to find mergable nodes from {len(candNodes)} candidates.")
            # print("candNodes",candNodes)
            # print("with data:",data)
            dataOnNode=dict()#(data,correct or not), record the data and label for the node with worresponding branch
            for cand in candNodes:
                # print(f"considering cand {cand}")
                if self.mergeSampleMethod=="pred":
                    if self.pred[cand]>=0:#leaf, gather numCorrect by predicting
                        if (label==self.pred[cand]).sum()/label.shape[0]>=self.threshold:#mergable, return
                            #print("found a mergable node on leaf")
                            return cand
                        #can't merge, have to record data and label for corresponding branch
                        # print("data shape:",data.shape)
                        # print("label shape:",label.shape)
                        assert(data.shape[0]==label.shape[0])
                        resData=[]
                        resCorrect=[]
                        for i,p in enumerate(self.parent[cand]):
                            parentVar=self.var[p]
                            mask=data[:,parentVar]==self.branch[cand][i]
                            # print("data: ",data)
                            # print("label shape:",label.shape)
                            # print("mask:",mask)
                            correct=(label[mask]==self.pred[cand])
                            resData.append(data[mask,:])
                            resCorrect.append(correct)
                            # print("correct:",correct)
                        dataOnNode[cand]=(resData,resCorrect)
                    else: #non-leaf nodes, use dataOnNode to get numCorrect
                        child1=self.leftChild[cand]
                        child2=self.rightChild[cand]
                        #print(f"Child1:{child1},Child2:{child2}")
                        parentInd1=self.parent[child1].index(cand)#cand is which parent of child1
                        parentInd2=self.parent[child2].index(cand)#cand is which parent of child2
                        # print("parentInd1:",parentInd1)
                        # print("parentInd2:",parentInd2)
                        # print("dataOnNode[child1][1]:",dataOnNode[child1][1])
                        # print("dataOnNode[child2][1]:",dataOnNode[child2][1])
                        numCorrect=dataOnNode[child1][1][parentInd1].sum()+dataOnNode[child2][1][parentInd2].sum()
                        #print("correct1:",dataOnNode[child1][0][parentInd1],dataOnNode[child1][1][parentInd1])
                        #print("correct2:",dataOnNode[child2][0][parentInd2],dataOnNode[child2][1][parentInd2])
                        # print("numCorrect:",numCorrect)
                        # print("label shape:",label.shape)
                        # print("ratio:",numCorrect/label.shape[0])
                        if numCorrect/label.shape[0]>=self.threshold:
                            #print("found a mergable node in internal nodes")
                            return cand
                        #can't merge record data and label
                        allData=[]
                        allCorrect=[]
                        for i,p in enumerate(self.parent[cand]):
                            
                            parentVar=self.var[p]
                            data1=dataOnNode[child1][0][parentInd1]
                            # print("child1:",child1)
                            # print("parentInd1:",parentInd1)
                            #print(dataOnNode[child1][0])
                            if len(data1.shape)==1:
                                data1=data1.reshape((1,-1))
                            #print("self.branch[cand]:",self.branch[cand])
                            #print("data1:",data1)
                            mask1=data1[:,parentVar]==self.branch[cand][i]
                            data1=data1[mask1,:]
                            #print("correct 1:",dataOnNode[child1][1])
                            correct1=dataOnNode[child1][1][parentInd1][mask1]
                            data2=dataOnNode[child2][0][parentInd2]
                            if len(data2.shape)==1:
                                data2=data2.reshape((1,-1))
                            mask2=data2[:,parentVar]==self.branch[cand][i]
                            data2=data2[mask2,:]
                            correct2=dataOnNode[child2][1][parentInd2][mask2]
                            dataMerged=np.concatenate((data1, data2), axis=0)
                            correct=np.concatenate((correct1,correct2), axis=0)
                            allData.append(dataMerged)
                            allCorrect.append(correct)
                        dataOnNode[cand]=(allData,allCorrect)
                        
                        dataOnNode[child1][0][parentInd1]=None
                        dataOnNode[child1][1][parentInd1]=None
                        dataOnNode[child2][0][parentInd2]=None
                        dataOnNode[child2][1][parentInd2]=None
                    



                    # if canMerge_classify(data,label,cand):
                    #    #print("found a mergable node")
                    #    return cand


                elif self.mergeSampleMethod=="data":
                    if canMerge(data,label,unUsedVar,cand):
                        print("found a mergable node")
                        return cand
            return -1
                

        def recursion(nodeInd,data,label,parentInd,branch,candMerge):
            assert(nodeInd==self.nodeCount)
            #print("recursion on node:",nodeInd)
            if parentInd!=-1 and (len(self.unUsedVar[parentInd])==0):# or label.shape[0]<=2):
                leafValue=1 if label.sum()/(label.shape[0]+1e-8)>0.5 else 0
                #print("going to construct leaf because no unusedVar")
                createLeaf(parentInd,branch,leafValue)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)#-1 is because createLeaf added a node
                #print("candMerge:",candMerge)
                return nodeInd,candMerge
            
            createRoot(parentInd,branch,data,label)
            #print("nodeInd:",nodeInd)    
            #print("unUsed var after create root:",self.unUsedVar)
            var=findVar(data,label,nodeInd)
            self.var.append(var)
            lData,lLabel,rData,rLabel=splitData(data,label,var)
            #print("splited data:",lData,lLabel,rData,rLabel)
            # trL=lLabel.sum()/lLabel.shape[0]
            # trR=rLabel.sum()/rLabel.shape[0]

            #construct left subtree
            ##0 special cases, link to a leaf
            ##0_1 no data remaining fro left child
            if lLabel.shape[0]==0:
                self.leftChild[nodeInd]=self.nodeCount
                #print("going to construct leaf because no lLabel")
                createLeaf(nodeInd,0,0 if label.sum()/label.shape[0]<0.5 else 1)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            ##0_2 all labels are 0
            elif lLabel.sum()==0:
                self.leftChild[nodeInd]=self.nodeCount
                #print("going to construct leaf because lLabel all zero")
                createLeaf(nodeInd,0,0)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            ##0_3 all labels are 1
            elif lLabel.sum()==lLabel.shape[0]:
                self.leftChild[nodeInd]=self.nodeCount
                #print("going to construct leaf because lLabel all 1")
                createLeaf(nodeInd,0,1)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            else:
                ##1. search for mergable nodes if doMerge
                unUsedVar=copy.deepcopy(self.unUsedVar[nodeInd])
                #unUsedVar.remove(var)
                if self.mergeCriteria!="noMerge" and len(candMerge)>0 and lLabel.shape[0]>=0:
                    #print("going to find mergable node for node:",nodeInd," (left branch)")
                    foundMergableNode=findMergableNode(lData,lLabel,unUsedVar,candMerge)
                else:
                    foundMergableNode=-1
                if foundMergableNode!=-1:
                    ##1_1 found mergable node, merge them
                    
                    self.leftChild[nodeInd]=foundMergableNode
                    self.parent[foundMergableNode].append(nodeInd)
                    self.branch[foundMergableNode].append(0)
                else:
                    ##1_2 if no mergable nodes found, construct left subtree
                    self.leftChild[nodeInd],addedCand=recursion(self.nodeCount,lData,lLabel,nodeInd,0,candMerge)
                    # print("merge mergeCand in node:",nodeInd)
                    # for c in addedCand:
                    #     assert(c not in candMerge)
                    #     if c not in candMerge:
                    #         candMerge.append(c)

            #construct right subtree
            ##0 special cases, link to a leaf
            ##0_1 no data remaining fro right child
            if rLabel.shape[0]==0:
                self.rightChild[nodeInd]=self.nodeCount
                #print("going to construct leaf because no rLabel")
                createLeaf(nodeInd,1,0 if label.sum()/label.shape[0]<0.5 else 1)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            ##0_2 all labels are 0
            elif rLabel.sum()==0:
                self.rightChild[nodeInd]=self.nodeCount
                #print("going to construct leaf because lLabel all 0")
                createLeaf(nodeInd,1,0)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            ##0_3 all labels are 1
            elif rLabel.sum()==rLabel.shape[0]:
               # print("going to construct leaf because rLabel all 1")
                self.rightChild[nodeInd]=self.nodeCount
                createLeaf(nodeInd,1,1)
                assert(self.nodeCount-1 not in candMerge)
                #if self.nodeCount-1 not in candMerge:
                candMerge.append(self.nodeCount-1)
                #print("candMerge:",candMerge)
            else:
                ##1. search for mergable nodes if doMerge
                unUsedVar=copy.deepcopy(self.unUsedVar[nodeInd])
                #unUsedVar.remove(var)
                if self.mergeCriteria!="noMerge" and len(candMerge)>0 and rLabel.shape[0]>=0:
                    #print("goint to find mergable node for node:",nodeInd," (right branch)")
                    foundMergableNode=findMergableNode(rData,rLabel,unUsedVar,candMerge)
                else:
                    foundMergableNode=-1
                if foundMergableNode!=-1:
                    ##1_1 found mergable node, merge them
                    self.rightChild[nodeInd]=foundMergableNode
                    self.parent[foundMergableNode].append(nodeInd)
                    self.branch[foundMergableNode].append(1)
                else:
                    ##1_2 if no mergable nodes found, construct right subtree
                    self.rightChild[nodeInd],addedCand=recursion(self.nodeCount,rData,rLabel,nodeInd,1,candMerge)
                    # for c in addedCand:
                    #     assert(c not in candMerge)
                    #     if c not in candMerge:
                    #         candMerge.append(c)
            assert(nodeInd not in candMerge)
            if nodeInd not in candMerge:
                candMerge.append(nodeInd)
            return nodeInd,candMerge
        
        nodeCandidatesToMerge=[]
        recursion(0,data,label,-1,None,nodeCandidatesToMerge)
        #print("candMerge after fitting:",nodeCandidatesToMerge)


    def predict(self,data,root=0):
        #print("predicting...")
        results=[]
        for d in data:
            #print("predicting:",d)
            nodeInd=root
            while self.pred[nodeInd]<0:
                var=self.var[nodeInd]
                #print("considering var:",var)
                if d[var]==0:
                    nodeInd=self.leftChild[nodeInd]
                else:
                    nodeInd=self.rightChild[nodeInd]
            results.append(self.pred[nodeInd])
        return results

    def saveWOData(self,fileName):
        self.data=None
        self.label=None
        with open(fileName,'wb') as f:
                pickle.dump(self,f)