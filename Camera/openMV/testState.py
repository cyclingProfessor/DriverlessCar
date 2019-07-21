from State import *
import unittest
from CompressUtil import compress

input1 = ['{', 'C', 'D', '1', '2', '3', '4', '}']
class TestReader(unittest.TestCase):
    def setUp(self):
        global rdr
        rdr = Reader()
    def test_Wait(self):
        rdr.handleChar(START)
        self.assertEqual(rdr.getStatus(), MSG_PARTIAL)
        self.assertIs(rdr._state, myStateCMD)
    def test_CMD(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        self.assertEqual(rdr.getStatus(), MSG_PARTIAL)
        self.assertEqual(rdr._state.message.command, 'C')
        self.assertEqual(rdr._state, myStateCount)
    def test_Count(self):
        rdr = Reader()
        for index in range(3):
            rdr.handleChar(input1[index])
        self.assertEqual(rdr.getStatus(), MSG_PARTIAL)
        self.assertEqual(rdr._state.message.expected, 4)
        self.assertEqual(rdr._state, myStateData)
    def test_Data(self):
        rdr = Reader()
        for index in range(7):
            rdr.handleChar(input1[index])
        self.assertEqual(rdr.getStatus(), MSG_PARTIAL)
        self.assertEqual(rdr._state.message.dataBuffer[0:rdr._state.message.expected], ['1', '2', '3', '4'])
        self.assertEqual(rdr._state, myStateEnd)
    def test_End(self):
        rdr = Reader()
        for index in range(8):
            rdr.handleChar(input1[index])
        self.assertEqual(rdr.getStatus(), MSG_GOOD)
        self.assertEqual(rdr._state, myStateWaiting)
    def test_Wait_Bad(self): # in state Waiting receive a bad input
        rdr.handleChar('X')
        self.assertIs(rdr._state, myStateWaiting)
    def test_CMD_Bad(self): # state CMD - receive bad input
        rdr = Reader()
        for index in range(1):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(rdr.getStatus(), MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Count_Bad(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(rdr.getStatus(), MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Data_Bad(self):
        rdr = Reader()
        for index in range(5):
            rdr.handleChar(input1[index])
        rdr.handleChar(END)
        self.assertEqual(rdr.getStatus(), MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_End_Bad(self):
        rdr = Reader()
        for index in range(7):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(rdr.getStatus(), MSG_GOOD)
        self.assertEqual(rdr._state, myStateWaiting)
    def test_Count_Recover(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        for index in range(8):
            rdr.handleChar(input1[index])
        self.assertEqual(rdr.getStatus(), MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_NOArgs(self):
        rdr = Reader()
        rdr.handleChar('{')
        rdr.handleChar('L')
        rdr.handleChar('@')
        rdr.handleChar('}')
        self.assertEqual(rdr.getStatus(), MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Image(self):
        rdr = Reader()
        rdr.handleChar('{')
        rdr.handleChar('P')
        data = bytes((index % 7) for index in range(3000))
        code = compress(data)
        rdr.handleChar(chr(len(code) >> 6 & 0x3F))
        rdr.handleChar(chr(len(code) & 0x3F))
        for value in code:
            rdr.handleChar(chr(value))
        rdr.handleChar('}')
        self.assertEqual(rdr.getStatus(), MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
        pixels = list(zip(data[::3], data[1::3], data[2::3]))
        for index in range(1000):
            self.assertSequenceEqual(rdr._state.message.picture[index], pixels[index])

if __name__ == '__main__':
    unittest.main()
