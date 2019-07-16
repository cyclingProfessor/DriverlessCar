from CompressUtil import *
import sys

fname = sys.argv[1]
outfile = fname + '.Z'

fp = open(fname, 'rb')
fp.readline()
fp.readline()
sizes = fp.readline()
sz = bytes([int(s) for s in sizes.split() if s.isdigit()][0:2])
print(sz[0])
print(sz[1])
check = fp.readline().rstrip() # discard this line which should be 255\n
print(check)
if check != b'255':
    print('oops')
    sys.exit(1)
data = fp.read()
fp.close()
code = compress(data)
print (len(code))
out = open(outfile, 'wb+')
out.write(b'P6' + sz + bytes.fromhex(fname[0:14]) + (b' ' * 20))
out.write(code)
