# Untitled - By: dave - Wed Jul 10 2019
from math import floor, ceil
from pyb import millis
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
#        print('padding')
        bits += ('0' * (8 - (len(bits) % 8)))
#    print(len(bits), ' ', range(ceil(len(bits) / 8)))
    retval = bytearray()
    for index in range(ceil(len(bits) / 8)):
        next_byte = int(bits[8 * index:8 * index + 8], 2)
        if next_byte in (ord('{'), ord('}'), ord('|')): # actually 123, 124, 125
            retval.append(ord('|')) # The Escape character
            next_byte -= 10
#            print('ESCAPE', index)
        retval.append(next_byte)
    # print("Num Keys: "  + str(numWords) + ", and average size: " + str(sizeWords / numWords))
    return retval

class DataStore:
    def __init__(self, coded, streamed = False):
        assert(len(coded) != 0)
        self.data = coded # If we are in incremental mode then coded will be a 3 byte array, and will not be escaped
        self.__byte_offset = 0
        self.__bit_offset = 0
        self.__streamed = streamed
	# For profiling
        # self.getNextTime = 0
        # self.storeByteTime = 0

#    @micropython.native
    def setStoreToByte(self, ch):
        # Add next character to the end of the array
        # start = millis()
        for index in range(2):
            self.data[0 + index] = self.data[1 + index]
        self.data[2] = ch
        self.__byte_offset -= 1
        # self.storeByteTime += millis() - start

    def getFirst(self):
        # print('Initial data bytes', self.data[0], self.data[1])
        retval = self.__processEscape() # The first byte
        # print(retval)
        self.__set_current() # increments __byte_offset and Sets __current to the (second) data byte
#        print(self.__byte_offset)
        return retval

#    @micropython.native
    def hasNextCode(self, key_size):
        # Here we count bytes that are beyond the .__byteCount index in .data to make sure that
        if self.__streamed:
            return 8 * (len(self.data) - self.__byte_offset) >= key_size + self.__bit_offset
        remaining_bits = key_size + self.__bit_offset - 8
        index = self.__byte_offset
#        print('Checking whether we have more codes in the data buffer. Remaining Bits: ', remaining_bits, 'Current part byte at offset', self.__byte_offset, ' Data Length:', len(self.data))
        while remaining_bits > 0 and index < len(self.data) - 1:
            remaining_bits -= 0 if self.data[index] == ord('|') else 8
#        print('    Answer:', remaining_bits <= 0)
        return remaining_bits <= 0

#    @micropython.native
    def getNext(self, key_size):
        # set current
        # start = millis()
        assert(key_size >= 8 and key_size < 16)
        # We need key_size bits (at most 10) so collect from at most two bytes
        # First part
        first_bits = 8 - self.__bit_offset
        remaining_bits = key_size - first_bits
        mask = (1 << first_bits) - 1
        retval = (self.__current & mask) << remaining_bits

        # Now fill in remaining bits - have to notice escape character.
        self.__set_current()
        if remaining_bits > 8:
            remaining_bits -= 8
            retval |= self.__current << remaining_bits 
            self.__set_current()

        retval |= self.__current >> (8 - remaining_bits)
        self.__bit_offset = remaining_bits
        # self.getNextTime += millis() - start
        return retval

    def __set_current(self):
#        print('Advancing')
        self.__byte_offset += 1
        self.__current = self.__processEscape()

    def __processEscape(self):
        retval = self.data[self.__byte_offset]
        if self.__streamed or retval != ord('|'):
            return retval
        self.__byte_offset += 1
        return self.data[self.__byte_offset] + 10

class Saver:
    def __init__(self, buffer):
        self.data = buffer
        self.outCount = 0
        # self.addTime = 0

class ImageSaver(Saver):
    def __init__(self, img):
        super().__init__(img)
        self.__colorBuffer = [0,0,0]
        self.__colorCount = 0
#    @micropython.native
    def add(self, colors, length):
        # start = millis()
        cptr = self.__colorBuffer
        dt = self.data
        oc = self.outCount
        cc = self.__colorCount
        for index in range(length):
            cptr[cc] = colors[-length + index]
            cc += 1
            if cc == 3:
                dt[oc] = cptr.copy()
                oc += 1
                cc = 0
        self.__colorCount = cc
        self.outCount = oc
        # self.addTime += millis() - start
        
class DataSaver(Saver):
    def __init__(self, data):
        super().__init__(data)
    def add(self, buffer, length):
        # offset is negative so actually gives the number of bytes to store
        self.data[self.outCount: self.outCount + length] = buffer[-length:]
        self.outCount += length

class Coder:
    def __init__(self, table):
        self.__keys = table
        self.__n_keys = 256
        self.__keySize = 8
        self.__decodeBuffer = bytearray(100)
	# For profiling
        # self.decodingTime = 0
        # self.addTime = 0
    def add(self, code, iletter):
        # start = millis()
        offset = 3 * (self.__n_keys - 256)
        kys = self.__keys
        kys[offset] = code >> 8
        kys[offset + 1] = (code & 255)
        kys[offset + 2] = iletter
        self.__n_keys += 1
        # self.addTime += millis() - start
#    @micropython.native
    def decode(self, code, buffer):
        # print('Decoding' + hex(code)[2:])
        # start = millis()
        kys = self.__keys
        retval = -1
        while code >= 256:
            c_code = 3 * (code - 256)
            if code >= self.__n_keys:
                return 0 # indicates fail
            buffer[retval] = kys[c_code + 2]
            retval -= 1
            code = (kys[c_code] << 8) + kys[c_code + 1] 
        buffer[retval] = code
        # self.decodingTime += millis() - start
        # print(millis(), ' ', millis() - start, ' Decoding Finished' + hex(code)[2:])
        return retval
    def getNumKeys(self):
        return self.__n_keys
    def getKeySize(self):
        return self.__keySize
    def getAndUpdateKeySize(self):
        if self.__n_keys == 1 << self.__keySize :
            self.__keySize += 1
        return self.__keySize
 
class Uncompressor:
    def __init__(self, svr, coder, dataStore):
        self.saver = svr
        self.keyCodes = coder
        self.previousOffset = -1
        self.current = 0
        self.prevCode = None
        self.ds = dataStore
        self.buffer = (bytearray(100), bytearray(100))
	# For profiling
        # self.uncompressBytesTime = 0
        # self.currentIndex = 0

    def uncompressFirst(self):
        code = self.ds.getFirst()
        # print('code ' + hex(code)[2:] + ' (' + str(self.keyCodes.getKeySize()) + ')')
        self.buffer[1][-1] = code
        self.prevCode = code
        self.saver.add(self.buffer[1], 1)

    def uncompressBytes(self):
        # start = millis()
        while self.ds.hasNextCode(self.keyCodes.getAndUpdateKeySize()):
            buff = self.buffer[self.current]
            prev = self.buffer[1 - self.current]
            code = self.ds.getNext(self.keyCodes.getKeySize())
            # print('code ' + hex(code)[2:] + ' (' + str(self.keyCodes.getKeySize()) + ')')
            if code < 256:
                buff[-1] = code
                current = -1
            else:
                current = self.keyCodes.decode(code, buff)
            if current < 0:
                self.keyCodes.add(self.prevCode, buff[current])
                self.prevCode = code
#                print('Current:', current)
            else:
                # print('Decoding an unseen code:', code, ', Maximum so far is:', self.keyCodes.getNumKeys() - 1)
                # print(self.previous, self.previousOffset)
                # print('Previous Decode:', self.previous[self.previousOffset:])
                # print('Extra character:', self.previous[self.previousOffset])
                current = self.previousOffset - 1
                buff[current:-1] = prev[self.previousOffset:]
                buff[-1] = prev[self.previousOffset]
                # print('Decoded as:', buff[current:])
                self.keyCodes.add(self.prevCode, buff[current])
                self.prevCode = self.keyCodes.getNumKeys() - 1
            self.saver.add(buff, -current)
            self.current = 1 - self.current
            self.previousOffset = current
        # self.uncompressBytesTime += millis() - start

def decompress(data, uncompressed):
    uc = Uncompressor(DataSaver(uncompressed), Coder(bytearray(3 * len(data))), DataStore(data))
    uc.uncompressFirst()
    uc.uncompressBytes()

def decompressImage(data, image):
    uc = Uncompressor(ImageSaver(image), Coder(bytearray(3 * len(data))), DataStore(data))
    uc.uncompressFirst()
    uc.uncompressBytes()

def decompressImageStart(image, compressTable, store):
    uc = Uncompressor(ImageSaver(image), Coder(compressTable), store)
    uc.uncompressFirst()
    return uc
