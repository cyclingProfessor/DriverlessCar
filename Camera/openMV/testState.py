from State import *
import unittest

input1 = ['{', 'C', 'D', '1', '2', '3', '4', '}']
class TestReader(unittest.TestCase):
    def setUp(self):
        global rdr
        rdr = Reader()
    def test_Wait(self):
        rdr.handleChar(START)
        self.assertEqual(message.status, MSG_PARTIAL)
        self.assertIs(rdr._state, myStateCMD)
    def test_CMD(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        self.assertEqual(message.status, MSG_PARTIAL)
        self.assertEqual(message.command, 'C')
        self.assertEqual(rdr._state, myStateCount)
    def test_Count(self):
        rdr = Reader()
        for index in range(3):
            rdr.handleChar(input1[index])
        self.assertEqual(message.status, MSG_PARTIAL)
        self.assertEqual(message.expected, 4)
        self.assertEqual(rdr._state, myStateData)
    def test_Data(self):
        rdr = Reader()
        for index in range(7):
            rdr.handleChar(input1[index])
        self.assertEqual(message.status, MSG_PARTIAL)
        self.assertEqual(message.dataBuffer[0:message.expected], ['1', '2', '3', '4'])
        self.assertEqual(rdr._state, myStateEnd)
    def test_End(self):
        rdr = Reader()
        for index in range(8):
            rdr.handleChar(input1[index])
        self.assertEqual(message.status, MSG_GOOD)
        self.assertEqual(rdr._state, myStateWaiting)
    def test_Wait_Bad(self): # in state Waiting receive a bad input
        rdr.handleChar('X')
        self.assertIs(rdr._state, myStateWaiting)
    def test_CMD_Bad(self): # state CMD - receive bad input
        rdr = Reader()
        for index in range(1):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(message.status, MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Count_Bad(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(message.status, MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Data_Bad(self):
        rdr = Reader()
        for index in range(5):
            rdr.handleChar(input1[index])
        rdr.handleChar(END)
        self.assertEqual(message.status, MSG_BAD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_End_Bad(self):
        rdr = Reader()
        for index in range(7):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        self.assertEqual(message.status, MSG_BAD)
        self.assertEqual(rdr._state, myStateWaiting)
    def test_Count_Recover(self):
        rdr = Reader()
        for index in range(2):
            rdr.handleChar(input1[index])
        rdr.handleChar('X')
        for index in range(8):
            rdr.handleChar(input1[index])
        self.assertEqual(message.status, MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_NOArgs(self):
        rdr = Reader()
        rdr.handleChar('{')
        rdr.handleChar('L')
        rdr.handleChar('@')
        rdr.handleChar('}')
        self.assertEqual(message.status, MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
    def test_Image(self):
        rdr = Reader()
        rdr.handleChar('{')
        rdr.handleChar('P')
        rdr.handleChar(chr(44))
        rdr.handleChar(chr(33))
        for index in range(44 * 33):
            rdr.handleChar(chr(254))
            rdr.handleChar(chr(124))
            rdr.handleChar(chr(0))
        rdr.handleChar('}')
        self.assertEqual(message.status, MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
        self.assertEqual(message.picture.width(), 44)
        self.assertEqual(message.picture.height(), 33)
        self.assertSequenceEqual(message.picture[199], (254, 124, 0))

if __name__ == '__main__':
    unittest.main()
