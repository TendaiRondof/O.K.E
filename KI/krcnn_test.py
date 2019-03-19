from keras_rcnn import models,backend
import keras
import numpy

categories = ["Joint"]
image_shape = (418,418,3)

model = models.RCNN(image_shape,categories)