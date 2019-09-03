import cv2
import numpy as np
import tkinter as tk
from PIL import Image, ImageTk
from keras_yolo3.yolo import YOLO
from UART_Con import MCRoboarm
from time import sleep


class YOLO_Manager(YOLO):
    def __init__(self):
        super().__init__()


    def splitter(self,goal_dimension,image):
        image_width = image.shape[0]
        image_height = image.shape[1]

        images = []

        for img_in_width in range(0,image_width,goal_dimension[0]):
            buffer_array = []
            for img_in_height in range(0,image_height,goal_dimension[1]):
                buffer_array.append(image[img_in_width:img_in_width+goal_dimension[0],img_in_height:img_in_height+goal_dimension[0]])
            images.append(buffer_array)
            
        return images
    
    def merge(self,goal_dimension,images):
        full_image = np.zeros((goal_dimension[1],goal_dimension[0],3),dtype=images[0][0].dtype)
        
        imshape = images[0][0].shape 
        for y,col in enumerate(images):
            for x,img in enumerate(col):
                full_image[y*imshape[0]:y*imshape[0]+imshape[0],x*imshape[1]:x*imshape[1]+imshape[1]] += img
        
        
        return full_image

    def detect_img(self,orig_img):
        img = orig_img
        pil_img = Image.fromarray(img)
        
        boxes,_,scores = self.detect_image(pil_img)
        boxes = np.around(boxes).astype(int)
        scores = (scores*100).astype(int)
        
        return boxes

    def analyze(self,img):
        images = self.splitter((256,256),img)
        all_boxes = []
        for col_nr,column in enumerate(images):
            for ind,image in enumerate(column):
                all_boxes.append(self.detect_img(image))
        return all_boxes



class GUI(tk.Tk):
    def __init__(self,):
        super().__init__()
        
        self.take_shot = 0
        self.img_label = tk.Label(self)
        self.img_label.pack()
        self.tk_shot_btn = tk.Button(self,text="Analyze",command=self.set_take_shot)
        self.tk_shot_btn.pack()

    def set_take_shot(self):
        self.take_shot = not self.take_shot



class Application:
    def __init__(self):
        self.myGui = GUI()
        self.yolo = YOLO_Manager()
        self.MCRobo = MCRoboarm("00:06:66:76:52:C3",1)
        self.cam = cv2.VideoCapture(0)

    def analyze(self,img):
        boxes = self.yolo.analyze(img)
        
        finale_boxes = []
        if len(boxes)>0:
            for img_nr,all_box in enumerate(boxes):
                if len(all_box)>0:
                    for box in all_box:
                        box[1] += int(img_nr%4)*256 #left
                        box[3] += int(img_nr%4)*256 #right
                        box[0] += int(img_nr/4)*256 #top
                        box[2] += int(img_nr/4)*256 #bottom
                        center = (int((box[3]-box[1])/2)+box[1],int((box[2]-box[0])/2)+box[0])

                        finale_boxes.append(center)

                        top, left, bottom, right = box
                        ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,0,0),2)
             
        return ret_img,finale_boxes
    
    def analyze_joints(self,boxes):
        for center in boxes[1:]:
            ###############################
            #send coordinates to robo arm
            self.MCRobo.send(center)
            # wait for ack.
            ack,succ = self.MCRobo.recieve()
            if not(succ and ack):
                raise Exception("Kommunikation transfär Error")
            ack,succ = self.MCRobo.recieve()
            if not(succ and ack):
                raise Exception("Kommunikation Error X")
            ack,succ = self.MCRobo.recieve()
            if not(succ and ack):
                raise Exception("Kommunikation Error Y")
            #wait for movement
            fin,succ = self.MCRobo.recieve()
            if not(succ and fin):
                raise Exception("Socked Down")
                
            joint_img = self.take_pic()
            sleep(1)
            self.show_image_in_tk(joint_img)
            sleep(0.1)
            
            # use second AI
            #get result
            ###############################
        self.MCRobo.sock.send("O\n")
        
    def take_pic(self):
        res,img = self.cam.read()
        img = cv2.resize(img,(1024,768))
        img = cv2.cvtColor(img,cv2.COLOR_BGR2RGB)
        
        return img
    
    def show_image_in_tk(self,img):
        img = cv2.resize(img,(480,360))
        im = Image.fromarray(img)
        imgtk = ImageTk.PhotoImage(image=im)
        self.myGui.img_label.configure(image=imgtk,text="")
        self.myGui.img_label.image = imgtk

        self.myGui.update()
    
    def run(self):
        try:
            while True:
                #Take image
                res,img = self.cam.read()

                if res:
                    #resize to worksize
                    img = cv2.resize(img,(1024,768))
                    img = cv2.cvtColor(img,cv2.COLOR_BGR2RGB)
                    
                    #if analysing initialized
                    if self.myGui.take_shot:
                        self.myGui.tk_shot_btn.configure(text="Stop")
                        
                        img,boxes = self.analyze(img)
                        
                        self.show_image_in_tk(img)
                        sleep(3)
                        self.analyze_joints(boxes)
                        while self.myGui.take_shot:
                            self.myGui.update()
                        
                    else:
                        img = cv2.resize(img,(480,360))
                        im = Image.fromarray(img)
                        imgtk = ImageTk.PhotoImage(image=im)

                        # Put it in the display window
                        self.myGui.img_label.configure(image=imgtk,text="")
                        self.myGui.img_label.image = imgtk
                        self.myGui.tk_shot_btn.configure(text="Analyze")
                else:
                    self.myGui.img_label.configure(text="CAM ERROR!!!!!!")
                
                self.myGui.update()
        finally:
            self.cam.release()

if __name__ == "__main__":
    app = Application()
    app.run()