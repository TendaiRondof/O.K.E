import cv2
from yolo import YOLO
from PIL import Image
import numpy as np
from os import listdir
import matplotlib.pyplot as plt

yolonet = YOLO()

for file in listdir("Data_set/Images")[-20:]:
    cv2_img = cv2.imread("Data_set/Images/"+file)
    pil_img = Image.fromarray(cv2_img)
    
    boxes,classes,scores = yolonet.detect_image(pil_img)
    boxes = np.around(boxes).astype(int)
    scores = (scores*100).astype(int)
    
    if len(boxes):
        for index,box in enumerate(boxes):
            top, left, bottom, right = box
            top = max(0, np.floor(top + 0.5).astype('int32'))
            left = max(0, np.floor(left + 0.5).astype('int32'))
            bottom = min(pil_img.size[1], np.floor(bottom + 0.5).astype('int32'))
            right = min(pil_img.size[0], np.floor(right + 0.5).astype('int32'))

            cv2_img = cv2.rectangle(cv2_img,(left,top),(right,bottom),(255,0,0),1)
            font = cv2.FONT_HERSHEY_SIMPLEX
            text = "{}%".format(str(scores[index]))
                
            cv2_img = cv2.putText(cv2_img,text,(box[1],box[0]),font,0.30,(0,255,128),1,cv2.LINE_AA)
        
        plt.imshow(cv2_img)
        plt.show()



