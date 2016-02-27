
from math import sqrt
import pyqtgraph
from pyqtgraph import plot


X=[]
Y=[]



# NEG_X = 1 -> X=X_center - X_tracer, else X=X_center + X_tracer
NEG_X = 1
# NEG_Y = 1 -> Y=Y_center - Y_tracer, else Y=Y_center + Y_tracer
NEG_Y = 2
# if FLIP_XY: X_Tracer, Y_Tracer = Y_Tracer, X_Tracer
FLIP_XY = 4


# tracer traces Q1 in CCW direction
Q1_CW = 0
Q2_CW = FLIP_XY + NEG_X
Q3_CW = NEG_X + NEG_Y
Q4_CW = FLIP_XY + NEG_Y
Q1_CCW = FLIP_XY
Q2_CCW = NEG_X
Q3_CCW = FLIP_XY + NEG_X + NEG_Y
Q4_CCW = NEG_Y

NEXT_Q = [6,7,4,5,1,0,3,2]
# same as next_Q = ((0x23015476 >> (old_Q*4) ) & 0x07
# same as next_Q = ((0x4c1b3e >> (old_Q*3) ) & 0x07


class ArcPiece(object):
    # tracing vars
    x = 0
    y = 0
    # error increments
    dx = 0
    dy = 0
    # initial error value
    err = 0
    # number of steps to take
    num_steps = 0
    quadrant=None
    
    def __init__(self, **kwds):
        self.__dict__.update(kwds)
    def __repr__(self):
        return 'ArcPiece('+', '.join('%s=%s' % (k,v) for k,v in sorted(self.__dict__.items()))+')'
            

class ArcTracer(object):
    def __init__(self):
        self.segments = []
        self.X=0
        self.Y=0

    def emit(self, x, y, *args):
        X.append(x)
        Y.append(y)

    def quadrant(self, x, y, ccw=False):
        # returns the quadrant a point is in
        r = (NEG_X if x < 0 else 0) + (NEG_Y if y < 0 else 0)
        if ccw and r in [0, 3]:
            return FLIP_XY + r
        elif not(ccw) and r in [1, 2]:
            return FLIP_XY + r
        return r

    def trace_step(self, ap):
        # ap must be an arc_piece
        # we return True for another step, False if arc is finished
        e2 = ap.err
        if e2 >= -ap.x:
            if ap.quadrant & FLIP_XY:
                # select Y
                self.Y = self.Y-1 if ap.quadrant & NEG_Y else self.Y+1
            else:
                # select X
                self.X = self.X-1 if ap.quadrant & NEG_X else self.X+1
            ap.x += 1
            ap.err += ap.dx
            ap.dx -= 2
        if e2 < ap.y:
            if ap.quadrant & FLIP_XY:
                # select X
                self.X = self.X+1 if ap.quadrant & NEG_X else self.X-1
            else:
                # select Y
                self.Y = self.Y+1 if ap.quadrant & NEG_Y else self.Y-1
            ap.y -= 1
            ap.err += ap.dy
            ap.dy -= 2
        ap.num_steps -= 1
        #~ print "stepping to %d, %d, err is %d" % (self.X,self.Y,ap.err)
        self.emit(self.X,self.Y)
        return ap.num_steps > 0

    def calc_piece(self, R, startx, starty, endx, endy):
        # calculate an cw arc-piece for two points in Q1 where endpoint is clockwise of startpoint

        if startx < 0:
            raise "Starting X must be >= 0!"
        if starty < 0:
            raise "Starting Y must be >= 0!"
        if endx < 0:
            raise ValueError("Final X must be >= 0! (endx=%d)" % endx)
        if endy < 0:
            raise "Final Y must be >= 0!"
        
        #~ if endx < startx or endy > starty:
            #~ raise "final point must be clockwise of start point"
            
        # calculate elements
        X = startx
        Y = starty
        dX = -(2*X + 3)
        dY = 2*Y - 3
        err = R**2 -(X+1)**2 - (Y-1) **2

        numpoints = 0
        maxpoints = int(sqrt(2)*R)
        ap = ArcPiece(x=X,y=Y,dx=dX,dy=dY,err=err,num_steps=maxpoints,quadrant=Q1_CW)
        #~ print ap
        
        #~ print "start tracing at %d,%d" %(X,Y)
        while Y > 0:
            #~ err = R**2 -(X+1)**2 - (Y-1) **2
            e2 = err
            if e2 >= -X:
                X += 1
                err += dX
                dX -= 2
            if e2 < Y:
                Y -= 1
                err += dY
                dY -= 2
            numpoints += 1
            ap.num_steps = numpoints
            if X>Y:
                # Y decides if we are finished
                if Y == endy:
                    # check if x matches as well
                    if X == endx:
                        #~ print "perfect match"
                        return ap, True
                    else:
                        #~ print "imperfect match: X=%d, should be X=%d" %(X,endx)
                        diff = endx-X
                        return self.calc_piece(R,startx-diff, starty, endx-diff, endy)
            else:
                # X decides if we are finished
                if X == endx:
                    # check if y matches as well
                    if Y == endy:
                        #~ print "perfect match"
                        return ap, True
                    else:
                        #~ print "imperfect match: Y=%d, should be Y=%d" %(Y,endy)
                        diff = endy-Y
                        return self.calc_piece(R,startx, starty-diff, endx, endy-diff)
            #~ print "stepping to %d, %d, err is %d" % (X,Y,err)
            if numpoints > maxpoints:
                raise "did not termintate !"
        #~ print "%d of %d points traced" % (numpoints, maxpoints)
        ap.num_steps = numpoints
        return ap, False # no match....


    def tracePoint(self, x, y, ccw=False):
        """translates given point to trace koordinates (Q1 in ccw direction)"""
        q = self.quadrant(x, y, ccw)
        print "Quadrant of %d,%d,%r is %d" % (x,y,ccw,q)
        if q & NEG_X:
            x = -x
        if q & NEG_Y:
            y = -y
        return x, y, q
        if q & FLIP_XY:
            x, y = y, x

    def arc(self, R, startX, startY, endX, endY, ccw=False):
        """does a dummy trace of an arc and returns a list of arc_pieces for the tracer"""
        # XXX: checks
        
        # calculate tracepoints for start, end
        startX, startY, startQ = self.tracePoint(startX, startY, ccw)
        endX, endY, endQ = self.tracePoint(endX, endY, ccw)

        print 'start:', startX, startY, startQ
        print 'end:', endX, endY, endQ
        
        arcs = []
        while len(arcs) < 6:
            if startQ == endQ:
                ap, match = self.calc_piece(R, startX, startY, endX, endY)
                ap.quadrant = startQ
                if match:
                    arcs.append(ap)
                    return arcs
            else:
                ap, match = self.calc_piece(R, startX, startY, R, 0)
            ap.quadrant = startQ
            arcs.append(ap)
            startX = 0
            startY = R
            startQ = NEXT_Q[startQ] # advance to next quadrant
            if (startX, startY, startQ) == (endX, endY, endQ):
                    return arcs

        raise "arc does not find end point!"


AT = ArcTracer()
print AT.arc(10, 2, 10, 10, 2, False)
print
print AT.arc(10, 2, 10, 10, 2, True)
print

AT.Y=10
AT.X=0

pl =  AT.arc(10,0,10,0,10)
print "start at %d,%d" % (AT.X,AT.Y)
for piece in pl:
    while AT.trace_step(piece):
        print "stepped to %d,%d" % (AT.X,AT.Y)
    print "stepped to %d,%d" % (AT.X,AT.Y)
    print "----"

XY = zip(X,Y)
for i in range(len(XY)):
    for j in range(len(XY)):
        for ccwflag in [True, False]:
            #~ X=[]
            #~ Y=[]
            pl = AT.arc(10, XY[i][0],XY[i][1],XY[j][0],XY[j][1])
            AT.X, AT.Y = XY[i]
            for piece in pl:
                while AT.trace_step(piece):
                    pass
            print i, j, len(X)
            

w = pyqtgraph.plot(X,Y, symbol='o')


pyqtgraph.QtGui.QApplication.exec_()





import sys
sys.exit(0)



def arc(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a konstant distance from (xm,ym)
    
    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    # transform x0,x1 to zero based circle
    x0 = x0 - xm    
    y0 = y0 - ym    
    x1 = x1 - xm    
    y1 = y1 - ym    


    # calculation is always clockwise, for ccw we mirror x at xm
    if ccw:
        x0 = -x0
        x1 = -x1


    rsq = x0**2 + y0**2 # radius squared
    R = sqrt(rsq)    # radius
    D = int(2*R)    # diameter in steps
    
    # q1
    # start
    # start 'pixel'
    x = -int(D/2)
    y = 0
    i = 0
    
    err = 2-D
    
    while x<=0:
        X.append(int(-x))
        Y.append(int(y))
        e2 = err *2;
        if e2 > (2*x+1):
            x+=1
            err += (2*x+1)
        if e2 < (2*y+1):
            y+=1
            err += (2*y+1)
        i+=1
        
#~ from http://members.chello.at/~easyfilter/bresenham.html
#~ void plotEllipse(int xm, int ym, int a, int b)
#~ {
   #~ int x = -a, y = 0;           /* II. quadrant from bottom left to top right */
   #~ long e2 = (long)b*b, err = (long)x*(2*e2+x)+e2;         /* error of 1.step */
                                                          #~ 
   #~ do {                                                   
       #~ setPixel(xm-x, ym+y);                                 /*   I. Quadrant */
       #~ setPixel(xm+x, ym+y);                                 /*  II. Quadrant */
       #~ setPixel(xm+x, ym-y);                                 /* III. Quadrant */
       #~ setPixel(xm-x, ym-y);                                 /*  IV. Quadrant */
       #~ e2 = 2*err;                                        
       #~ if (e2 >= (x*2+1)*(long)b*b)                           /* e_xy+e_x > 0 */
          #~ err += (++x*2+1)*(long)b*b;                     
       #~ if (e2 <= (y*2+1)*(long)a*a)                           /* e_xy+e_y < 0 */
          #~ err += (++y*2+1)*(long)a*a;
   #~ } while (x <= 0);
#~ 
   #~ while (y++ < b) {                  /* too early stop of flat ellipses a=1, */
       #~ setPixel(xm, ym+y);                        /* -> finish tip of ellipse */
       #~ setPixel(xm, ym-y);
   #~ }
#~ }
#~ void plotOptimizedEllipse(int xm, int ym, int a, int b)
#~ {
   #~ long x = -a, y = 0;          /* II. quadrant from bottom left to top right */
   #~ long e2 = b, dx = (1+2*x)*e2*e2;                       /* error increment  */
   #~ long dy = x*x, err = dx+dy;                             /* error of 1.step */
                                                     #~ 
   #~ do {                                              
       #~ setPixel(xm-x, ym+y);                                 /*   I. Quadrant */
       #~ setPixel(xm+x, ym+y);                                 /*  II. Quadrant */
       #~ setPixel(xm+x, ym-y);                                 /* III. Quadrant */
       #~ setPixel(xm-x, ym-y);                                 /*  IV. Quadrant */
       #~ e2 = 2*err;
       #~ if (e2 >= dx) { x++; err += dx += 2*(long)b*b; }             /* x step */
       #~ if (e2 <= dy) { y++; err += dy += 2*(long)a*a; }             /* y step */
   #~ } while (x <= 0);
#~ 
   #~ while (y++ < b) {            /* too early stop for flat ellipses with a=1, */
       #~ setPixel(xm, ym+y);                        /* -> finish tip of ellipse */
       #~ setPixel(xm, ym-y);
   #~ }
#~ }


    print ('%d points generated' % i)

def arc2(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a konstant distance from (xm,ym)
    
    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    # transform x0,x1 to zero based circle
    x0 = x0 - xm    
    y0 = y0 - ym    


    rsq = x0**2 + y0**2 # radius squared
    R = sqrt(rsq)    # radius
    D = int(2*R)    # diameter in steps
    
    # q1
    # start
    # start 'pixel'
    x = 0
    y = int(D/2)
    i=0
    
    err = 2-D
    
    while y>=0:
        X.append(int(x))
        Y.append(int(y))
        e2 = err *2;
        if e2 < (2*x+1):
            x+=1
            err += (2*x+1)
        if e2 > -(2*y-1):
            y-=1
            err += -(2*y-1)
        i+=1

    print ('%d points generated' % i)

def arc2n(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a konstant distance from (xm,ym)
    
    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    """
    # transform x0,x1 to zero based circle
    x0 = x0 - xm    
    y0 = y0 - ym    


    rsq = x0**2 + y0**2 # radius squared
    R = sqrt(rsq)    # radius
    D = int(2*R)    # diameter in steps
    
    # q1
    # start
    # start 'pixel'
    x = 0
    y = int(D/2)
    i = 0
    
    dx = 2*x+1
    dy = -(2*y-1)
    
    err = 1-D
    
    while y>=0:
        X.append(int(x))
        Y.append(int(y))
        e2 = err *2;
        if e2 < dx:
            x += 1
            dx += 2
            err += dx
        if e2 > dy:
            y -= 1
            dy += 2
            err += dy
        i+=1

    print ('%d points generated' % i)

def myarc(x0,y0,x1,y1,xm,ym,ccw=False):
    """interpolates an arc from (x0,y0) to (x1,y1) while keeping a konstant distance from (xm,ym)
    
    either in clockwise(ccw=False) or counter-clockwise rotation(ccw=True)
    
    atm only ccw !!!
    """
    # transform x0,x1 to zero based circle
    x0 = x0 - xm    
    y0 = y0 - ym    
    x1 = x1 - xm    
    y1 = y1 - ym    

    rdiff = sqrt(x0**2 + y0**2) - sqrt(x1**2 + y1**2) # radius difference
    if abs(rdiff) > 1:
        print "Radius difference too big! %.3f -> %.3f" %(sqrt(x0**2 + y0**2),sqrt(x1**2 + y1**2))
        return

    rsq = (x0**2 + y0**2 + x1**2 + y1**2) /2 # average squared radius

    # initial error, negative is inside
    err = x0**2 + y0**2 - rsq
    
    i=0
    
    def sgn(x):
        return +1 if x > 0 else (-1 if x < 0 else 0)

    def check_octant(x0,y0, x1,y1):
        # check target octant
        if sgn(x0) == sgn(x1) and sgn(y0) == sgn(y1):
            if abs(x0) > abs(y0):
                return abs(x1) > abs(y1)
            elif abs(x0) < abs(y0):
                return abs(x1) < abs(y1)
        return False
    
    def target_reached(x0,y0, x1,y1):
        if check_octant(x0,y0, x1,y1):
            if abs(x0) > abs(y0):
                return y0==y1
            else:
                return x0==x1
        return False
        
    while True:
        if target_reached(x0,y0,x1,y1):
            break
        # do a single step until completed
        X.append(x0)
        Y.append(y0)
        
        # determine step directions
        # sx depends on y0 and sy on x0
        sy = sgn(x0) if ccw else -sgn(x0)
        sx = -sgn(y0) if ccw else sgn(y0)
        
        print x0, y0, sx, sy, err
        # do ONE step, either in x, y or both
        
        
        if abs(x0) > abs(y0):
            # Y is major, always step this
            if sx*x0 > 0:
                if 2*(1+sx*2*x0)*(x0*x0+y0*y0-rsq+1+sy*2*y0) + (1+sx*2*x0) < 0:
                    x0 +=sx
                    # tick X
            elif sx*x0 < 0:
                if 2*(1+sx*2*x0)*(x0*x0+y0*y0-rsq+1+sy*2*y0) + (1+sx*2*x0) > 0:
                    x0 +=sx
                    # tick X
            y0 += sy
            # tick Y
        elif abs(x0) < abs(y0):
            # X is major, always step this
            if sy*y0 > 0:
                if 2*(1+sy*2*y0)*(x0*x0+y0*y0-rsq+1+sx*2*x0) + (1+sy*2*y0) < 0:
                    y0 += sy
                    # tick Y
            elif sy*y0 < 0:
                if 2*(1+sy*2*y0)*(x0*x0+y0*y0-rsq+1+sx*2*x0) + (1+sy*2*y0) > 0:
                    y0 += sy
                    # tick Y
            x0 += sx
            # tick X
        else:
            # on the diagonal step both
            x0 += sx
            y0 += sy
            # tick X&Y

            
        i += 1
            
        if i  > 1000:
            break

        if x0 == x1 and y0 == y1: 
            break
    
    if x0 != x1:
        x0 += sx
        # tick X as last step correction
    if y0 != y1:
        y0 +=sy
        # tick Y as last step correction

    print ('%d points generated' % i)



arc(40.0,0,  -10.4,0,  0,0, False)

w = pyqtgraph.plot(X,Y, symbol='o')
X=[];Y=[]
arc(41.0,0,  -10.4,0,  0,0, False)
w.plot(X,Y, symbol='x')
X=[];Y=[]
arc(40.5,0,  -10.4,0,  0,0, False)
w.plot(X,Y)
X=[];Y=[]
arc2(3,0,  -10.4,0,  0,0, False)
w.plot(X,Y)
X=[];Y=[]
arc2n(3,0,  -10.4,0,  0,0, False)
w.plot(X,Y)

#~ X=[];Y=[]
#~ myarc(42, 0, 30,-30,  0,0, True)
#~ w.plot(X,Y, symbol='o')

pyqtgraph.QtGui.QApplication.exec_()
