import bluetooth
import sys
from time import sleep

class MCRoboarm:
    def __init__(self,bd_addr,port):
#        "00:06:66:76:52:C3",port = 1
        self.bd_addr = bd_addr
        self.port = port
        self.sock = bluetooth.BluetoothSocket( bluetooth.RFCOMM )
        self.sock.connect((bd_addr, port))
        sleep(1)
        print('Connected')
    
    def send(self,center_tuple):
        x = center_tuple[0]
        y = center_tuple[1]
        if x<1000:
            if x<100:
                if x<10:
                    x = "000"+str(x)
                else:
                    x = "00"+str(x)
            else:
                x = "0"+str(x)
                
        if y<1000:
            if y<100:
                if y<10:
                    y = "000"+str(y)
                else:
                    y = "00"+str(y)
            else:
                y = "0"+str(y)
        self.sock.send("X{}Y{}\n".format(x,y))
        
    def recieve(self):
        try:
            ack = int(self.sock.recv(1))
            return ack,1
        except Exception as e:
            return e,0
    
    def disconnect(self):
        sock.close()