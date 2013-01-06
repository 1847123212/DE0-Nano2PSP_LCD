#!/usr/bin/env python

import sys

import numpy as np
import matplotlib.image as mpimg
import matplotlib.pylab as pl
import scipy.misc as misc

screen_res = (272, 480)

def load_image(img_path):
    while True:
        try:
            img = mpimg.imread(img_path)
            print "Loaded image: {0}".format(img_path)
            break
        except IOError:
            print "Could not locate file: ", img_path
            img_path=raw_input('Please enter a new path or empty to exit program: ')
        
        if img_path == '':
            print "Program terminated"
            sys.exit()
            break
    return img

if __name__=="__main__":
    try:
        img_path = str(sys.argv[1])
    except:
        img_path=raw_input('Please enter a image path or empty to exit program: ')
        
    if img_path == '':
        print "Program terminated"
        sys.exit()
              
    img = load_image(img_path)
    org_img = img.copy()
    
    # Convert to [ 0x00 , R, G, B]
    if screen_res == img.shape[0:2]:
        if img.dtype != 'uint8':
            img = img * 255
            
        img = img.astype('uint32')
        new_img = (img[:,:,0]<<16) | (img[:,:,1]<<8) | img[:,:,2]
    else:
        #We need to convert the resolution to fit the screen
        x_ratio = img.shape[0]/float(screen_res[0])
        y_ratio = img.shape[1]/float(screen_res[1])
        
        if x_ratio > y_ratio:
            new_size = tuple((np.array(img.shape[0:2])/x_ratio).astype('int'))
        else:
            new_size = tuple((np.array(img.shape[0:2])/y_ratio).astype('int'))
        print new_size
        
        conv_img = misc.imresize(img, new_size)
        conv_img = conv_img.astype('uint32')
        new_img = (conv_img[:,:,0]<<16) | (conv_img[:,:,1]<<8) | conv_img[:,:,2]

    # jpeg images needs to be fliped
    if img_path.split('.')[-1] == ("jpg" or "JPG" or "JPEG" or "jpeg"):
        new_img = np.flipud(new_img)
        
    # Save result
    try:
        img_save_path = sys.argv[2]
    except IndexError:
        img_save_path=raw_input('Where to save the image?: ')
    
    new_img.tofile(img_save_path)
    print "Saved image to: ", img_save_path
