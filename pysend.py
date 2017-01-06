#!/bin/env python

import sys
import time
import serial

class Sender(object):
    def __init__(self, port, baudrate, xonxoff, timeout):
        self.port = serial.Serial(port=port, baudrate=baudrate, xonxoff=xonxoff, timeout=timeout)
        self.queuefree = 0
        self.checkqueue()

    def checkqueue(self):
        self.port.write('\n')
        self.port.flush()
        self.checkreply()

    def checkreply(self, line=''):
        # XXX: use regexes here !
        r = ''
        while '\n' not in r:
           r += self.port.read(1000)
        r = r.strip()
        if '\n' in r:
            # extract relevant (last) response
            _, r = r.rsplit('\n', 1)
        if 'OK' in r:
            txt, queuefree = r.split(' ')
            try:
                self.queuefree = int(queuefree)
            except Exception:
                pass
        if 'resend!' in r:
            if line:
                self.sendline(line)

    def sendline(self, line):
        while self.queuefree <= 1: # safety: leave 1 open
            time.sleep(1)
            self.checkqueue()
        self.port.write(line.strip())
        # calculate checksum
        chk=0
        for c in line.strip():
            chk = chk ^ ord(c)
        # send checksum
        self.port.write('*%d' % chk)
        self.port.write('\n')
        self.port.flush()
        self.checkreply(line.strip())

    def sendfile(self, file):
        # file object, not filename!
        lines = file.readlines()
        for idx,l in enumerate(lines):
            print "\rline %d of %d" %(idx, len(lines)),
            sys.stdout.flush()
            self.sendline(l)
        print "100% done"

    def sendnamedfile(self, filename):
        with open(filename, 'r') as f:
            self.sendfile(f)

s = Sender(port='/dev/ttyUSB0', baudrate=57600, xonxoff=True, timeout=0.02)

# XXX: use argparse (or equivalent) here
if len(sys.argv) > 1:
    s.sendnamedfile(sys.argv[1])
else:
    s.sendfile(sys.stdin)
