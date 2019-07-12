# Untitled - By: dave - Wed Jul 10 2019
from math import floor, ceil

ASCII_TO_INT = {i.to_bytes(1, 'big'): i for i in range(256)}
INT_TO_ASCII = {i: b for b, i in ASCII_TO_INT.items()}

def compress(data) -> bytes:
    if isinstance(data, str):
        data = data.encode()
    keys = ASCII_TO_INT.copy()
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
                # If n_keys is a power of two then increase output length
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
    return retval

# Now uses adaptive length.
def decompress(data) -> bytes:
    if isinstance(data, str):
        data = data.encode()
    keys = INT_TO_ASCII.copy()
    # Concatenate a list of the binary representation of each byte in the data.
    bits = ""
    escape = False
    for value in data:
        if value == ord('|'):
            escape = True
            continue
        n = bin(value)[2:] if not escape else bin(value + 10)[2:]
        escape = False
        n = ('0' * (8 - len(n))) + n
        bits += n

    n_keys = 256
    key_size = 8
    offset = 0
    previous = keys[int(bits[offset: offset + key_size],2)]
    offset += key_size
    uncompressed = [previous]
    while len(bits) - offset >= key_size:
        if ((n_keys & (n_keys - 1)) == 0):
            key_size += 1
        i = int(bits[offset: offset + key_size],2)
        #print ('Dcode ' + hex(i)[2:] + ' (' + str(key_size) + ')')
        offset += key_size
        try:
            current = keys[i]
        except KeyError:
            current = previous + previous[:1]
        uncompressed.append(current)
        keys[n_keys] = previous + current[:1]
        previous = current
        n_keys += 1
    return b''.join(uncompressed)

