#! /usr/bin/env python3 
# Dataset Example 
# This assumes you have the environmental PUEO_ROOT_DATA pointing somewhere reasonable

import ROOT
import sys
import numpy as np
# I think this has to go after ROOT for GUI event loops not to get messed up?
from matplotlib import pylab as pl

run = 813 
entry = 10

if len(sys.argv) > 1: 
    run = int(sys.argv[1])
if len(sys.argv) > 2: 
    entry = int(sys.argv[2])

ROOT.gSystem.Load("libpueoEvent.so") # assume in LD_LIBRARY_PATH (or DYLD_LIBRARY_PATH if you're using a mac for some reason) 



# create a dataset using MC data

pueo = ROOT.pueo # shortcut
d = pueo.Dataset(run)


d.getEntry(entry) 

# get channel number for  phi 10, top ring,  , vpol
chan = pueo.GeomTool.Instance().getChanIndexFromRingPhiPol(pueo.ring.ring_t.kTopRing, 10, pueo.pol.pol_t.kVertical)

v = d.useful().volts[chan] 
t = d.useful().dt[chan] * np.arange(d.useful().volts[chan].size())
pl.plot(t,v)
pl.xlabel("ns"); 
pl.ylabel("volts") 
pl.title("Run %d, Entry %d, Ant 1001V" % (run,entry))
pl.show()


