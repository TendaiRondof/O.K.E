import numpy as np
import cv2
from os import listdir
from tqdm import tqdm

static_pic_shape = (1024,768,3)
static_relation = (4,3)
input_image_shape = (256,256,3)

images = []
for img_nr, image_path in tqdm(enumerate(listdir("RawImages")),total=len(listdir("RawImages"))):
    img = cv2.imread("RawImages/"+image_path)
    img = cv2.resize(img,static_pic_shape[:2])

    for img_row_nr in range(static_relation[0]):
        for image_column_nr in range(static_relation[1]):
            save_img = img[img_row_nr*input_image_shape[0]:img_row_nr*input_image_shape[0]+input_image_shape[0],image_column_nr*input_image_shape[1]:image_column_nr*input_image_shape[1]+input_image_shape[1],:]
            cv2.imwrite("ProcessedImages/{}{}{}.png".format(img_nr,img_row_nr,image_column_nr),save_img)
