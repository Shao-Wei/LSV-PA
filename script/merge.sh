#!/bin/bash
target=$1 #"c1908.3000"
benchmarkFile=$2 #"benchmarks/c1908.blif"
timeLimit=7200

pathName="samples/$target"
baseFile="samples/$target/$target.c00.v01"
samplesFile="samples/$target/$target.c00.v01.samples.pla"
logFile="samples/$target/$target.csv"

# dt
fskDT="1"
subName="dt"
aMerge="samples/$target/$target.c00.v01.aMerge.$subName.blif"
aResubN0="samples/$target/$target.c00.v01.aResubN0.$subName.blif"
aRewrite="samples/$target/$target.c00.v01.aRewrite.$subName.blif"
aResubN3="samples/$target/$target.c00.v01.aResubN3.$subName.blif"

commandLine1="ntkconstruct -d $fskDT -i 0 -o $logFile -v -s $benchmarkFile $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
commandLine2="ntkconstruct -d $fskDT -i 0 -o $logFile -v                   $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
timeout $timeLimit ./abc -c "$commandLine1"
timeout $timeLimit ./abc -c "$commandLine2"

# dgdo
fskDT="2"
subName="dgdo"
aMerge="samples/$target/$target.c00.v01.aMerge.$subName.blif"
aResubN0="samples/$target/$target.c00.v01.aResubN0.$subName.blif"
aRewrite="samples/$target/$target.c00.v01.aRewrite.$subName.blif"
aResubN3="samples/$target/$target.c00.v01.aResubN3.$subName.blif"

commandLine1="ntkconstruct -d $fskDT -i 0 -o $logFile -v -s $benchmarkFile $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
commandLine2="ntkconstruct -d $fskDT -i 0 -o $logFile -v                   $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
timeout $timeLimit ./abc -c "$commandLine1"
timeout $timeLimit ./abc -c "$commandLine2"

# fringe
fskDT="3"
subName="fringe"
aMerge="samples/$target/$target.c00.v01.aMerge.$subName.blif"
aResubN0="samples/$target/$target.c00.v01.aResubN0.$subName.blif"
aRewrite="samples/$target/$target.c00.v01.aRewrite.$subName.blif"
aResubN3="samples/$target/$target.c00.v01.aResubN3.$subName.blif"

commandLine1="ntkconstruct -d $fskDT -i 0 -o $logFile -v -s $benchmarkFile $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
commandLine2="ntkconstruct -d $fskDT -i 0 -o $logFile -v                   $samplesFile $pathName $baseFile; st; ps; signalmerge -o $logFile -v $samplesFile; ps; write_blif $aMerge; ntkresub -o $logFile -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -o $logFile -v $samplesFile; ps; write_blif $aRewrite"
timeout $timeLimit ./abc -c "$commandLine1"
timeout $timeLimit ./abc -c "$commandLine2"

