import Serial
import time

mySerial = Serial.Serial()

# mySerial needs a few seconds to establish connection
time.sleep(5)

while( 1 ):
    data = raw_input("> ")
    mySerial.write( data )
    time.sleep(0.01)
    #mySerial.read()
    #time.sleep(2)
    response = mySerial.read()
    print( response )
