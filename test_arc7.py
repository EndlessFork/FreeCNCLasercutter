
import sys, traceback
from math import sqrt

def print_exc_plus():
    """
    Print the usual traceback information, followed by a listing of all the
    local variables in each frame.
    """
    tb = sys.exc_info()[2]
    while 1:
        if not tb.tb_next:
            break
        tb = tb.tb_next
    stack = []
    f = tb.tb_frame
    while f:
        stack.append(f)
        f = f.f_back
    stack.reverse()
    traceback.print_exc()

    for frame in stack:
        print
        print "Frame %s in %s at line %s" % (frame.f_code.co_name,
                                             frame.f_code.co_filename,
                                             frame.f_lineno)
        for key, value in frame.f_locals.items():
            print "%20s = " % key,
            #We have to be careful not to cause a new error in our error
            #printer! Calling str() on an unknown object could cause an
            #error we don't want.
            try:
                print value
            except:
                print "<ERROR WHILE PRINTING VALUE>"



# kernel method
# the kernel makes exactly one step.-
# computation depends on the octand of the point
# cw arcs are the same as ccw, but with flipped Y

# changed dX from 2X-1 to 2*X (same for Y)
# -> init err with one less to simplify comparisons
# simplify!

# add speed factor calculation

def octant(X, Y):
    if X > 0 and Y >= 0:
        return 0 if X > Y else 1
    elif Y > 0 and X <= 0:
        return 2 if Y > -X else 3
    elif X < 0 and Y <= 0:
        return 4 if X < Y else 5
    elif Y < 0 and X >= 0:
        return 6 if Y < -X else 7
    else:
        raise ValueError("can not determine octant!")

class Data(object):
    # offset for real objects, not really needed here if X,Y are zero based
    XM = 0
    YM = 0
    # rendering coordinates (real coordinates)
    X=0
    Y=0
    # rendering error: init to R**2 -X**2 -Y**2 -1
    err = 0
    # increment of rendering error
    dX = 0
    dY = 0

    # to choose the right kernel
    octant = 0
    ccw = True # mathematical

    # amount of points to trace
    num_steps = 0

    # final point, if given
    endX = 0
    endY = 0

    # current speedfactor
    # =1 for hor/ver moves, 1.41... for diagonal ones, else inbetween

    def __init__(self,**kwds):
        self.__dict__.update(kwds)
        self.octant = octant(self.X, self.Y) if self.ccw else 8+octant(self.X, -self.Y)
        self.dX = 2*self.X
        self.dY = 2*self.Y
        self.err = self.R**2 - self.X**2 - self.Y **2 -1
        del self.__dict__['R']

    def __repr__(self):
        return 'Data('+', '.join('%s=%r' % it for it in self.__dict__.items()) + ')'

# octants 0,1 are in Q1, 2,3 in Q2 etc....
# if cw: add 8
# if ccw: o=0, 1, 2, 3,...7, 0, 1, 2....
# else: o=8, 9 ,...15, 8, 9...


def kernel(data):
    """returns True until the final point is reached, or num_steps gets zero
    else the arc is done and we return False"""
    X = data.X
    Y = data.Y

    #~ data.octant = octant(X,Y) if data.ccw else 8+octant(X,-Y)

    def step_X(data, negative):
        if negative:
            data.err = data.err + data.dX -1
            data.dX -= 2
            data.X = data.X -1 # STEP
        else:
            data.X = data.X +1 # STEP
            data.dX +=2
            data.err = data.err - data.dX +1

    def step_Y(data, negative):
        if negative:
            data.err = data.err + data.dY -1
            data.dY -= 2
            data.Y = data.Y -1 # STEP
        else:
            data.Y = data.Y +1 # STEP
            data.dY += 2
            data.err = data.err - data.dY +1

    # big 'case' switch
    if data.octant == 0:
        # octant 1: X > 0, Y >= 0, X > Y
        # always y+, sometimes x-
        if data.err <= data.dY -data.X:
            step_X(data, True)
        step_Y(data,False)

        if data.Y >= data.X:
            data.octant += 1

    elif data.octant == 1:
        # octant 2: X > 0, Y > 0, X <= Y
        # always x-, sometimes y+
        if data.err > data.Y - data.dX:
            step_Y(data, False)
        step_X(data, True)

        if data.X == 0:
            data.octant += 1

    elif data.octant == 2:
        # octant 3: X <= 0, Y > 0, -X <= Y
        # always x-, sometimes y-
        if data.err <= -(data.dX + data.Y):
            step_Y(data, True)
        step_X(data, True)

        if data.X <= -data.Y:
            data.octant +=1

    elif data.octant == 3:
        # octant 4: X < 0, Y > 0, -X > Y
        # always y-, sometimes x-
        if data.err > -(data.dY + data.X):
            step_X(data, True)
        step_Y(data,True)

        if data.Y == 0:
            data.octant +=1

    elif data.octant == 4:
        # octant 5: X < 0, Y <= 0, -X > -Y
        # always y-, sometimes x+
        if data.err <= data.X - data.dY:
            step_X(data, False)
        step_Y(data,True)

        if data.X >= data.Y:
            data.octant +=1

    elif data.octant == 5:
        # octant 6: X < 0, Y < 0, -X <= -Y
        # always x+, sometimes y-
        if data.err > data.dX - data.Y:
            step_Y(data, True)
        step_X(data,False)

        if data.X == 0:
            data.octant +=1

    elif data.octant == 6:
        # octant 7: X >= 0, Y < 0, X <= -Y
        # always x+, sometimes y+
        if data.err <= data.dX + data.Y:
            step_Y(data, False)
        step_X(data,False)

        if data.X >= -data.Y:
            data.octant +=1

    elif data.octant == 7:
        # octant 8: X > 0, Y < 0, X > -Y
        # always y+, sometimes x+
        if data.err > data.X + data.dY:
            step_X(data, False)
        step_Y(data,False)

        if data.Y == 0:
            data.octant = 0

    elif data.octant == 8:
        # octant -8: X > 0, Y <= 0, X > -Y
        # always y-, sometimes x-
        if data.err <= -(data.X + data.dY):
            step_X(data, True)
        step_Y(data, True)

        if data.Y <= -data.X:
            data.octant += 1

    elif data.octant == 9:
        # octant -7: X > 0, Y < 0, X<= -Y
        # always x-, sometimes y-
        if data.err > -(data.dX + data.Y):
            step_Y(data, True)
        step_X(data, True)

        if data.X == 0:
            data.octant +=1

    elif data.octant == 10:
        # octant -6: X <= 0, Y < 0, -X <= -Y
        # always x-, sometimes y+
        if data.err <= data.Y - data.dX:
            step_Y(data, False)
        step_X(data, True)

        if data.X<=data.Y:
            data.octant +=1

    elif data.octant == 11:
        # octant -5: X < 0, Y < 0, -X > -Y
        # always y+, sometimes x-
        if data.err > data.dY - data.X:
            step_X(data, True)
        step_Y(data, False)

        if data.Y == 0:
            data.octant +=1

    elif data.octant == 12:
        # octant -4: X < 0, Y >= 0, -X > Y
        # always y+, sometimes x+
        if data.err <= data.X + data.dY:
            step_X(data, False)
        step_Y(data, False)

        if data.Y > -data.X:
            data.octant +=1

    elif data.octant == 13:
        # octant -3: X < 0, Y > 0, -X < Y
        # always x+, sometimes y+
        if data.err > data.dX + data.Y:
            step_Y(data, False)
        step_X(data,False)

        if data.X == 0:
            data.octant += 1

    elif data.octant == 14:
        # octant -2: X >= 0, Y > 0, X < Y
        # always x+, sometimes y-
        if data.err <= data.dX - data.Y:
            step_Y(data, True)
        step_X(data,False)

        if data.X >= data.Y:
            data.octant +=1

    elif data.octant == 15:
        # octant -1: X > 0, Y > 0, X >= Y
        # always y-, sometimes x+
        if data.err > data.X - data.dY:
            step_X(data, False)
        step_Y(data, True)

        if data.Y == 0:
            data.octant = 8

    else:
        raise ValueError("undefined octant %d!" %octant)

    data.speedfactor = sqrt(data.X**2 + data.Y**2) / max(abs(data.X), abs(data.Y))
    assert 1<= data.speedfactor <= 1.42

    assert data.dX == 2*data.X
    assert data.dY == 2*data.Y
    should_be_octant = octant(data.X, data.Y) if data.ccw else 8+octant(data.X, -data.Y)
    assert data.octant == should_be_octant

    data.num_steps -= 1

    print data

    if data.endX or data.endY:
        if (data.endX == data.X) and (data.endY == data.Y):
            return False
    return data.num_steps != 0

X=[100]
Y=[0]

dl = [Data(X=100,Y=0, R=100, endX=100, endY=0,ccw=True, num_steps=144*4),
        Data(X=95,Y=0, R=100, endX=100, endY=0,ccw=True, num_steps=144*4),
        Data(X=95,Y=0, R=90, endX=90, endY=0,ccw=True, num_steps=144*4),
        Data(X=90,Y=0, R=90, endX=90, endY=0,ccw=True, num_steps=144*4),
        Data(X=90,Y=0, R=90, endX=90, endY=0,ccw=False, num_steps=144*4),
        Data(X=95,Y=0, R=90, endX=90, endY=0,ccw=False, num_steps=144*4),
        Data(X=95,Y=0, R=100, endX=100, endY=0,ccw=False, num_steps=144*4),
        Data(X=100,Y=0, R=100, endX=100, endY=0,ccw=False, num_steps=144*4),
]

#~ import pdb; pdb.set_trace()
try:
    for d in dl:
        while kernel(d):
            X.append(d.X)
            Y.append(d.Y)
except:
    print_exc_plus()

import pyqtgraph
from pyqtgraph import plot

w = pyqtgraph.plot(X,Y, symbol='o')
pyqtgraph.QtGui.QApplication.exec_()
