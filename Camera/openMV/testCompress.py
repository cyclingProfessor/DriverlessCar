from CompressUtil import *
from random import seed, randrange
import unittest

class TestReader(unittest.TestCase):
    def test_Basic(self):
        data = b"I am a String"
        code = compress(data)
 #       print(bin(int.from_bytes(code, 'big')))
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)
    def test_Simple(self):
        data = b"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ"
        code = compress(data)
 #       print(bin(int.from_bytes(code, 'big')))
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)

    def test_Random(self):
        seed(0)
        data = bytes(randrange(255) for i in range(10000))
        code = compress(data)
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)

    def test_Image(self):
        fp = open('IMAGE.ppm', 'rb')
        data = fp.read()
        fp.close()
        code = compress(data)
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)

    def test_Escape(self):
        data = b'{|}'
        self.assertIn(b"{|}", data)
        code = compress(data)
        # Raw code is 01111100, 01110001, 00111110, 00011111, 01000000
        # After escape processing:
        #     code is 01111011, 00111110, 00011111, 01000000
        #    which is 01111011 (123), 001111100 (124), 001111101 (125)

#        print(code)
#        print(bin(int.from_bytes(code, 'big')))
        self.assertNotIn(b"{", code)
        self.assertNotIn(b"}", code)
        self.assertEqual(code.count(b"|"), 1)
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)

    def test_Escapes(self):
        data = b'{|}'
        data += bytes(range(256))
        self.assertIn(b"{|}", data)
        code = compress(data)
        self.assertNotIn(b"{", code)
        self.assertNotIn(b"}", code)
        self.assertEqual(code.count(b"|"), 2)
        uncompressed = bytearray(len(data))
        decompress(code, uncompressed)
        self.assertSequenceEqual(data, uncompressed)

    def test_RGB565(self):
        seed(0)
        data = bytes(randrange(255) for i in range(3333))
        code = compress(data)
        image = [None] * 1111
        decompressImage(code, image)
        self.assertSequenceEqual(data, [item for sublist in image for item in sublist])

if __name__ == '__main__':
    unittest.main()
