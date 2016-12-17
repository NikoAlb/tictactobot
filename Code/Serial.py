import serial
import sys
from collections import deque

# Note that bit masks are string representations of hex values
DATA_RECEIVED = 0xff
BUFFER_SIZE = 60 # to be safe
PORT = "COM3"
BAUDRATE = 9600


""" 
The pyserial library proved to be very tedious and prone to errors when initialization steps are forgotten or
when a read happens when there is nothing to read or when more than one character is sent.                  
"""
class Serial:
    
    def __init__(self):
        self.bufferSize = 0 # Keeps track of bytes sent to prevent buffer overflow
        
        self.ser = serial.Serial()
        self.ser.port = PORT
        self.ser.baudrate = BAUDRATE
        self.ser.open()
        self.writeBuffer = deque( [] ) # implementation of a queue

    def write(self, data):
        if( self.ser.isOpen() ):
            self.ser.write( data )
            #self.bufferSize += 1
            
        else: #( !sys.set.isOpen() ):
            print("Attempted write when serial port was not open")
        
    def read(self):
        if( self.ser.isOpen() ):
            if( self.ser.inWaiting() ):
                response = self.ser.read() #reads one byte
                return response
            else:
                print( "No data to read" )
                return ""
        else:
            print("Attempted read when serial port was not open")
            return ""

    def getBufferSize(self):
        return self.bufferSize

    def getWriteBuffer(self):
        return self.writeBuffer

        
