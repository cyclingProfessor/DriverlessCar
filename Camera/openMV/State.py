START = '{' # 123
END = '}' # 125
MSG_GOOD = 1
MSG_PARTIAL = 0
MSG_BAD = -1
MSG_NONE = -2

# A message
# START: CMD: Length: Data : End
# e.g 8: 'D': 3: '1': '2' : '3' : 10

class Message:
    def __init__(self):
        self.command = None
        self.dataBuffer = [None for i in range(20)]
        self.status = MSG_NONE # -2 for None yet, -1 for error, 0 for partial, 1 for complete
        self.dataCount = 0
        self.expected = -1

######################################################################################
message = Message()

class State:
    """
    Define an interface for encapsulating the behavior associated with a
    particular state of the Context.
    """
    def __init__(self):
        self._name = "None"
    def handleChar(self, rdr, recvd):
        #print('Error : NO STATE')
        pass

class StateImageReader(State):
    def __init__(self):
        global message
        self.called = 0;
        self._name = "BMP Image Reader"
    def handleChar(self, rdr, recvd):
        global message
        # The first two bytes are little endian length of file.
        if self.called == 0:
            message.dataCount += recvd
        else:
            message.dataCount += 256 * recvd
        self.called += 1
        if self.called < 2:
            return
        if message.dataCount > 10000:
            print(self._name + " - got Bad Image Length:" + message.dataCount)
            message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        rdr._state = myReadFile

class StateReadFile(State):
    def __init__(self):
        self._name = "Image File Reader"
        self.fl = open("IMAGE", "wb+")
    def handleChar(self, rdr, recvd):
        global message
        self.fl.write(bytes([recvd]))
        message.dataCount -= 1
        if message.dataCount > 0:
            return
        self.fl.close()
        rdr._state = myStateEnd

class StateWaiting(State):
    def __init__(self):
        self._name = "Waiting"
    def handleChar(self, rdr, recvd):
        # Simply ignore no-start characters.
        if recvd != START:
            print(self._name + " - Still Waiting - got:" + str(recvd))
            return
        message.status = MSG_PARTIAL
        rdr._state = myStateCMD

class StateCMD(State):
    def __init__(self):
        self._name = "CMD"
    def handleChar(self, rdr, recvd):
        global message
        if not recvd in ['C', 'R', 'L', 'P']: # Possible commands
            print(self._name + " - got Bad:" + str(recvd))
            message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        # Check CMD validity
        message.status = MSG_PARTIAL
        message.command = recvd
        message.dataCount = 0
        if recvd == 'P': # Picture command
            rdr._state = myImageReader
            print(self._name + " - got a picture command")
            return
        rdr._state = myStateCount

class StateCount(State): # count is offset from 64 ('@') - So 'A' == 1
    def __init__(self):
        self._name = "Count"
    def handleChar(self, rdr, recvd):
        global message
        count = ord(recvd) - ord('@')
        if count > 20:
            print(self._name + " - got Bad:" + str(recvd))
            message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        # Check Count validity (Certainly < 20)
        # If expected = 0 then straight to END
        message.expected = count
        message.dataCount = 0
        if message.expected == 0:
            rdr._state = myStateEnd
        else:
            rdr._state = myStateData

class StateData(State):
    def __init__(self):
        self._name = "Data"
    def handleChar(self, rdr, recvd):
        global message
        if recvd < '0' or recvd > '9':
            print(self._name + " - got Bad:" + str(recvd))
            rdr._state = myStateWaiting
            message.status = MSG_BAD
            return
        message.dataBuffer[message.dataCount] = recvd
        message.dataCount += 1
        if message.dataCount >= message.expected:
            rdr._state = myStateEnd

class StateEnd(State):
    def __init__(self):
        self._name = "End"
    def handleChar(self, rdr, recvd):
        global message
        if recvd != END:
            print(self._name + " - got Bad:" + str(recvd))
            message.status = MSG_BAD
        else:
            message.status = MSG_GOOD # Got a whole good message.
        rdr._state = myStateWaiting

## SInce states are stateless (sic) we can re-use them
myStateWaiting = StateWaiting()
myStateCMD = StateCMD()
myStateCount = StateCount()
myStateData = StateData()
myStateEnd = StateEnd()
myImageReader = StateImageReader()
myReadFile = StateReadFile()
class Reader:
    """ This is the state pattern to respond to UART tokens

    Some are busy and others are quiescent.  It uses a start character an finishes with an end cahracter
    """
    def __init__(self):
        self._state = myStateWaiting

    def handleChar(self, recvd):
        self._state.handleChar(self, recvd)
