import sys
import time
import serial

p = serial.Serial(port='/dev/ttyUSB0',baudrate=57600, xonxoff=True, timeout=0.1)

p.write('\n');p.flush()

t=0.5

for l in sys.stdin:
    print l,
    p.write(l);p.write('\n');p.flush()
    r=p.read(1000)
    while '\n' not in r:
       r = r+p.read(1000)
    if 'OK' in r:
        _, q = r.rsplit(' ',1)
        if int(q) > 10 and t < 5:
            t += (int(q)-10)*0.1
        else:
            t =max(0.3, t-int(q)*0.1)
    print r.strip(),t,
    time.sleep(t)
