import json
import os
import requests
import cv2
import numpy as np
import matplotlib.pyplot as plt 
import re

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
                    save_array = [int(val) for val in element]+[1]
                else:
                    save_array = element
                string+= re.sub(r"[\[\] ]",'',str(save_array))+' '
            string+="\n"
        
        file.write(string)
        


    

