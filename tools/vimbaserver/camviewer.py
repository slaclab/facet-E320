################################################################################
# TCP/IP client for displaying a data stream of images
###############################################################################
#
# Connects to a streaming server (vimbaserv.cpp) and displays images.
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

from PyQt5.QtWidgets	import QApplication, QWidget, QStyle, QSizePolicy, QFrame, QLabel, QScrollArea, QDesktopWidget
from PyQt5.QtGui		import QColor, QPixmap, QImage
from PyQt5.QtCore		import Qt
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

DEFAULT_NETWORK_PORT 	= 42001
NETWORK_SERVER			= "e320pi.slac.stanford.edu"
NETWORK_SERVER			= "192.168.0.8"

SERVER_TIMEOUT			= 3 #seconds


# Initial window size
WINDOW_WIDTH	= 1280
WINDOW_HEIGHT	= 960


#https://www6.slac.stanford.edu/about/logo-resources
SLAC_color	= "#8c1515"

mainwindow		= None
boot_status		= True

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
	image		= np.zeros((WINDOW_HEIGHT,WINDOW_WIDTH))
	qimage 			= QImage(image, image.shape[1],
						image.shape[0], QImage.Format_Grayscale8)
	pixmap 			= QPixmap(qimage) 
	mainwindow.label.setPixmap(pixmap)

def socket_read(netsocket, length):
	tcpreadstart	= time.time()
	data			= b''
	while(True):	
		now			= time.time()
		if((now-tcpreadstart) > SERVER_TIMEOUT):
			raise ValueError("connection timed out")
		readlength	= length - len(data) 
		if(readlength == 0):
			break
		if(readlength < 0):
			print("error reading data")
			sys.exit()
		data 		+= netsocket.recv(readlength)
	return	data

def camviewerNetworkThread(mainwindow):
	global	network_port
	while(True):
		netsocket	= None
		if(testRunningCondition(mainwindow) == False):
			return
		showEmptyImage(mainwindow)

		try:
			netsocket		= socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			hostip			= socket.gethostbyname(NETWORK_SERVER)
			netsocket.connect((hostip, network_port))
			print ("network connection established: ", hostip)


			image_size		= None
			while(True):
				if(testRunningCondition(mainwindow) == False):
					netsocket.close()
					return
			
				loopcount	= 20
				starttime	= time.time()*1000.0 #ms
				for i in range(loopcount):
					#####
					# Read header
					header		= socket_read(netsocket, 12)
					width		= 0
					width		+= (header[0] << 0)
					width		+= (header[1] << 8)
					width		+= (header[2] << 16)
					width		+= (header[3] << 24)

					height		= 0
					height		+= (header[4] << 0)
					height		+= (header[5] << 8)
					height		+= (header[6] << 16)
					height		+= (header[7] << 24)

					length		= 0
					length		+= (header[8] << 0)
					length		+= (header[9] << 8)
					length		+= (header[10] << 16)
					length		+= (header[11] << 24)
					
					#print("image size: ", width, "\t", height, "\t", length)
					image_size	= width * height * length

					#####
					# Read one image
					imagebuffer	= socket_read(netsocket, image_size)
				
					#print(imagebuffer)
					#####
					# Convert image
					#  https://www.kernel.org/doc/html/v5.0/media/uapi/v4l/pixfmt-yuyv.html
					# The Cb/Cr components have only half the resolution. Therefore, we need to rescale.
					nparr			= np.frombuffer(imagebuffer, dtype=np.uint16)
					image			= np.multiply(nparr.reshape(height,width), int(65535/4095))

					#print(image)

					qimage 			= QImage(image, image.shape[1],
										image.shape[0], QImage.Format_Grayscale16)
					pixmap 			= QPixmap(qimage) 
					mainwindow.label.setPixmap(pixmap)
		
					#QApplication.style.pixelMetric(QStyle.PM_TitleBarHeight)
		
					total_height	= height + QStyle.PM_DefaultFrameWidth
					total_width		= width	+ QStyle.PM_DefaultFrameWidth

					global boot_status
					if(boot_status == True):
						boot_status	= False
						mainwindow.resize(total_width,total_height)
					mainwindow.setMaximumWidth(total_width)
					mainwindow.setMaximumHeight(total_height)





				stoptime	= time.time()*1000.0 #ms
				fps		= float(loopcount*1000)/float(stoptime-starttime)
				print("Data: ", fps*image_size*8/1e6 , "MBit/s; FPS: ", fps)


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

		#self.setMaximumWidth(PIXEL_WIDTH)
		#self.setMaximumHeight(PIXEL_HEIGHT)

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
		# no maximize
		self.setWindowFlags(self.windowFlags() | Qt.CustomizeWindowHint)
		self.setWindowFlags(self.windowFlags() & ~Qt.WindowMaximizeButtonHint)


		
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
		
	network_port	= DEFAULT_NETWORK_PORT

	#print(len(args), args)
	args = sys.argv
	if(len(args) == 2):
		network_port	= int(args[1])

	qt_main()

	
	
###############################################################################	
