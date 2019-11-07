from keras.models import Sequential
from keras.layers import BatchNormalization,Conv2D,MaxPool2D,Dropout,Flatten,Dense
from keras.applications.vgg16 import VGG16
from os import listdir
from cv2 import imread
from random import shuffle

class AiManager:
    def __init__(self,input_shape):
        self.model = self.create_Model(input_shape)
        self.input_shape = input_shape

    


    def create_Model(self,input_shape):
        model = Sequential()
        model.add(BatchNormalization(input_shape=(512,512,3)))
        model.add(Conv2D(32,(10,10),activation='relu'))
        model.add(Conv2D(16,(5,5),activation='relu'))
        model.add(MaxPool2D(pool_size=(2,2)))
        model.add(Dropout(0.5))
        model.add(Flatten())
        model.add(Dense(128,activation='relu'))
        model.add(Dense(21,activation='softmax'))
        model.compile(optimizer='adam',
                loss='categorical_crossentropy',
                metrics=['accuracy']
                )
        return model

    def train_model(self,path):
        data = []

        for img_name in listdir(path+"/0"):
            img = imread(path+"/0/"+img_name)
            data.append([img,[1,0]])
        
        for img_name in listdir(path+"/1"):
            img = imread(path+"/1/"+img_name)
            data.append([img,[0,1]])

        shuffle(data)
        X = []
        Y = []
        for x,y in data:
            X.append(x)
            Y.append(y)
        
        history = self.model.fit(x,y,epochs=3)



