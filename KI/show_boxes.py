import cv2
from time import sleep


with open("train.txt","r") as f:
    for line in f.readlines():
        splitted_line = line.split(" ")
        img = cv2.imread(splitted_line[0])
        boxes = []
        for box_str in splitted_line[1:]:
            box = [int(val) for val in box_str.split(",")[:-1]]
            print(box)
            if len(box)>0:
                cv2.rectangle(img,(box[0],box[1]),(box[2],box[3]),(255,0,0),2)
                
        
        cv2.imshow("Image",img)
        
        if cv2.waitKey(1) & 0xff == ord("q"):
            break
        
        sleep(0.1)
