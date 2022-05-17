/*
 Currently needs to run as root to reset the camera.

 Finds and resets the camera via libusb.
 Captures images from the camera via v4l.
 Converts YUV2 -> BGR using opencv
 Runs a simple TCP server to stream the images.
 
 Context: we would like to stream uncompressed images from a raspberry 
 pi in the FACET tunnel to a control computer in the SLAC/FACET network 
 or even outside. The FACET control computers can be reached via an 
 ssh port tunnel.
 
 Compile with g++ camserv.cpp -I/usr/include/opencv4/ -lusb-1.0 -lv4l2 -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -o camserver -o camserver
 
 This program is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published by the
 Free Software Foundation, either version 3 of the License, or (at your
 option) any later version.
 
 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program. If not, see <https://www.gnu.org/licenses/>.
 
 Copyright Sebastian Meuren, 2022 
 
 */
 

 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <string>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <thread>
#include <mutex>
#include <boost/crc.hpp>
#include <iterator>
#include <list>

#include <libusb-1.0/libusb.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/usbdevice_fs.h>

#include <linux/videodev2.h>
#include <libv4l2.h>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


/*
   Camera parameters
 */
#define	USB_CAM_ID		"0c45:6366"
#define	USB_CAM_DEVICE	"/dev/video0"
#define	PIXEL_WIDTH		1920
#define	PIXEL_HEIGHT	1080
#define	BUFFER_COUNT	4

/*
   Server parameters
 */
#define SERVER_PORT				42000
#define SERVER_SEND_TIMEOUT		10	//timeout in seconds

#define TCP_BUFFER_SIZE			1024*128

#define IMAGE_HORZ				PIXEL_WIDTH
#define IMAGE_VERT				PIXEL_HEIGHT

#define TIME_STRING_BUFFER_SIZE	32
#define CRC_STRING_BUFFER_SIZE		8


/*
   Stores information about a camera buffer
 */
struct CAM_DATA_BUFFER_STRUCT{
	void*  		start;
	size_t		length;
};




int main(int argc, char const *argv[])
{
	using namespace std;
	using namespace std::chrono;
	using namespace cv;

	int	network_port		= SERVER_PORT;

	cout << "Welcome. I hope you are having a great day." << endl << endl;
	fflush(stdout);

	if(argc == 2){
		cout << "Network port selected: " << arg[1] << endl;
	}

	return 0;
}


