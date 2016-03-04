#!/bin/env python

import sys
from PIL import Image
import base64


MINWHITESKIP = 16
MINLEVEL = 33

class Render(object):
    #defaults
    BPP = 8
    PixelSize = 0.2
    bidir = True
    image = None
    speed_mm_per_s = 20

    @property
    def MINLEVEL(self):
        return max(1,MINLEVEL >> (8-self.BPP))

    def loadImage(self, filename):
        im = Image.open(filename)
        # convert to gray levels
        self.image = im.convert('L')
        bb = self.image.getbbox()
        _left,_top,_right,_bottom = bb
        self.height = _bottom-_top
        self.width = _right-_left

    def encodePixelData(self, pixels, reverse = False):
        """encodes the given pixels (range 0..255) into BPP bits wide bitstrings, joins them and encodes as base64
        if reverse is true, invert order of pixels beforehand"""
        if reverse:
            data = reversed(pixels)
        else:
            data = pixels

        res = []
        lastbits = []
        for p in data:
            # append self.BPP bits from p to lastbits
            for b in range(self.BPP-1,-1,-1):
                lastbits.append(1 if p & (1<<b) else 0)
            if len(lastbits) >=8:
                # move a byte into res
                n = 0
                for b in range(8-1,-1,-1):
                    n += (1<<b) * lastbits.pop(0)
                res.append(chr(n))
        # fill up remaining bits with 0, move the last byte to res
        if lastbits:
            while len(lastbits) < 8:
                lastbits.append(0)
            # move a byte into res
            n = 0
            for b in range(8-1,-1,-1):
                n += (1<<b) * lastbits.pop(0)
            res.append(chr(n))
        bytestring = b''.join(res)
        # call base64.encodestring on byte strings of size <=48, collect in a list, return list
        res = []
        while len(bytestring) > 0:
            res.append("$%s" % base64.encodestring(bytestring[:48])[:-1]) # remove trailing \n as well
            bytestring = bytestring[48:]
        return res



    def renderlines(self):
        """returns a list with height lists.
        sublists contain (startoffset, numberofpixels,[pixeldata]) tuples """
        res = []
        bb = self.image.getbbox()
        _left,_top,_right,_bottom = bb
        for y in range(_bottom-1, _top-1,-1):
            # first round: extract all pixels into list
            pixelline = []
            for x in range(_left, _right):
                # convert white(255) to no laser(0) and black(0) to full laser(255)
                # also kill LSB's outside self.BPP
                pixelline.append( (255 - self.image.getpixel((x,y))) >> (8-self.BPP) )

            # second round: erode lines from left and right, removing white pixels
            startx = 0
            endx = self.width - 1
            while pixelline[startx] < self.MINLEVEL:
                startx += 1
                if startx > endx:
                    break
            while pixelline[endx] < self.MINLEVEL:
                endx -= 1
                if startx > endx:
                    break
            # line may be fully empty
            if startx > endx:
                res.append([]) # no pixeldata
                continue

            # third round: check inner areas
            # find the leftmost run of at leastMINWHITESKIP white pixels, extend as much as possible
            i = startx
            ll = []
            while True:
                # find first whitespace
                while (i < endx) and pixelline[i] >= self.MINLEVEL:
                    i += 1
                # find end of run
                j = i + 1
                while (j < endx) and pixelline[j] < self.MINLEVEL:
                    j += 1
                # check length
                if j-i > MINWHITESKIP:
                    ll.append((startx, pixelline[startx:i]))
                    startx = j
                    i = j
                if j < endx:
                    i += 1
                    continue
                break
            ll.append((startx, pixelline[startx:endx+1]))
            res.append(ll)
        return res


    def render(self):
        """renders a previously loaded image and returns the string which should be send to the cutter"""
        # header
        res = [ ";rendered with Renderpicture.py by EndlessFork",
                "; picture is %dx%d pixels" % (self.width, self.height),
                "; rastersize is %f x %f mm" % (self.width*self.PixelSize, self.height*self.PixelSize),
                "; resolution is %f mm" % self.PixelSize,
                "; using %d BitsPerPixel" % self.BPP,
                "; bidir rendering is %s" % ("ON" if self.bidir else "OFF"),
                "",
                "G28;home",
                "G21;mm",
                "F%d" % int(self.speed_mm_per_s * 60),
                "G90;absolute units",
                "M649 S100 C%d L%.3f P5 B6;B6=PulseRaster B5=CWRaster, L=pulse length" % (self.BPP, 900*self.PixelSize/self.speed_mm_per_s),
                ""
              ]
        res.append("G0 X0 Y0 ; Adjust lower left corner here")
        res.append("G91")

        data = self.renderlines()
        lasty = 0
        lastx = 0
        y = 0
        dirflag = +1
        for line in data: # line by line
            res.append("")
            res.append(";line %d" % (y+1))
            if line:
                if dirflag == +1:
                    for piece in line:
                        startx, pixels = piece
                        length = len(pixels)
                        # generate G0-moveToStart
                        res.append("G0 X%.3f Y%.3f" % ((startx-lastx)*self.PixelSize, (y-lasty)*self.PixelSize))
                        # generate G1-rasterToEndOfPiece
                        res.append("G1 X%.3f Q%d" % (length*self.PixelSize, length))
                        # encode Pixeldata and append
                        res.extend(self.encodePixelData(pixels))
                        res.append("G4")
                        # bookkeeping
                        lastx = startx+length
                        lasty = y
                else:
                    # XXX: encode backwards!
                    for piece in reversed(line):
                        startx, pixels = piece
                        length = len(pixels)
                        startx += length
                        # generate G0-moveToStart
                        res.append("G0 X%.3f Y%.3f" % ((startx-lastx)*self.PixelSize, (y-lasty)*self.PixelSize))
                        # generate G1-rasterToEndOfPiece
                        res.append("G1 X%.3f Q%d" % (-length*self.PixelSize, length))
                        # encode Pixeldata and append
                        res.extend(self.encodePixelData(pixels, True))
                        res.append("G4")
                        # bookkeeping
                        lastx = startx-length
                        lasty = y
                if self.bidir:
                    dirflag = -dirflag
            y += 1

        # append footer
        res.append("")
        res.append(";footer")
        res.append("G90")
        res.append("G0 X0 Y230")
        res.append("G28")
        res.append("M2")

        return '\n'.join(res)

# XXX: use argparse or similiar!
if len(sys.argv) > 1:
	r = Render()
	r.loadImage(sys.argv[1])
	print r.render()
