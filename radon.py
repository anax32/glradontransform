import numpy as np
import sys

from skimage.io import imread, imsave
from skimage.transform import radon #, rescale

image = imread(sys.argv[1])

if len(image.shape) == 3:
  image = image.mean(axis=-1)

print("image.shape: %s" % str(image.shape))

theta = np.linspace(0., 180., max(image.shape), endpoint=False)
sinogram = radon(image, theta=theta, circle=True)
imsave(sys.argv[2], sinogram)
