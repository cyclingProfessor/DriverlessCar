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
        fl = open("testImage", "rb")
        img = fl.read()
        fl.close()
        rdr = Reader()
        rdr.handleChar('{')
        bCount = len(img).to_bytes(2, byteorder='little')
        rdr.handleChar('P')
        rdr.handleChar(bCount[0])
        rdr.handleChar(bCount[1])
        for index in range(len(img)):
            rdr.handleChar(img[index])
        rdr.handleChar('}')
        fl = open("IMAGE", "rb")
        imgNew = fl.read()
        fl.close()
        self.assertEqual(message.status, MSG_GOOD)
        self.assertIs(rdr._state, myStateWaiting)
        self.assertSequenceEqual(imgNew, img)

if __name__ == '__main__':
    unittest.main()
