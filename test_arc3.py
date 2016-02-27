
# kernel method
# the kernel makes exactly one step.- 
# computation depends on the octand of the point
# cw arcs are the same as ccw, but with flipped Y


class Data(object):
    # offset for real objects
    XM = 0
    YM = 0
    # rendering coordinates (real coordinates)
    X=0
    Y=0
    # rendering error
    err_par = 0 # X or Y step
    err_dia = 0 # diagonal step
    
    # may be removed: desired Radius
    R = 0
    
    # to choose the right kernel
    octant = 0
    ccw = True # mathematical
    
    # amount of points to trace
    num_steps = 0
    
    # final point, if given
    endX = 0
    endY = 0
    
    def __init__(self,**kwds):
        self.__dict__.update(kwds)
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

    data.octant = octant(X,Y) if data.ccw else 8+octant(X,-Y)
    
    def step_X(data, negative):
        if negative:
            data.err = data.err + 2*data.X -1
            data.X = data.X -1
        else:
            data.err = data.err - 2*data.X -1
            data.X = data.X +1

    def step_Y(data, negative):
        if negative:
            data.err = data.err + 2*data.Y -1
            data.Y = data.Y -1
        else:
            data.err = data.err - 2*data.Y -1
            data.Y = data.Y +1
    
    data.err = data.R**2 - data.X**2 - data.Y **2
    # big 'case' switch
    if data.octant == 0:
        # octant 1: always y+, sometimes x-
        #~ err_par = data.R**2 - data.X**2 - (data.Y+1)**2
        err_par = data.err - 2*data.Y - 1
        err_dia = err_par + 2*data.X - 1
        
        step_Y(data,False)
        if abs(err_dia) < abs(err_par):
            step_X(data, True)
            
    elif data.octant == 1:
        # octant 2: always x-, sometimes y+
        #~ err_par = data.R ** 2 - (data.X-1) ** 2 - data.Y**2
        err_par = data.err + 2*data.X - 1
        err_dia = err_par - 2*data.Y - 1

        step_X(data, True)
        if abs(err_dia) < abs(err_par):
            step_Y(data, False)
            
    elif data.octant == 2:
        # octant 3: always x-, sometimes y-
        #~ err_par = data.R ** 2 - (data.X-1) ** 2 - data.Y**2
        err_par = data.err +2*data.X -1
        err_dia = err_par + 2*data.Y - 1

        step_X(data, True)
        if abs(err_dia) < abs(err_par):
            step_Y(data, True)
            
    elif data.octant == 3:
        # octant 4: always y-, sometimes x-
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y - 1)**2
        err_par = data.err + 2*data.Y -1
        err_dia = err_par + 2*data.X - 1

        step_Y(data,True)
        if abs(err_dia) < abs(err_par):
            step_X(data, True)
            
    elif data.octant == 4:
        # octant 5: always y-, sometimes x+
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y - 1)**2
        err_par = data.err + 2*data.Y -1
        err_dia = err_par - 2*data.X - 1

        step_Y(data,True)
        if abs(err_dia) < abs(err_par):
            step_X(data, False)
            
    elif data.octant == 5:
        # octant 6: always x+, sometimes y-
        #~ err_par = data.R ** 2 - (data.X+1) ** 2 - data.Y **2
        err_par = data.err - 2*data.X -1
        err_dia = err_par + 2*data.Y - 1

        step_X(data,False)
        if abs(err_dia) < abs(err_par):
            step_Y(data, True)
            
    elif data.octant == 6:
        # octant 7: always x+, sometimes y+
        #~ err_par = data.R ** 2 - (data.X+1) ** 2 - data.Y**2
        err_par = data.err - 2*data.X -1
        err_dia = err_par - 2*data.Y - 1

        step_X(data,False)
        if abs(err_dia) < abs(err_par):
            step_Y(data, False)
            
    elif data.octant == 7:
        # octant 8: always y+, sometimes x+
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y + 1)**2
        err_par = data.err - 2*data.Y -1 
        err_dia = err_par - 2*data.X - 1

        step_Y(data,False)
        if abs(err_dia) < abs(err_par):
            step_X(data, False)
            
    elif data.octant == 8:
        # octant -8: always y-, sometimes x-
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y -1)**2
        err_par = data.err + 2*data.Y -1
        err_dia = err_par + 2*data.X - 1
        
        step_Y(data, True)
        if abs(err_dia) < abs(err_par):
            step_X(data, True)
            
    elif data.octant == 9:
        # octant -7: always x-, sometimes y-
        #~ err_par = data.R ** 2 - (data.X-1) ** 2 - data.Y**2
        err_par = data.err + 2*data.X -1
        err_dia = err_par + 2*data.Y - 1
        
        step_X(data, True)
        if abs(err_dia) < abs(err_par):
            step_Y(data, True)
            
    elif data.octant == 10:
        # octant -6: always x-, sometimes y+
        #~ err_par = data.R ** 2 - (data.X-1) ** 2 - data.Y**2
        err_par = data.err + 2*data.X -1
        err_dia = err_par - 2*data.Y - 1
        
        step_X(data, True)
        if abs(err_dia) < abs(err_par):
            step_Y(data, False)
            
    elif data.octant == 11:
        # octant -5: always y+, sometimes x-
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y + 1)**2
        err_par = data.err -2*data.Y -1
        err_dia = err_par + 2*data.X - 1
        
        step_Y(data, False)
        if abs(err_dia) < abs(err_par):
            step_X(data, True)
            
    elif data.octant == 12:
        # octant -4: always y+, sometimes x+
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y + 1)**2
        err_par = data.err - 2*data.Y -1
        err_dia = err_par - 2*data.X - 1
        
        step_Y(data, False)
        if abs(err_dia) < abs(err_par):
            step_X(data, False)
            
    elif data.octant == 13:
        # octant -3: always x+, sometimes y+
        #~ err_par = data.R ** 2 - (data.X+1) ** 2 - data.Y **2
        err_par = data.err -2*data.X -1
        err_dia = err_par - 2*data.Y - 1
        
        step_X(data,False)
        if abs(err_dia) < abs(err_par):
            step_Y(data, False)
            
    elif data.octant == 14:
        # octant -2: always x+, sometimes y-
        #~ err_par = data.R ** 2 - (data.X+1) ** 2 - data.Y**2
        err_par = data.err -2*data.X -1
        err_dia = err_par + 2*data.Y - 1
        
        step_X(data,False)
        if abs(err_dia) < abs(err_par):
            step_Y(data, True)
            
    elif data.octant == 15:
        # octant -1: always y-, sometimes x+
        #~ err_par = data.R ** 2 - data.X ** 2 - (data.Y - 1)**2
        err_par = data.err +2*data.Y -1
        err_dia = err_par - 2*data.X - 1
        
        step_Y(data, True)
        if abs(err_dia) < abs(err_par):
            step_X(data, False)
            
    else:
        raise ValueError("undefined octant %d!" %octant)
    
    data.num_steps -= 1
    
    print data
    
    if data.endX or data.endY:
        if (data.endX == data.X) and (data.endY == data.Y):
            return False
    return data.num_steps != 0
    
X=[100]
Y=[0]
#~ d = Data(X=71,Y=70, R=100, endX=0, endY=100, num_steps=141*3,ccw=False)
d = Data(X=100,Y=0, R=100, endX=100, endY=0,ccw=True)
#~ import pdb; pdb.set_trace()
while kernel(d):
    X.append(d.X)
    Y.append(d.Y)

d = Data(X=100,Y=0, R=100, endX=100, endY=0,ccw=False)
while kernel(d):
    X.append(d.X)
    Y.append(d.Y)


import pyqtgraph
from pyqtgraph import plot

w = pyqtgraph.plot(X,Y, symbol='o')
pyqtgraph.QtGui.QApplication.exec_()

    