# Untitled - By: dave - Wed Jul 10 2019
from math import floor, ceil
# from pyb import millis
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

    def setStoreToByte(self, ch):
        # Add next character to the end of the array
        # start = millis()
        for index in range(2):
            self.data[0 + index] = self.data[1 + index]
        self.data[2] = ch
        self.__byte_offset -= 1
        # self.storeByteTime += millis() - start

    def getFirst(self):
#        print('Initial data bytes', self.data[0], self.data[1])
        retval = self.__processEscape() # The first byte
        self.__set_current() # increments __byte_offset and Sets __current to the (second) data byte
#        print(self.__byte_offset)
        return retval

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

class ImageSaver(Saver):
    def __init__(self, img):
        super().__init__(img)
        self.__colorBuffer = [0,0,0]
        self.__colorCount = 0
    def add(self, colors, length):
        for index in range(length):
            self.__colorBuffer[self.__colorCount] = colors[-length + index]
            self.__colorCount += 1
            if self.__colorCount == 3:
                self.data[self.outCount] = self.__colorBuffer.copy()
                self.outCount += 1
                self.__colorCount = 0
        
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
        num = 256 * code + iletter
        val = num.to_bytes(3, 'big')
        offset = 3 * (self.__n_keys - 256)
        self.__keys[offset: offset + 3] = val
        self.__n_keys += 1
        # self.addTime += millis() - start
    def decode(self, code, buffer):
        # print('Decoding' + hex(code)[2:])
        # start = millis()
        if code < 256:
            buffer[-1] = code
            return -1
        retval = -1
        while code >= 256:
            if code >= self.__n_keys:
                raise KeyError()
            buffer[retval] = self.__keys[3 * (code - 256) + 2]
            retval -= 1
            code = int.from_bytes(self.__keys[3 * (code - 256):3 * (code - 256) + 2],'big')
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
        self.previous = None
        self.prevCode = None
        self.ds = dataStore
        self.buffer = bytearray(100)
	# For profiling
        # self.uncompressNextTime = 0
        # self.uncompressBytesTime = 0
        # self.currentIndex = 0
        # self.buffer = (bytearray(100), bytearray(100))

    def uncompressNext(self, ch):
#        print('Uncompress Next Byte:', ch)
        # start = millis()
        self.ds.setStoreToByte(ch)
        self.uncompressBytes()
        # self.uncompressNextTime += millis() - start

    def uncompressFirst(self):
        code = self.ds.getFirst()
        self.previous = [code]
        self.prevCode = code
        self.saver.add(self.previous, 1)

    def uncompressBytes(self):
        # start = millis()
        while self.ds.hasNextCode(self.keyCodes.getAndUpdateKeySize()):
            code = self.ds.getNext(self.keyCodes.getKeySize())
#            print('code ' + hex(code)[2:] + ' (' + str(self.keyCodes.getKeySize()) + ')')
            try:
                currentOffset = self.keyCodes.decode(code, self.buffer)
                self.keyCodes.add(self.prevCode, self.buffer[currentOffset])
                self.prevCode = code
#                print('Current:', current)
            except KeyError:
                currentOffset = -1 * len(self.previous) - 1
                self.buffer[currentOffset:] = self.previous + self.previous[:1]
                self.keyCodes.add(self.prevCode, self.buffer[currentOffset])
                self.prevCode = self.keyCodes.getNumKeys() - 1
            self.saver.add(self.buffer, -currentOffset)
            self.previous = self.buffer[currentOffset:]
        # self.uncompressBytesTime += millis() - start

def decompress(data, uncompressed):
    uc = Uncompressor(DataSaver(uncompressed), Coder(bytearray(3 * len(data))), DataStore(data))
    uc.uncompressFirst()
    uc.uncompressBytes()

def decompressImage(data, image):
    uc = Uncompressor(ImageSaver(image), Coder(bytearray(3 * len(data))), DataStore(data))
    uc.uncompressFirst()
    uc.uncompressBytes()

def decompressImageStart(image, compressTable, recvd):
    uc = Uncompressor(ImageSaver(image), Coder(compressTable), DataStore(recvd, streamed = True))
    uc.uncompressFirst()
    return uc

# Deals with escapes outside
def decompressImageProcess(uc, recvd):
    uc.uncompressNext(recvd)
