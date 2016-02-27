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

    def checkreply(self):
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

    def sendline(self, line):
        while self.queuefree <= 1: # safety: leave 1 open
            time.sleep(1)
            self.checkqueue()
        self.port.write(line.strip())
        self.port.write('\n')
        self.port.flush()
        self.checkreply()

    def sendfile(self, file):
        # file object, not filename!
        lines = file.readlines()
        for l in lines:
            print "\rline %d of %d:",l.strip(),
            self.sendline(l)

    def sendnamedfile(self, filename):
        with open(filename, 'r') as f:
            self.sendfile(f)

s = Sender(port='/dev/ttyUSB0', baudrate=57600, xonxoff=True, timeout=0.1)

# XXX: use argparse (or equivalent) here
if len(sys.argv) > 1:
    s.sendnamedfile(sys.argv[1])
else:
    s.sendfile(sys.stdin)
