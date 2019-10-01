import bluetooth
import sys
from time import sleep
bd_addr = "00:06:66:76:52:C3"

port = 1
sock = bluetooth.BluetoothSocket( bluetooth.RFCOMM )
sock.connect((bd_addr, port))
sleep(1)
print('Connected')

while input("send")!= "n":
    sock.send("X1024Y0768\n")
    print("sent")

    sleep(0.1)
    print("1",sock.recv(1))
    sleep(0.01)
    print("2",sock.recv(1))
    sleep(0.01)
    print("3",sock.recv(1))
    sleep(0.02)
    print("4",sock.recv(1))

sock.close()