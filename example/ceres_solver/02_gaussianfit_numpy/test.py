###############################################################################
#
# Fits a 2D rotated Gaussian to an image.
# Two methods are compared: 
#	a) ceres solver (via c++ numpy extension)
#	b) scipy solver
#
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
# for more details.
# 
# You should have received a copy of the GNU General Public License along
# with this program. If not, see <https://www.gnu.org/licenses/>.
# 
# Copyright Sebastian Meuren, 2022
# 
###############################################################################

import sys
sys.path.append('libary path')


import twodimgaussianfit as gfit
import numpy as np
from numpy import linalg as la
from scipy.optimize import curve_fit

###############################################################################
# Matplotlib
import matplotlib as mpl
mpl.use('Qt5Agg')
from matplotlib import rc
from matplotlib import ticker
import matplotlib.pyplot as plt
import matplotlib.colors as colors

matlab_yellow = '#edb120'
matlab_blue   = '#0072bd'

import cv2

###############################################################################
# IMPORTANT: numpy/matplotlib index convention
# y-axis: left index
# x-axis: right index
###############################################################################


ZOOM_LENGTH			= 30

FIT_XNOT_START		= ZOOM_LENGTH		# We fit after zooming into the area around the spot
FIT_YNOT_START		= ZOOM_LENGTH		# 
FIT_SIGMAONE_START	= 10.0				# in units of pixels
FIT_SIGMATWO_START	= 10.0				# in units of pixels
FIT_PHI_START		= 0.0				# angle (rad)
FIT_OFFSET_START	= -1.0				# 0...1
FIT_AMPLITUDE_START	= 5.0				# 0...1	


###############################################################################
# Read image from file
###############################################################################
print("read image data")
rawimage 	= cv2.imread('image.tiff', cv2.IMREAD_UNCHANGED)
background 	= cv2.imread('bg.tiff', cv2.IMREAD_UNCHANGED)

### Median filter
#print("median filter")
#rawimage = ndimage.median_filter(rawimage, size=2)


floatimage				= np.empty(rawimage.shape, dtype=float)
maxvalue_theory			= float(int("FFF0",16))
print("maximum value: ", maxvalue_theory)

###############################################################################
# Background subtraction
################################################################################
print("subtract background")
maxvalue			= float(floatimage[0,0])/float(maxvalue_theory) 
minvalue			= float(floatimage[0,0])/float(maxvalue_theory) 
maxindex			= (0,0)
minindex			= (0,0)
for j in range(floatimage.shape[0]):		#y-index
	for i in range(floatimage.shape[1]):	#x-index
		pixelvalue		= float(rawimage[j,i])/float(maxvalue_theory) - float(background[j,i])/float(maxvalue_theory)
		floatimage[j,i]	= pixelvalue
		if(pixelvalue > 1.0):
			raise ValueError("pixelvalue exceeding 1.0")
		if(pixelvalue > maxvalue):
			maxvalue	= pixelvalue
			maxindex	= (j,i)
		if(pixelvalue < minvalue):
			minvalue	= pixelvalue
			minindex	= (j,i)
			
shape		= floatimage.shape
print(shape)
print("1: ",shape[1])
print("0: ",shape[0])
print(floatimage.dtype)

print("min: ", minvalue, '\t', minindex)
print("max: ", maxvalue, '\t', maxindex)



###############################################################################
# Normalize image, get zoom region
###############################################################################
floatimage	= floatimage/maxvalue

print("get zoomed area around focal spot...")
# For debugging purposes
xoffset					= 0
yoffset					= 0
cropimage	= floatimage[
				maxindex[0]-ZOOM_LENGTH+yoffset:
				maxindex[0]+ZOOM_LENGTH+yoffset+1,
				maxindex[1]-ZOOM_LENGTH+xoffset:
				maxindex[1]+ZOOM_LENGTH+xoffset+1]


################################################################################
# Show image
################################################################################
print("plot image")
fig 	= plt.figure()
sp		= fig.add_subplot(111)
norm 	= colors.Normalize(vmin=0.0,vmax=1.0)
image	= sp.imshow(cropimage, cmap="hot", norm=norm, interpolation=None, alpha=0.9, origin='lower') # norm=LogNorm(vmin=1e-2,vmax=1.0)


#print(zgrid_data)
#fig.colorbar(image)
cbar	= fig.colorbar(image,format=ticker.FormatStrFormatter('%.1f'))
cbar.set_ticks([0.1, 0.3, 0.5, 0.7, 0.9])


plt.show()

###############################################################################
# Fit using ceres solver via c++ extension
##############################################################################
# Starting values for the parameters
param 		= np.zeros(7)
param[0]	= FIT_XNOT_START
param[1]	= FIT_YNOT_START
param[2]	= FIT_SIGMAONE_START
param[3]	= FIT_SIGMATWO_START
param[4]	= FIT_PHI_START
param[5]	= FIT_OFFSET_START
param[6]	= FIT_AMPLITUDE_START

print("max: ", np.amax(cropimage))

gfit.fit(cropimage,param)
print(param)


###############################################################################
# Rotated 2D Gaussian
###############################################################################
def	twodim_Gaussian(x,y,xnot,ynot,sigmaone,sigmatwo,phi,offset,amplitude):
	"""
	Actual function
	"""
	# shift coordinates  by (xnot, ynot)
	# rotate coordinates by phi
	xprime	= np.cos(phi)*(x-xnot) - np.sin(phi)*(y-ynot)
	yprime	= np.sin(phi)*(x-xnot) + np.cos(phi)*(y-ynot)
	# Gaussian with sigmaone and sigmatwo
	res		= amplitude*np.exp(-xprime**2/(2.0*sigmaone**2)-yprime**2/(2.0*sigmatwo**2))
	# Add offset
	return	res + offset

###############################################################################
# Fit Gaussian
###############################################################################
# We are using a 1D fitting function for 2D data. To this end we use the
# "ravel" function, which results in a continous, fattened 1D array
# https://numpy.org/doc/stable/reference/generated/numpy.ravel.html
print("fit Gaussian")

cropimage1D	= cropimage.ravel()
gaussian1D	= np.empty_like(cropimage1D)

def	fitGaussian(dummy,xnot,ynot,sigmaone,sigmatwo,phi,offset,amplitude):
	"""
	Fitfunction. Note that we should integrate the Gaussian over the pixel
	size instead of evaluating it at the pixel center. However, that would	
	take much more time. Therefore, we assume that the Gaussian is smooth
	over the size of a single pixel.
	"""
	global cropimage1D
	global gaussian1D
	for i in range(len(cropimage1D)):
		# get the corresponding indices
		y,x	=	np.unravel_index(i,cropimage.shape)
		
		res = twodim_Gaussian(x,y,xnot,ynot,sigmaone,sigmatwo,phi,offset,amplitude)
		
		gaussian1D[i]	= res
	return	gaussian1D

#####
# First guess for the fit
p0	= [
	FIT_XNOT_START,
	FIT_YNOT_START,
	FIT_SIGMAONE_START,
	FIT_SIGMATWO_START,
	FIT_PHI_START,
	FIT_OFFSET_START,
	FIT_AMPLITUDE_START]

# https://docs.scipy.org/doc/scipy/reference/generated/scipy.optimize.curve_fit.html
# Note that we are actually not using the xdata, instead we are reading the image from a global variable
popt, pcov 	= curve_fit(fitGaussian, np.zeros_like(cropimage1D), cropimage1D, p0=p0)		
print("fit values: ")
for i in range(len(popt)):
	print(popt[i],np.sqrt(pcov[i,i]))
	
###############################################################################
