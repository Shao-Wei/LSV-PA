#!/bin/bash
target="c1908.3000"
benchmarkFile="benchmarks/c1908.blif"

pathName="samples/$target"
baseFile="samples/$target/$target.c00.v01"
samplesFile="samples/$target/$target.c00.v01.samples.pla"

aMerge="samples/$target.c00.v01.aMerge.blif"
aResubN0="samples/$target.c00.v01.aResubN0.blif"
aRewrite="samples/$target.c00.v01.aRewrite.blif"
aResubN3="samples/$target.c00.v01.aResubN3.blif"

commandLine1="ntkconstruct -d -v -s $benchmarkFile $samplesFile $pathName $baseFile; st; ps; signalmerge -v $samplesFile; ps; write_blif $aMerge; ntkresub -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -v $samplesFile; ps; write_blif $aRewrite; ntkresub -v -N 0 $samplesFile; ps; write_blif $aResubN3"
commandLine2="ntkconstruct -d -v                   $samplesFile $pathName $baseFile; st; ps; signalmerge -v $samplesFile; ps; write_blif $aMerge; ntkresub -v -N 0 $samplesFile; ps; write_blif $aResubN0; aigrewrite -v $samplesFile; ps; write_blif $aRewrite; ntkresub -v -N 0 $samplesFile; ps; write_blif $aResubN3"
./abc -c "$commandLine1"
./abc -c "$commandLine2"
