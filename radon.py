"""
runs scikit radon transform against an image
see:

https://github.com/scikit-image/scikit-image/blob/master/skimage/transform/radon_transform.py
https://github.com/scikit-image/scikit-image/blob/master/skimage/transform/_warps.py
"""
import logging
import sys

import numpy as np

from skimage.io import imread, imsave
from skimage.transform import radon #, rescale

logger = logging.getLogger(__name__)
logger.addHandler(logging.StreamHandler(sys.stdout))
logger.setLevel(level=logging.DEBUG)


image = imread(sys.argv[1]) # read image
image = image.astype(np.float)/255.0 # make unit scale
image = -1.0 + (image*2.0) # make [-1,1] scale

# make greyscale
if len(image.shape) == 3:
  image = image.mean(axis=-1)

logger.info("image.shape,rng,type: %s [%f -> %f as %s]" %
      (str(image.shape), image.min(), image.max(), str(image.dtype)))

theta = np.linspace(0., 180., max(image.shape), endpoint=False)
sgram = radon(image, theta=theta, circle=False)

sgram -= sgram.min()
sgram /= sgram.max()

logger.info("sgram.shape,rng,type: %s [%f -> %f as %s]" %
      (str(sgram.shape), sgram.min(), sgram.max(), str(sgram.dtype)))

imsave(sys.argv[2], sgram)
