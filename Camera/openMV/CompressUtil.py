# Untitled - By: dave - Wed Jul 10 2019
from math import floor, ceil
import gc

def compress(data) -> bytes:
    # numWords = 0
    # sizeWords = 0
    keys = {i.to_bytes(1, 'big'): i for i in range(256)}
    n_keys = 256
    compressed = []
    start = 0
    n_data = len(data)+1
    bits = 8
    while True:
        for i in range(1, n_data-start):
            w = data[start:start+i]
            if w not in keys:
                keys[w] = n_keys
                # numWords += 1
                # sizeWords += len(w)
                # # If n_keys is a power of two then increase output length
                next = keys[w[:-1]]
                bin_next = bin(next)[2:]
                bin_next = ('0' * (bits - len(bin_next))) + bin_next
                compressed.append(bin_next)
                #print ('code ' + hex(next)[2:] + ' (' + str(bits) + ')')
                #print(bin_next)
                if ((n_keys & (n_keys - 1)) == 0):
                    bits += 1
                start += i-1
                n_keys += 1
                break
        else:
            next = keys[w]
            bin_next = bin(next)[2:]
            bin_next = ('0' * (bits - len(bin_next))) + bin_next
            compressed.append(bin_next)
            #print ('code ' + hex(next)[2:] + ' (' + str(bits) + ')')
            #print(bin_next)
            break
    # Now we need to add zeroes to the end of this string so that the last byte begins with the last code bits
    bits = ''.join(compressed)
    if len(bits) % 8 != 0:
        bits += ('0' * (8 - (len(bits) % 8)))
    #print(bits)
    retval = bytearray()
    for index in range(ceil(len(bits) / 8)):
        next_byte = int(bits[8 * index:8 * index + 8], 2)
        if next_byte in (ord('{'), ord('}'), ord('|')): # actually 123, 124, 125
            retval.append(ord('|')) # The Escape character
            next_byte -= 10
        retval.append(next_byte)
    # print("Num Keys: "  + str(numWords) + ", and average size: " + str(sizeWords / numWords))
    return retval

class DataStore:
    def __init__(self, coded):
        assert(len(coded) != 0)
        self.data = coded # If we are in incremental mode then coded will be a 2 byte array
        self.__byte_offset = 0
        self.__bit_offset = 0
        self.__empty = False
    def setStoreToByte(self, ch):
        # Here the idea is that we have just one new byte each time.  So even though we were empty
        # we have now got more data.
        # Because of data transparency it is a problem keeping the bit buffer the calling function must
        # process the escape '|' character, or make sure that the final character in the nextBytes array is never escape.
        assert(ch != '|' and self.__byte_offset != 0 and self.__byte_offset <= 2)
        self.data[0] = self.data[1]
        self.data[1] = ch
        self.__byte_offset -= 1
        self.__Empty = False

    def getFirst(self):
        retval = self.__processEscape()
        self.__set_current()
        return retval

    def getNext(self, key_size):
        # set current
        assert(key_size >= 8 and key_size < 16)
        # We need key_size bits (at most 10) so collect from at most two bytes
        # First part
        first_bits = 8 - self.__bit_offset
        remaining_bits = key_size - first_bits
        mask = (1 << first_bits) - 1
        retval = (self.__current & mask) << remaining_bits

        # Now fill in remaining bits - have to notice escape character.
        self.__set_current()
        if remaining_bits >= 8:
            remaining_bits -= 8
            retval |= self.__current << remaining_bits 
            self.__set_current()

        retval |= self.__current >> (8 - remaining_bits)
        self.__bit_offset = remaining_bits
        if (self.__bit_offset == 8): # we have read the whole current byte
            self.__bit_offset = 0
            self.__set_current()
        return retval

    def __set_current(self):
        if self.__empty:
            return
        self.__byte_offset += 1
        self.__current = self.__processEscape()
        self.__empty = (len(self.data) <= 1 + self.__byte_offset)

    def __processEscape(self):
        retval = self.data[self.__byte_offset]
        if retval == ord('|'):
            self.__byte_offset += 1
            retval = self.data[self.__byte_offset] + 10
        return retval

    def isEmpty(self):
        return self.__empty

class Saver:
    def __init__(self, buffer):
        self.data = buffer
        self.outCount = 0

class ImageSaver(Saver):
    def __init__(self, img):
        super().__init__(img)
        self.__buffer = [0,0,0]
        self.__colorCount = 0
    def add(self, extra):
        for index in range(len(extra)):
            self.__buffer[self.__colorCount] = extra[index]
            self.__colorCount += 1
            if self.__colorCount == 3:
                # TO_DO
                self.data[self.outCount] = self.__buffer.copy() # Do I need to copy Here?  I hope not!
                self.outCount += 1
                self.__colorCount = 0
        
class DataSaver(Saver):
    def __init__(self, data):
        super().__init__(data)
    def add(self, extra):
        self.data[self.outCount: self.outCount + len(extra)] = extra
        self.outCount += len(extra)

class Coder:
    def __init__(self, table):
        self.__keys = table
#        print (len(table))
        self.__n_keys = 256
        self.__keySize = 8
        # self.keys = [] # array indexed by code: 2byte code, 1 byte letter (stored as bytes())
    def add(self, prev, letter):
        # prev is the bytes representation of an element of the keys array - we now find it.
        # Find first character as Code (code is an integer)
        # while next letter L in prev, set Code to the entry for 'Code:L'
        code = prev[0]
#        print('Adding:<' + str(prev) + ':' + str(letter) + '>')
        for extra in prev[1:]:
            lookup = (256 * code + extra).to_bytes(3, 'big')
            code = self.__keys.index(lookup) + 256
        iletter = letter[0]
        num = 256 * code + iletter
        val = num.to_bytes(3, 'big')
        self.__keys[self.__n_keys - 256] = val
#        print('Added ' + str(self.__keys[self.__n_keys - 256]) + ' at index ' + str(self.__n_keys))
        self.__n_keys += 1
    def decode(self, code):
#        print('Decoding' + str(code))
        if code < 256:
            return code.to_bytes(1, 'big')
        retval = bytearray()
        while code >= 256:
            val = self.__keys[code - 256]
            if val == None:
                raise KeyError()
            retval = bytearray(val[2:]) + retval 
            code = int.from_bytes(val[0:2],'big')
        retval = bytearray([code]) + retval
        return retval
        #return self.decode(int.from_bytes(val[0:2],'big')) + val[2:]
    def getAndUpdateKeySize(self):
        if self.__n_keys == 1 << self.__keySize :
            self.__keySize += 1
        return self.__keySize
 
# Now uses adaptive length.
def uncompress(data, saver):
    # First saving - do not keep the dictionary for codes up to 255.
    ds = DataStore(data)
    keyCodes = Coder([None] * len(data))
    code = ds.getFirst()
    previous = keyCodes.decode(code)
    saver.add(previous)
    while not ds.isEmpty():
        code = ds.getNext(keyCodes.getAndUpdateKeySize())
        try:
            current = keyCodes.decode(code)
        except KeyError:
            current = previous + previous[:1]
        saver.add(current)
        keyCodes.add(previous, current[:1])
        previous = current

class Uncompressor:
    def __init__(self, svr, coder, dataStore):
        self.saver = svr
        self.keyCodes = coder
        self.previous = None
        self.ds = dataStore

    def uncompressNext(self, ch):
        assert(ch != '|') # A data store cannot end with an escape character.
        self.ds.addToStore(ch)
        self.uncompressData()

    def uncompressFirst(self):
        code = ds.getFirst()
        self.previous = code
        self.saver.add(self.previous)
    def __uncompressBytes(self):
        while not self.ds.isEmpty():
            code = self.ds.getNext(self.keyCodes.getAndUpdateKeySize())
            try:
                current = self.keyCodes.decode(code)
            except KeyError:
                current = self.previous + self.previous[:1]
            self.saver.add(current)
            self.keyCodes.add(self.previous, current[:1])
            self.previous = current

# Now uses adaptive length.
def decompress(data, uncompressed):
    uncompress(data, DataSaver(uncompressed))
    # uc = Uncompressor(DataSaver(uncompressed), Coder([None] * len(data)))
    # if data[0] == ord('|'):
    #     data[1] += 10
    #     data = data[1:]
    # uc.uncompressFirst(data[:1])
    # uc.uncompressBytes(DataStore(data[1:]))

# Now uses adaptive length.
def decompressImage(data, image):
    uncompress(data, ImageSaver(image))
    # uc = Uncompressor(ImageSaver(image), Coder([None] * len(data)))
    # if data[0] == ord('|'):
    #     data[1] += 10
    #     data = data[1:]
    # uc.uncompressFirst(data[:1])
    # uc.uncompressBytes(DataStore(data))

def decompressImageStart(image, compressTable):
    return Uncompressor(ImageSaver(image), Coder(compressTable))

# Deals with escapes outside
def decompressImageProcess(uc, recvd, first=True):
    assert(recvd != '|')
    if first:
        uc.uncompressFirst(recvd)
        uc.setDataStore(DataStore(), None)
    else:
        uc.uncompressNext(recvd)
