import cv2
from yolo import YOLO
from PIL import Image
import numpy as np
from os import listdir
import matplotlib.pyplot as plt
import time

def splitter(goal_dimension,image):
    image_width = image.shape[0]
    image_height = image.shape[1]

    images = []

    for img_in_width in range(0,image_width,goal_dimension[0]):
        buffer_array = []
        for img_in_height in range(0,image_height,goal_dimension[1]):
            buffer_array.append(image[img_in_width:img_in_width+goal_dimension[0],img_in_height:img_in_height+goal_dimension[0]])
        images.append(buffer_array)
        
    return images

def merge(goal_dimension,images):
    
    full_image = np.zeros((goal_dimension[1],goal_dimension[0],3),dtype=images[0][0].dtype)
    
    imshape = images[0][0].shape 
    for y,col in enumerate(images):
        for x,img in enumerate(col):
            full_image[y*imshape[0]:y*imshape[0]+imshape[0],x*imshape[1]:x*imshape[1]+imshape[1]] += img
    
    
    return full_image

def detect_img(img):
    pil_img = Image.fromarray(img)
    
    boxes,_,scores = yolonet.detect_image(pil_img)
    boxes = np.around(boxes).astype(int)
    scores = (scores*100).astype(int)
    
    if len(boxes):
        for index,box in enumerate(boxes):
            top, left, bottom, right = box
            top = max(0, np.floor(top + 0.5).astype('int32'))
            left = max(0, np.floor(left + 0.5).astype('int32'))
            bottom = min(pil_img.size[1], np.floor(bottom + 0.5).astype('int32'))
            right = min(pil_img.size[0], np.floor(right + 0.5).astype('int32'))

            ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,0,0),2)
            #font = cv2.FONT_HERSHEY_SIMPLEX
            #text = "{}%".format(str(scores[index]))
                
            #cv2_img = cv2.putText(cv2_img,text,(box[0],box[1]),font,0.25,(0,0,255),1,cv2.LINE_AA)
        return ret_img
    return img


yolonet = YOLO()


cam = cv2.VideoCapture(1)

while True:
    start = time.time()
    ack,img = cam.read()

    img = cv2.resize(img,(1024,768))

    #cv2_img = cv2.cvtColor(img,cv2.COLOR_BGR2RGB)
    cv2_img = img
    images = splitter((256,256),cv2_img)

    for x,column in enumerate(images):
        for y,image in enumerate(column):
            images[x][y] = detect_img(image)

    result = merge((1024,768),images)
    cv2.imshow("classified",result)
    print(time.time()-start)
    if cv2.waitKey(1)&0xFF == ord('q'):
        break

cam.release()
cv2.destroyAllWindows()