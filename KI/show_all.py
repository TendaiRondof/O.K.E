import cv2
from os import listdir

dir_name = "Data_set/Images"
for image in listdir(dir_name):
    img = cv2.imread(dir_name+"/"+image)
    print(img.shape)
    cv2.imshow("Fisch",img)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break