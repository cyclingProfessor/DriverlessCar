from CompressUtil import *
import sys

fname = sys.argv[1]
outfile = fname + '.u'

fp = open(fname, 'rb')
magic = fp.read(2)
if magic != b'P6':
    print('oops:' + str(magic))
    sys.exit(1)
width = str(ord(fp.read(1))).encode('utf-8')
height = str(ord(fp.read(1))).encode('utf-8')
fp.read(27)
code = fp.read()
fp.close()
data = decompress(code)
out = open(outfile, 'wb+')
out.write(b'P6\n')
out.write(width + b' ' + height + b'\n255\n')
out.write(data)
