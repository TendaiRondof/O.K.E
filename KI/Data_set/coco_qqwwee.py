import json
import os
import requests
import cv2
import numpy as np

filename = "Data_set/coco_data.json"

with open(filename, 'r') as f:
    full_json_file = json.load(f)
    all_images = full_json_file["images"]
    all_anotations = full_json_file["annotations"]

    data_array = {}

    for image in all_images:
        img_as_bytes = requests.get(image["file_name"], stream=True).content

        img = np.frombuffer(img_as_bytes,dtype=np.uint8)
        print(img[:10])
        img = img.reshape((image["height"],image["width"],3))
        cv2.imshow("bla",img)
        data_array[image["id"]] = [img]

    for annotation in all_anotations:
        data_array[annotation["image_id"]].append(annotation["bbox"])

