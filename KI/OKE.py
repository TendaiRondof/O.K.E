import cv2
import numpy as np
import tkinter as tk
from PIL import Image, ImageTk
from keras_yolo3.yolo import YOLO
from UART_Con import MCRoboarm
from time import sleep
import random
from tqdm import tqdm
from os import listdir

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
    def __init__(self,parent):
        super().__init__()
        self.bind("<F11>",self.toggle_fullscreen)
        self.bind("<F4>",self.kill_programm)
        self.fullscreen = True
        self.attributes("-fullscreen",self.fullscreen )
        self.parent = parent
        self.take_shot = 0
        self.info_label = tk.Label(self,text="Starting...")
        self.info_label.pack()
        self.img_label = tk.Label(self)
        self.img_label.pack()
        self.tk_shot_btn = tk.Button(self,text="Analyze",bg="gray",command=self.set_take_shot)
        self.tk_shot_btn.pack()
        
    def toggle_fullscreen(self,x):
        self.fullscreen = not self.fullscreen
        self.attributes("-fullscreen",self.fullscreen)

    def kill_programm(self):
        exit()

    def set_take_shot(self):
        self.take_shot = not self.take_shot
        
        if self.take_shot:
            self.tk_shot_btn.configure(text="Analysing...",bg="green")
        else:
            self.tk_shot_btn.configure(text="Start",bg="gray")
        self.update()
        
class Application:
    def __init__(self):
        self.myGui = GUI(self)
        self.print("Starting...")
        self.yolo = YOLO_Manager()
        self.print("AI 1 has been loded")
        self.MCRobo = MCRoboarm("00:06:66:EC:07:8A",1)
        self.print("Röbi is connected")        
        self.cam = cv2.VideoCapture(1)
        self.print("Cameras is conected")

    def analyze(self,img):
        boxes = self.yolo.analyze(img)
        
        centers = []
        return_boxes = []
        if len(boxes)>0:
            for img_nr,all_box in enumerate(boxes):
                if len(all_box)>0:
                    for box in all_box:
                        box[1] += int(img_nr%4)*256 #left
                        box[3] += int(img_nr%4)*256 #right
                        box[0] += int(img_nr/4)*256 #top
                        box[2] += int(img_nr/4)*256 #bottom
                        center = (int((box[3]-box[1])/2)+box[1],int((box[2]-box[0])/2)+box[0])

                        centers.append(center)
                        return_boxes.append(box)

                        top, left, bottom, right = box
                        ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,0,0),2)
        else:
            return None,centers,return_boxes
             
        return ret_img,centers,return_boxes
    
    def analyze_joints(self,centers):
        results = []
        
        self.print("{} Solderjoints have been found".format(len(centers)))
        sleep(10)
        print("Mitte: 512,384")
        for index,center in enumerate(centers):
            if not self.myGui.take_shot:
                break
            self.print("Solder Joint: {} From {}".format(index+1,len(centers)))
            ###############################
            #send coordinates to robo arm
            self.MCRobo.send(center)
            # wait for ack.
            try:
                ack,succ = self.MCRobo.recieve()
            except:
                print("sent a 'D' again")
                ack,succ = self.MCRobo.recieve()
                
            if not(succ and ack):
                raise Exception("Kommunikation transfähr Error, röbi sent {}".format(ack))
            x,y = self.MCRobo.sock.recv(10).decode().split(" ")
            x = int(x)
            y = int(y)
            
            if (y,x) != center:
                self.print("Not same Coordinates recieved:{}, expected: {}".format((x,y),center))
                self.MCRobo.sock.send("E")
            self.MCRobo.sock.send("G")
            
            #wait for movement
            fin,succ = self.MCRobo.recieve()
            if not(succ and fin):
                raise Exception("Socked Down")
            sleep(2)                
            joint_img = self.take_pic()
            self.show_image_in_tk(joint_img)
            sleep(0.5)
            # use second AI
            results.append(int((random.random()*1000))%3)
            #get result
            ###############################
            
        #send Jan finish signal
        self.MCRobo.sock.send("O")
        print("Ende")
        
        return results
        
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
        
    def draw_result(self,img,boxes,results):
        if len(results)>0:
            for i,result in enumerate(results):
                top, left, bottom, right = boxes[i]
                if result == 0:
                    ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,0,0),2)
                elif result == 1:
                    ret_img = cv2.rectangle(img,(left,top),(right,bottom),(255,255,0),2)
                elif result == 2:
                    ret_img = cv2.rectangle(img,(left,top),(right,bottom),(0,255,0),2)
            self.show_image_in_tk(ret_img)
            
    def print(self,_str):
        self.myGui.info_label.configure(text=_str)
        self.myGui.update()
            
    def run(self):
        try:
            self.print("waiting for Röbi...")
            #self.MCRobo.sock.recv(1)
            #self.MCRobo.sock.send("M")
            #self.MCRobo.sock.settimeout(2)
            #try:
            #    self.MCRobo.sock.recv(1024)
            #except:
            #    pass
            self.MCRobo.sock.settimeout(10)
            while True:
                #Take image
                res,img = self.cam.read()

                if res:
                    #resize to worksize
                    img = self.take_pic()
                    
                    #if analysing initialized
                    if self.myGui.take_shot:
                        #infom Jan for calc stared
                        self.MCRobo.sock.send("C")
                        #search all joints
                        original_image = img
                        img,centers,boxes = self.analyze(img)
                        self.show_image_in_tk(img)
                        self.myGui.tk_shot_btn.configure(text="Abbrechen",bg="red")
                        sleep(1)
                        #start Analysing session
                        self.MCRobo.sock.send("A")
                        results = self.analyze_joints(centers)
                        #show results on screen
                        self.draw_result(original_image,boxes,results)
                        #wait until manualy endig
                        while self.myGui.take_shot:
                            self.myGui.update()
                        self.MCRobo.sock.send("S")
                        self.print("back to Idle")
                        
                    else:
                        self.show_image_in_tk(img)
                else:
                    self.myGui.img_label.configure(text="CAM ERROR!!!!!!")
                self.print("idle...")                
                self.myGui.update()
        finally:
            self.cam.release()
            self.MCRobo.disconnect()

if __name__ == "__main__":
    app = Application()
    app.run()