import numpy as np
import cv2
from os import listdir




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

for file_name in listdir("Rawimages"):
    bsp_img = cv2.imread("Rawimages/"+file_name)
    bsp_img = cv2.resize(bsp_img,(1024,768))
    splitted_img = splitter((256,256),bsp_img)
    print(len(splitted_img))

    for j,col in enumerate(splitted_img):
        for i,img in enumerate(col):
            cv2.imshow("split img",img)
            
            wait = True
            while wait:
                key = cv2.waitKey(1) & 0xFF
                if key == ord("y"):
                    cv2.imwrite("ProcessedImages/img"+str(len(listdir("ProcessedImages")))+".png",img)
                    wait = False
                elif key == ord("b"):
                    wait = False
                    break

if key == ord("q"):
    cv2.destroyAllWindows()