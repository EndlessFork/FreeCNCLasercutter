#!/bin/env python

import sys
import pyqtgraph

X=[]
Y=[]

for l in sys.stdin:
    if l.startswith("Stepper is at"):
        x,y,z = l[13:].split(',')
        X.append(int(x))
        Y.append(int(y))

import pyqtgraph
w = pyqtgraph.plot(X,Y, symbol='o')
pyqtgraph.QtGui.QApplication.exec_()
