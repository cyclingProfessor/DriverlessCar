import numpy as np
import cv2

rawImage = cv2.imread('rawImage.jpg')
cv2.imshow('Original Image',rawImage)
cv2.waitKey(0)

hsv = cv2.cvtColor(rawImage, cv2.COLOR_BGR2LAB)
cv2.imshow('HSV Image',hsv)
cv2.waitKey(0)

hue, saturation, value = cv2.split(hsv)
cv2.imshow('Saturation', hue)
cv2.waitKey(0)

retval, thresholded = cv2.threshold(hue, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
cv2.imshow('Saturation', thresholded)
cv2.waitKey(0)

medianFiltered = cv2.medianBlur(thresholded, 5)
cv2.imshow('Saturation', medianFiltered)
cv2.waitKey(0)
