import cv2
import numpy as np
import tkinter as tk
from PIL import Image, ImageTk
from keras_yolo3.yolo import YOLO














take_shot = False
run = True
yolonet = YOLO()

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
        for box in boxes:
            top, left, bottom, right = box
            top = max(0, np.floor(top + 0.5).astype('int32'))
            left = max(0, np.floor(left + 0.5).astype('int32'))
            bottom = min(pil_img.size[1], np.floor(bottom + 0.5).astype('int32'))
            right = min(pil_img.size[0], np.floor(right + 0.5).astype('int32'))

            ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,0,0),2)
            #font = cv2.FONT_HERSHEY_SIMPLEX
            #text = "{}%".format(str(scores[index]))
                
            #cv2_img = cv2.putText(cv2_img,text,(box[0],box[1]),font,0.25,(0,0,255),1,cv2.LINE_AA)
        return ret_img,boxes
    return img,None

def yolo_analyze(img):
    images = splitter((256,256),img)
    all_boxes = []
    for x,column in enumerate(images):
        for y,image in enumerate(column):
            images[x][y],boxes = detect_img(image)
            all_boxes.append(boxes)


    result_img = merge((1024,768),images)
    return result_img,all_boxes

def set_take_shot():
    global take_shot
    take_shot = not take_shot

if __name__ == "__main__":

    screen = tk.Tk()
    screen.title("First Shot")

    cam = cv2.VideoCapture(0)

    img_label = tk.Label(screen)
    img_label.pack()
    tk_shot_btn = tk.Button(screen,text="Analyze",command=lambda: set_take_shot())
    tk_shot_btn.pack()

    while run:
        #Take image
        res,img = cam.read()

        if res:

            img = cv2.resize(img,(1024,768))
            img = cv2.cvtColor(img,cv2.COLOR_BGR2RGB)

            if take_shot:
                img,boxes = yolo_analyze(img)
                print(np.array(boxes).shape)
                tk_shot_btn.configure(text="Stop")

                im = Image.fromarray(img)
                imgtk = ImageTk.PhotoImage(image=im)

                # Put it in the display window
                img_label.configure(image=imgtk,text="")
                img_label.image = imgtk

                screen.update()
                while take_shot:
                    screen.update()
                
            else:
                im = Image.fromarray(img)
                imgtk = ImageTk.PhotoImage(image=im)

                # Put it in the display window
                img_label.configure(image=imgtk,text="")
                img_label.image = imgtk
                tk_shot_btn.configure(text="Analyze")
        else:
            img_label.configure(text="CAM ERROR!!!!!!")
        
        screen.update()

    cam.release()
    