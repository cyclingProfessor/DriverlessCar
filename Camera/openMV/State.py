import image, sensor
from CompressUtil import decompressImageStart, Uncompressor, DataStore

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
        self.picture = image.Image(50,44,sensor.RGB565)
        self.decompressTable = bytearray(3000)

######################################################################################

class State:
    """
    Define an interface for encapsulating the behavior associated with a
    particular state of the Context.
    """
    message = Message() # Single class variable
    def __init__(self):
        self._name = "None"
    def handleChar(self, rdr, recvd):
        #print('Error : NO STATE')
        pass

class StateImageReader(State):
    def __init__(self):
        super().__init__()
        self.started = False;
        self._name = "Image Reader"
        self.width = 0
    def handleChar(self, rdr, recvd):
        if ord(recvd) > 100:
            self.started = False;
            print(self._name + " - got Bad:" + str(recvd))
            self.message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        # The first two bytes are big endian 6-bit Data (compressed) counts.
        if not self.started:
            self.message.expected = ord(recvd)
            self.started = True
            return

        self.message.expected = 64 * self.message.expected + ord(recvd)
        self.message.dataCount = 0
        self.started = False

        print("Expecting : " + str(self.message.expected))
        if self.message.expected > 1000: # Size of decode table
            print(self._name + " - got Bad Image Size:" + str(self.message.expected))
            self.message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        rdr._state = myReadPixels
        myReadPixels.begin()

class StateReadPixels(State):
    def __init__(self):
        super().__init__()
        self._name = "Pixel Reader"
        self._buffer = bytearray(3)
        self._uc = None
        self._ds = None
        self.__escaped = False
    def begin(self):
        self.__escaped = False
    def handleChar(self, rdr, recvd):
        if recvd in [END, START]: # Only two bad characters! premature!
            print(self._name + " - got Bad:" + str(recvd))
            self.message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        val = ord(recvd)
        if self.__escaped:
            val += 10
            self.__escaped = False
        elif recvd == '|':
            # wait until next character
            self.message.dataCount += 1
            self.__escaped = True
            return

        if self.message.dataCount < 3: # build up initial buffer
            self._buffer[self.message.dataCount] = val
            if self.message.dataCount == 2:
                self._ds = DataStore(self._buffer, streamed = True)
                self._uc = decompressImageStart(self.message.picture, self.message.decompressTable, self._ds)
        else:
            self._ds.setStoreToByte(val)
            self._uc.uncompressBytes()

        self.message.dataCount += 1
        if self.message.dataCount == self.message.expected:
            rdr._state = myStateEnd

class StateWaiting(State):
    def __init__(self):
        super().__init__()
        self._name = "Waiting"
    def handleChar(self, rdr, recvd):
        # Simply ignore no-start characters.
        if recvd != START:
            print(self._name + " - Still Waiting - got:" + str(recvd))
            return
        self.message.status = MSG_PARTIAL
        rdr._state = myStateCMD

class StateCMD(State):
    def __init__(self):
        super().__init__()
        self._name = "CMD"
    def handleChar(self, rdr, recvd):
        if not recvd in ['C', 'R', 'L', 'P']: # Possible commands
            print(self._name + " - got Bad:" + str(recvd))
            self.message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        # Check CMD validity
        self.message.status = MSG_PARTIAL
        self.message.command = recvd
        self.message.dataCount = 0
        if recvd == 'P': # Picture command
            rdr._state = myImageReader
            print(self._name + " - got a picture command")
            return
        rdr._state = myStateCount

class StateCount(State): # count is offset from 64 ('@') - So 'A' == 1
    def __init__(self):
        super().__init__()
        self._name = "Count"
    def handleChar(self, rdr, recvd):
        count = ord(recvd) - ord('@')
        if count > 20:
            print(self._name + " - got Bad:" + str(recvd))
            self.message.status = MSG_BAD
            rdr._state = myStateWaiting
            return
        # Check Count validity (Certainly < 20)
        # If expected = 0 then straight to END
        self.message.expected = count
        self.message.dataCount = 0
        if self.message.expected == 0:
            rdr._state = myStateEnd
        else:
            rdr._state = myStateData

class StateData(State):
    def __init__(self):
        super().__init__()
        self._name = "Data"
    def handleChar(self, rdr, recvd):
        if recvd < '0' or recvd > '9':
            print(self._name + " - got Bad:" + str(recvd))
            rdr._state = myStateWaiting
            self.message.status = MSG_BAD
            return
        self.message.dataBuffer[self.message.dataCount] = recvd
        self.message.dataCount += 1
        if self.message.dataCount >= self.message.expected:
            rdr._state = myStateEnd

class StateEnd(State):
    def __init__(self):
        super().__init__()
        self._name = "End"
    def handleChar(self, rdr, recvd):
        if recvd != END:
            print(self._name + " - got Bad:" + str(recvd))
            self.message.status = MSG_BAD
        else:
            self.message.status = MSG_GOOD # Got a whole good self.message.
        rdr._state = myStateWaiting

## SInce states are stateless (sic) we can re-use them
myStateWaiting = StateWaiting()
myStateCMD = StateCMD()
myStateCount = StateCount()
myStateData = StateData()
myStateEnd = StateEnd()
myImageReader = StateImageReader()
myReadPixels = StateReadPixels()
class Reader:
    """ This is the state pattern to respond to UART tokens

    Some are busy and others are quiescent.  It uses a start character an finishes with an end cahracter
    """
    def __init__(self):
        self._state = myStateWaiting

    def getStatus(self):
        return self._state.message.status

    def getMessage(self):
        return self._state.message;

    def handleChar(self, recvd):
        self._state.handleChar(self, recvd)
