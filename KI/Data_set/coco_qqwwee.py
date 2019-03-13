import json
import os
import requests
import cv2
import numpy as np
import matplotlib.pyplot as plt 
import re
from tqdm import tqdm

filename = "Data_set/coco_data.json"


with open(filename, 'r') as f:
    full_json_file = json.load(f)
    all_images = full_json_file["images"]
    all_anotations = full_json_file["annotations"]

    data_dic = {}

    for image in all_images:
        img_as_bytes = requests.get(image["file_name"], stream=True).content
        with open("Data_set/Images/"+image["id"]+".jpg", 'wb') as handler:
            handler.write(img_as_bytes)

        img_path = "Data_set/Images/"+image["id"]+".jpg"
        data_dic[image["id"]] = [img_path]

    for annotation in all_anotations:
        data_dic[annotation["image_id"]].append(annotation["bbox"])

    with open("train.txt","w") as file:
        string = ""
        for data in data_dic:
            for element in data_dic[data]:
                if isinstance(element[0],float) :
                    save_array = [int(val) for val in element]+[0]
                else:
                    save_array = element
                string+= re.sub(r"[\[\] ]",'',str(save_array))+' '
            string+="\n"
        
        file.write(string)
'''
filename = "Data_set/face_detection.json"
with open(filename, 'r') as f:
    full_json_file = json.load(f)
    all_data = full_json_file["contents"]

    writing_string = ""

    for img_nr,content in tqdm(enumerate(all_data),total=len(all_data)):
        try:
            img_as_bytes = requests.get(content["content"], stream=True).content
            img_path = "Data_set/Images/"+str(img_nr)+".jpg"
            with open(img_path, 'wb') as handler:
                handler.write(img_as_bytes)
            
            boxes_as_string = img_path
            for box in content["annotation"]:
                width = box["imageWidth"]
                height = box["imageHeight"]

                x1 = int(box["points"][0]["x"]*width)
                y1 = int(box["points"][0]["y"]*height)
                x2 = int(box["points"][1]["x"]*width)
                y2 = int(box["points"][1]["y"]*height)

                boxes_as_string+=" {},{},{},{},0".format(x1,y1,x2,y2)
            
            boxes_as_string+="\n"
            writing_string+=boxes_as_string
        except:
            print("Image nr {} skipped!!".format(img_nr))

with open("train.txt","w") as file:
    file.write(writing_string)
'''

        



    

