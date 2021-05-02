#!/bin/bash
target="c1908.3000.c00.v01"
benchmarkFile="benchmarks/c1908.blif"

baseFile="samples/$target"
samplesFile="samples/$target.samples.pla"

#aResyn2="samples/$target.aResyn2.blif"
aMerge="samples/$target.aMerge.blif"
aResubN0="samples/$target.aResubN0.blif"
aResubN3="samples/$target.aResubN3.blif"
commandLine="ntkconstruct -v -s $benchmarkFile $samplesFile $baseFile; st; ps; signalmerge -v $samplesFile; ps; write_blif $aMerge; ntkresub -v -N 0 $samplesFile; ps; write_blif $aResubN0; ntkresub -v -N 3 $samplesFile; ps; write_blif $aResubN3"

./abc -c "$commandLine"

