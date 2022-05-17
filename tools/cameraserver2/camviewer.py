################################################################################
# TCP/IP client for displaying a data stream of images
###############################################################################
#
# Connects to a streaming server (camserv.cpp) and displays the images.
#
# GUI realized with PyQT5
# Python multithreading to realize the network interface
# Employs mutex to communicate safely between threads
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
import signal
import pickle
import numpy as np
import threading
import time
import socket

from PyQt5.QtWidgets	import QApplication, QWidget, QSizePolicy, QFrame, QLabel, QScrollArea, QDesktopWidget
from PyQt5.QtGui		import QColor, QPixmap, QImage

#
# There is an issue with using OpenCV (cv2) and PyQt simultaneously,
# as they are both using Qt and not the same library. Thus, we are
# trying to avoid that. OpenCV is now only used in the C++ server code.
#
#https://stackoverflow.com/questions/52337870/python-opencv-error-current-thread-is-not-the-objects-thread
#https://github.com/opencv/opencv-python/issues/386
#opencv-python-headless
#import	cv2
#from	cv2				import imread	as cv2imread
#from	cv2				import cvtColor	as cv2cvtColor
#from	cv2				import resize	as cv2resize
#from	cv2				import merge	as cv2merge

###############################################################################

NETWORK_PORT 	= 42000
NETWORK_SERVER	= "e320pi.slac.stanford.edu"
SERVER_TIMEOUT	= 3 #seconds

# Parameters of the image
PIXEL_WIDTH	= 1920
PIXEL_HEIGHT	= 1080

# Initial window size
WINDOW_WIDTH	= 1366
WINDOW_HEIGHT	= 768

#Image is an OpenCV BGR integer array (8-bit)
IMAGE_SIZE	= 3*PIXEL_WIDTH*PIXEL_HEIGHT

#https://www6.slac.stanford.edu/about/logo-resources
SLAC_color	= "#8c1515"

mainwindow	= None

###############################################################################
# Network thread
###############################################################################

def testRunningCondition(mainwindow):
	mainwindow.networkmutex.acquire()
	networkrun	= mainwindow.networkrun
	mainwindow.networkmutex.release()
	return		networkrun

def showEmptyImage(mainwindow):
	#####
	# Display empty image
	rgb_image		= np.zeros((PIXEL_HEIGHT,PIXEL_WIDTH,3))
	qimage 			= QImage(rgb_image, rgb_image.shape[1],
						rgb_image.shape[0], QImage.Format_RGB888)
	pixmap 			= QPixmap(qimage) 
	mainwindow.label.setPixmap(pixmap)


def camviewerNetworkThread(mainwindow):
	while(True):
		netsocket	= None
		if(testRunningCondition(mainwindow) == False):
			return
		showEmptyImage(mainwindow)

		try:
			netsocket		= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			hostip			= socket.gethostbyname(NETWORK_SERVER)
			netsocket.connect((hostip, NETWORK_PORT))
			print ("network connection established: ", hostip)

			while(True):
				if(testRunningCondition(mainwindow) == False):
					netsocket.close()
					return
			
				loopcount	= 20
				starttime	= time.time()*1000.0 #ms
				for i in range(loopcount):
					#####
					# Read one image
					data		= b''
					tcpreadstart	= time.time()
					while(True):	
						now			= time.time()
						if((now-tcpreadstart) > SERVER_TIMEOUT):
							raise ValueError("connection timed out")
						readlength	= IMAGE_SIZE - len(data) 
						if(readlength == 0):
							break
						if(readlength < 0):
							print("error reading data")
							sys.exit()
						data 		+= netsocket.recv(readlength)
					#####
					# Convert image
					#  https://www.kernel.org/doc/html/v5.0/media/uapi/v4l/pixfmt-yuyv.html
					# The Cb/Cr components have only half the resolution. Therefore, we need to rescale.
					nparr			= np.frombuffer(data, dtype=np.uint8)
					bgr_image		= nparr.reshape(PIXEL_HEIGHT,PIXEL_WIDTH,3)
					rgb_image		= bgr_image[:,:,::-1].copy()

					qimage 			= QImage(rgb_image, rgb_image.shape[1],
										rgb_image.shape[0], QImage.Format_RGB888)
					pixmap 			= QPixmap(qimage) 
					mainwindow.label.setPixmap(pixmap)



				stoptime	= time.time()*1000.0 #ms
				fps		= float(loopcount*1000)/float(stoptime-starttime)
				print("Data: ", fps*IMAGE_SIZE*8/1e6 , "MBit/s; FPS: ", fps)


		except Exception as e:
			print ("exception occured: ")
			print(e)
			showEmptyImage(mainwindow)
			time.sleep(3)




###############################################################################
# Main window
###############################################################################


class MainWindow(QWidget):
	"""
	
	"""
	def __init__(self, *args, **kwargs):
		super(QWidget, self).__init__(*args, **kwargs)



		self.scroll 		= QScrollArea(self)
		self.scroll.move(0,0)

		self.setMaximumWidth(PIXEL_WIDTH)
		self.setMaximumHeight(PIXEL_HEIGHT)

		self.label 			= QLabel(self)
		self.label.move(0,0)

		self.scroll.setWidget(self.label)
		self.scroll.setWidgetResizable(True)

		self.networkmutex	= threading.Lock()
		self.networkrun		= True
		self.networkthread	= threading.Thread(name = 'camviewerNetworkThread', target = camviewerNetworkThread, args = (self,))

		self.networkthread.start()

		#####
		# Set window size:
		# int x, int y, int w, int h
		# 1920 x 1080 pixels (16:9)
		# 1366 x 768
		self.setGeometry(0,0,WINDOW_WIDTH,WINDOW_HEIGHT)

		
		#####
		# Center window on Desktop
		framerect 		= self.frameGeometry()
		desktopcenter	= QDesktopWidget().availableGeometry().center()
		framerect.moveCenter(desktopcenter)
		self.move(framerect.topLeft())
		#print(desktopcenter,framerect, framerect.topLeft())
		
	
	def resizeEvent(self, event):
		size	= event.size()
		width	= size.width()
		height	= size.height()
		#print("resize: ", width, height)
		self.scroll.resize(width,height)

	def closeEvent(self, event):
		print("close window")
		sys.stdout.flush()
		self.networkmutex.acquire()
		self.networkrun	= False	
		self.networkmutex.release()
		self.networkthread.join()
		event.accept()


		

def qt_main():
	global	mainwindow
	app		= QApplication([])
	mainwindow	= MainWindow()
	mainwindow.setWindowTitle("Camera lifestream viewer")

	mainwindow.show()
	app.exec_()
	sys.exit()
	
if __name__ == '__main__':
	# Make sure CTRL+C is able to stop the program
	#https://stackoverflow.com/questions/5160577/ctrl-c-doesnt-work-with-pyqt
	signal.signal(signal.SIGINT, signal.SIG_DFL)
	qt_main()
	
	
###############################################################################	
