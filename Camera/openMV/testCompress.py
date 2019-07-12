from Compress import *
from random import seed, randrange
import unittest

class TestReader(unittest.TestCase):
    def test_Simple(self):
        data = b"I am a string"
        code = compress(data)
        uncompressed = decompress(code)
        self.assertSequenceEqual(data, uncompressed)

    def test_Random(self):
        seed(0)
        data = bytes(randrange(255) for i in range(10000))
        code = compress(data)
        uncompressed = decompress(code)
        self.assertSequenceEqual(data, uncompressed)

    def test_Image(self):
        fp = open('IMAGE.ppm', 'rb')
        data = fp.read()
        fp.close()
        code = compress(data)
        uncompressed = decompress(code)
        self.assertSequenceEqual(data, uncompressed)

    def test_Escape(self):
        data = b'{|}'
        data += bytes(range(256))
        self.assertIn(b"{|}", data)
        code = compress(data)
        self.assertNotIn(b"{", code)
        self.assertNotIn(b"}", code)
        self.assertEqual(code.count(b"|"), 2)
        uncompressed = decompress(code)
        self.assertSequenceEqual(data, uncompressed)

if __name__ == '__main__':
    unittest.main()
