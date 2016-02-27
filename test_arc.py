
from math import sqrt
import pyqtgraph
from pyqtgraph import plot

X=[]
Y=[]


def move(x, y):
    if X:
        raise ProgrammingError('in this test, move can only be called first')
    X.append(x)
    Y.append(y)

def stroke(x, y, **kwds):
    if not X:
        raise ProgrammingError('in this test, stroke can not be called first')
    X.append(x)
    Y.append(y)


def arc_nonrec(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a konstant distance from (xm,ym)
    
    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    #~ print "arc(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f)" % (x0,y0,x1,y1,xm,ym)
    # maximum allowed deviation from ideal circle
    max_dev = 0.01 # absolute
    max_dev_rel = 1 # in percent of radius

    r0 = sqrt((xm-x0)**2+(ym-y0)**2)
    r1 = sqrt((xm-x1)**2+(ym-y1)**2)

    # use the stronger criterion (absolute vs. relative
    max_dev = min(max(r0,r1)*max_dev_rel*0.01, max_dev)

    # get max allowed distance for allowed deviation
    perp_max_length = 2 * sqrt(r0**2-(r0-max_dev)**2)

    # get distance
    dist = sqrt((x1-x0)**2+(y1-y0)**2)

    # get sense of rotation of Pm, P0, P1
    sense = ((x0-xm)*(y1-ym) - (x1-xm)*(y0-ym)) > 0

    if sense != ccw:
        pass
        # zwischenpunkt ausrechnen und kreisbogen halbieren.

    while dist > perp_max_length:
        # get next point at perp_max_length distance and stroke and re-calculate dist
        
        # XXXX! !!!!!!!!!!!!!
        x0=(x0+x1)/2
        y0=(y1+y0)/2
        
        #stroke to next point
        stroke(x0,y0)
        
        # get distance
        dist = sqrt((x1-x0)**2+(y1-y0)**2)
        
    # final stroke (if needed)
    if (x1!=x0) or (y1!=y0):
        stroke(x1,y1)




def arc_recursiv(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a constant distance from (xm,ym)

    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    #~ print "arc(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f)" % (x0,y0,x1,y1,xm,ym)
    # maximum allowed deviation from ideal circle
    max_dev = 0.01 # absolute
    max_dev_rel = 1 # in percent of radius

    r0 = sqrt((xm-x0)**2+(ym-y0)**2)
    r1 = sqrt((xm-x1)**2+(ym-y1)**2)

    # use the stronger criterion (absolute vs. relative
    max_dev = min(max(r0,r1)*max_dev_rel*0.01, max_dev)

    # get max allowed distance for allowed deviation
    perp_max_length = 2 * sqrt(r0**2-(r0-max_dev)**2)

    # get distance
    dist = sqrt((x1-x0)**2+(y1-y0)**2)

    # get sense of rotation of Pm, P0, P1
    sense = ((x0-xm)*(y1-ym) - (x1-xm)*(y0-ym)) > 0

    if sense != ccw:
        pass
        # zwischenpunkt ausrechnen und kreisbogen halbieren.

    while dist > perp_max_length:
        # get next point at perp_max_length distance and stroke and re-calculate dist
        
        # XXXX! !!!!!!!!!!!!!
        x0=(x0+x1)/2
        y0=(y1+y0)/2
        
        #stroke to next point
        stroke(x0,y0)
        
        # get distance
        dist = sqrt((x1-x0)**2+(y1-y0)**2)
        
    # final stroke (if needed)
    if (x1!=x0) or (y1!=y0):
        stroke(x1,y1)



def arc_recursiv(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a constant distance from (xm,ym)

    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    #~ print "arc(%.3f, %.3f, %.3f, %.3f, %.3f, %.3f)" % (x0,y0,x1,y1,xm,ym)
    # maximum allowed deviation from ideal circle
    max_dev = 0.01 # absolute
    max_dev_rel = 1 # in percent of radius
    
    r0 = sqrt((xm-x0)**2+(ym-y0)**2)
    r1 = sqrt((xm-x1)**2+(ym-y1)**2)
    
    #~ if abs(r0-r1) > 1e-6:
        #~ print "radi differ ! r0=%f, r1=%f" %(r0,r1)
        #~ return
    
    # use the stronger criterion (absolute vs. relative
    max_dev = min(max(r0,r1)*max_dev_rel*0.01, max_dev)
    
    # get max allowed distance for allowed deviation
    perp_max_length = 2 * sqrt(r0**2-(r0-max_dev)**2)
    
    # get distance
    dist = sqrt((x1-x0)**2+(y1-y0)**2)

    # get sense of rotation of Pm, P0, P1
    sense = ((x0-xm)*(y1-ym) - (x1-xm)*(y0-ym)) > 0

    if sense != ccw:
        pass
        # zwischenpunkt ausrechnen und kreisbogen halbieren.

    if dist > perp_max_length:
        # calculate midpoint
        # x2 = xm + (90 deg rotation_of)(x1-x0)/dist*(r0+r1)/2
        # get unit vector:
        xu = (x1-x0)/dist
        yu = (y1-y0)/dist
        # rotate by 90 deg
        if ccw:
            xu, yu = yu, -xu
        else:
            xu, yu = -yu, xu
        
        # get average radius
        rn = (r0+r1)/2
        #~ rn = r0
        
        # get new point
        x2 = xm + xu*rn
        y2 = ym + yu*rn
        
        # recurse
        arc_recursiv(x0, y0, x2, y2, xm, ym, ccw)
        arc_recursiv(x2, y2, x1, y1, xm, ym, ccw)
    else:
        stroke(x1,y1)

move(2, 0)
arc_nonrec(X[0], Y[0],  0,0,  1,1, True)

print X
import pyqtgraph
w = pyqtgraph.plot(X,Y, symbol='o')
pyqtgraph.QtGui.QApplication.exec_()
