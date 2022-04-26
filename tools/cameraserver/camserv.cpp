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
#define	USB_CAM_ID	"0c45:6366"
#define	USB_CAM_DEVICE	"/dev/video0"
#define	PIXEL_WIDTH	1920
#define	PIXEL_HEIGHT	1080
#define	BUFFER_COUNT	4

/*
   Server parameters
 */
#define SERVER_PORT			42000
#define SERVER_SEND_TIMEOUT		10	//timeout in seconds

#define TCP_BUFFER_SIZE		1024*128

#define IMAGE_HORZ			PIXEL_WIDTH
#define IMAGE_VERT			PIXEL_HEIGHT

#define TIME_STRING_BUFFER_SIZE	32
#define CRC_STRING_BUFFER_SIZE		8


/*
   Stores information about a camera buffer
 */
struct CAM_DATA_BUFFER_STRUCT{
	void*  	start;
	size_t		length;
};




int main(int argc, char const *argv[])
{
	using namespace std;
	using namespace std::chrono;
	using namespace cv;

	printf("Welcome. I hope you are having a great day.\n\n");
	printf("\e[?25l");	//hide cursor
	fflush(stdout);


	// loop variables
	unsigned int 			i;
	unsigned int 			j;
	unsigned int 			k;

	int				server_fd;
	int				new_socket;
	struct timeval 		tv;

	struct v4l2_buffer 		buf;
	CAM_DATA_BUFFER_STRUCT* 	buffers;
	int 				fd;
	struct v4l2_fmtdesc 		fmtdesc;	
	
	buffers = (CAM_DATA_BUFFER_STRUCT*) calloc(BUFFER_COUNT, sizeof(*buffers));
	if(buffers == NULL){
		perror("memory request failed");
		return -1;
	}
	for (i = 0; i < BUFFER_COUNT; i++) {
		buffers[i].length	= 0;
		buffers[i].start	= NULL;
	}

	/*
	  Main loop: whenever something goes wrong, we start over	
	 */
	while(true){
		auto	now		= chrono::system_clock::now();
		time_t	now_time	= chrono::system_clock::to_time_t(now);
	
		printf("--- new session started ---\n");
		cout	<<  ctime(&now_time) << endl;
		/*
		  Initialize everything properly
		  */
		fd		= 0;
		server_fd	= 0;
		new_socket	= 0;
			
		try{

			/*************************************************************
				Reset the webcam via libusb and find the right device
				file
			*/
			/*
			  Reset the USB camera; libusb:
			  https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
			 */
			libusb_device**	usb_dev_list;		//libusb device list
			libusb_device*		usb_device;		//libusb device 	
			libusb_device_handle*	cam_device;		//camera (if found)
			libusb_context*	usb_ctx 	= NULL; //libusb session information	
			if(libusb_init(&usb_ctx) < 0){
				perror("libusb init error");
				throw -1;
			}

			int devcount	= libusb_get_device_list(NULL, &usb_dev_list);
			if(devcount < 0){
				perror("libusb get device list error");
				throw -1;
			}
			/* No error handling from now on, in order to properly free/exit libusb.
			   This s be improved in the future.
			 */
			printf("Number of usb devices (vendor:device): %i\n", devcount);
			libusb_device_descriptor usb_dev_desc	= {0};
			char dev_id_string [64];
			
			cam_device	= NULL; 
			
			for (i = 0; i < devcount; i++){
				usb_device	= usb_dev_list[i];
				if(libusb_get_device_descriptor(usb_device, &usb_dev_desc) < 0){
					perror("libusb get device descriptor error");
					throw -1;
				}
				sprintf (dev_id_string, "%04x:%04x", usb_dev_desc.idVendor, usb_dev_desc.idProduct);
				printf("\t%s", dev_id_string);
				if(strcmp(dev_id_string, USB_CAM_ID) == 0){
					printf(" camera found\n");
					cam_device	= libusb_open_device_with_vid_pid(usb_ctx, usb_dev_desc.idVendor, usb_dev_desc.idProduct);
				}else{
					printf("\n");
				}
			}
			if(cam_device != NULL){
				libusb_reset_device(cam_device);
				libusb_release_interface(cam_device, 0);
				printf("Camera reset.\n");			
				sleep(1);
				
			}
	
			libusb_free_device_list(usb_dev_list, 1);
			libusb_exit(usb_ctx);
			
			 //libusb_reset_device	
			
			/*************************************************************
				Setup the webcam readout via v4l
			*/
			//printf("Number of buffers: %i\n", BUFFER_COUNT);
			/*
			  Get a file descriptor for the web cam	
			 */
			fd = v4l2_open(USB_CAM_DEVICE, O_RDWR);
			if(fd < 0){
				perror("Failed to open the web cam");
				throw -1;
			}
			/*
			  List the available web-cam formats	
			 */
			// v4l2-ctl --list-formats-ext
			// 1920 x 1080
			memset(&fmtdesc,0,sizeof(fmtdesc));
			fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			while (ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) == 0)
			{    
				printf("%s\n", fmtdesc.description);
				//printf("%i\n", fmtdesc.pixelformat);
				fmtdesc.index++;
			}
			/*
			  Set the image format	
			 */	
			v4l2_format imageFormat;
			imageFormat.type 			= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			imageFormat.fmt.pix.width  		= PIXEL_WIDTH;
			imageFormat.fmt.pix.height 		= PIXEL_HEIGHT;
			imageFormat.fmt.pix.pixelformat 	= V4L2_PIX_FMT_YUYV;  // V4L2_PIX_FMT_MJPEG;
			imageFormat.fmt.pix.field 		= V4L2_FIELD_NONE;
			// tell the device you are using this format
			if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
				perror("Setting the image format failed");
				throw -1;
			}else{
				//printf("image format set\n");
				//fflush(stdout);

			}
			
			/*
			  Request the buffers for the image	
			 */	
			struct v4l2_requestbuffers req = {0};
			req.count	= BUFFER_COUNT;
			req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
			req.memory	= V4L2_MEMORY_MMAP;
			if(ioctl(fd, VIDIOC_REQBUFS, &req) < 0){
				perror("Requesting image buffers failed");
				throw -1;
			}else{
				//printf("image buffers requested\n");
				//fflush(stdout);
			}
				
			
			/*
			  Map the buffers:
			  we store the information for every buffer in "buffers", but we use
			  only one "buf" structure for passing the information to v4l
			 */

			for (i = 0; i < BUFFER_COUNT; i++) {
				memset(&buf,0,sizeof(buf));
				buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory      = V4L2_MEMORY_MMAP;
				buf.index       = i;

				if(ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0){
					perror("VIDIOC_QUERYBUF failed");
					throw -1;
				}
				buffers[i].length = buf.length;
				//printf("Buffer length: %i\n",buffers[i].length);
				buffers[i].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		    
				if (buffers[i].start == MAP_FAILED) {
					perror("Can not map this buffer.");
					throw -1;
				}
			}	
			//printf("Image buffer queried\n");
			//fflush(stdout);

			/*
			  Put the buffers into the streaming queue	
			 */	
			for (i = 0; i < req.count; i++) {
				memset(&buf,0,sizeof(buf));
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;
				if(ioctl(fd, VIDIOC_QBUF, &buf) < 0){
					perror("Buffer queuing failed");
					throw -1;
				}
			}
			/*
			  Activate streaming: the kernel puts images into the buffer	
			 */		
			enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
				perror("Starting webcam streaming failed");
				throw -1;
			}

			
			/*************************************************************
				Setting up TCP server
			*/


			/*
			  Declare variables for the TCP server
			 */

			uint32_t	rand_seed	=  0;
			uint32_t	rand_num	=  0;

			int		valread;
			struct		sockaddr_in address;
			int 		opt 		= 1;
			int 		addrlen = sizeof(address);
			uint8_t 	in_buffer[TCP_BUFFER_SIZE] 		= {0};
			uint8_t 	out_buffer[TCP_BUFFER_SIZE] 		= {0};
			uint8_t 	time_buffer[TIME_STRING_BUFFER_SIZE] 	= {0};
			uint8_t 	crc_buffer[CRC_STRING_BUFFER_SIZE] 	= {0};
			string  	time_string;

			//printf("create socket\n");
			//fflush(stdout);
			server_fd = socket(AF_INET, SOCK_STREAM, 0);
			if (server_fd == 0){
				perror("creating socket failed");
				throw -1;
			}
			if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
				perror("attaching socket to port failed");
				throw -1;
			}
			
			address.sin_family		= AF_INET;
			address.sin_addr.s_addr	= INADDR_ANY;
			address.sin_port 		= htons(SERVER_PORT);

			//printf("binding socket\n");
			//fflush(stdout);
			if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
				perror("binding socket failed");
				throw -1;
			}
			if (listen(server_fd, 10) < 0){
				perror("listening on port failed");
				throw -1;
			}
			printf("listening on port: %i\n", SERVER_PORT);
			fflush(stdout);


			//printf("accepting\n");
			//fflush(stdout);
			while(true){
				new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
				if(new_socket == -1){
					printf("Error while accepting new connection");
					continue;
				}
				/*************************************************************
				  Set socket timeout
				*/					
				
				tv.tv_sec	= SERVER_SEND_TIMEOUT;
				tv.tv_usec	= 0;
				setsockopt(new_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
					
					
					
				printf("start sending data...\n\n\n");
				while(true){
					/*************************************************************
						Data transmission loop
					 */

					auto millisec_since_epoch_start = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
					for (i = 0; i < req.count; i++){
						//printf("loop: %i,%i\n", i, j);
						//std::cout << std::flush;
						memset(&buf,0,sizeof(buf));
						buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
						buf.memory = V4L2_MEMORY_MMAP;
						buf.index = i;


						/*
						   Wait until data are ready using the select statement
						 */

						fd_set fds;
						FD_ZERO(&fds);
						FD_SET(fd, &fds);
						/*
						FD_ZERO()
						      This macro clears (removes all file descriptors from) set.
						      It should be employed as the first step in initializing a
						      file descriptor set.

						FD_SET()
						      This macro adds the file descriptor fd to set.  Adding a
						      file descriptor that is already present in the set is a
						      no-op, and does not produce an error.
						*/

						// Set the timeout value
						tv.tv_sec 	= 5;
						tv.tv_usec	= 0;
						/*
						  Select: https://man7.org/linux/man-pages/man2/select.2.html
						  
						  int pselect(int nfds, fd_set *restrict readfds,
							fd_set *restrict writefds, fd_set *restrict exceptfds,
							const struct timespec *restrict timeout,
							const sigset_t *restrict sigmask);
						  
						  readfds
						      The file descriptors in this set are watched to see if
						      they are ready for reading.  A file descriptor is ready
						      for reading if a read operation will not block; in
						      particular, a file descriptor is also ready on end-of-
						      file.

						      After select() has returned, readfds will be cleared of
						      all file descriptors except for those that are ready for
						      reading.
						  
						  
						  nfds	This argument should be set to the highest-numbered file
							descriptor in any of the three sets, plus 1.
						  
						  
						 */
						if(select(fd+1, &fds, NULL, NULL, &tv) == -1){
						    perror("Error while waiting for file descriptor to get ready for read");
						    throw -1;
						}

						/*********************************************************************
						 Dequeue the buffer
						*/	 
						if(ioctl(fd, VIDIOC_DQBUF, &buf) < 0){
						    perror("Dequeuing buffer failed");
						    throw -1;
						}
						
						/*
						 Convert the image
						*/	
						

						cv::Mat cvimage = cv::Mat(PIXEL_HEIGHT, PIXEL_WIDTH, CV_8UC2, buffers[i].start);
						cv::Mat cvimgBGR;
						cv::cvtColor(cvimage, cvimgBGR, cv::COLOR_YUV2BGR_YUY2);
						//cv::imwrite("test.png", cvimgBGR); 
						//printf("buffer size: %li %li", cvimgBGR.total(), cvimgBGR.elemSize());
						//std::cout << std::flush;
						if(cvimgBGR.total() != (PIXEL_HEIGHT * PIXEL_WIDTH)){
							perror("Picture size wrong");
							throw -1;							
						}
						if(cvimgBGR.elemSize() != 3){
							perror("Not a color picture");
							throw -1;							
						}
						ssize_t total_length	= (cvimgBGR.total() * cvimgBGR.elemSize());
						ssize_t sent_length	= send(new_socket, cvimgBGR.data, total_length, 0);
						if(sent_length != total_length){
							perror("Socket send error");
							throw -1;
						}

						/*
						 Queue the buffer again
						*/	
						memset(&buf,0,sizeof(buf));
						buf.type 	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
						buf.memory 	= V4L2_MEMORY_MMAP;
						buf.index 	= i;
						if(ioctl(fd, VIDIOC_QBUF, &buf) < 0){
							perror("Buffer queuing failed");
							throw -1;
						}

					}
					auto millisec_since_epoch_stop  = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
					// 8 bit per byte, 3 byte per pixel, 1000 ms per second, 1000*1000 bit per Mbit
					double	FPS		= double(BUFFER_COUNT*1000)/double(millisec_since_epoch_stop - millisec_since_epoch_start);
					double	data_rate	= FPS*double(8*3*PIXEL_WIDTH*PIXEL_HEIGHT)/(1000.0*1000.0);

					printf("\e[?25l");	//hide cursor
					printf("\033[F");	//move one line up
					printf("\33[2K");	//erase line

					now			= chrono::system_clock::now();
					now_time		= chrono::system_clock::to_time_t(now);
				
					printf("%s data rate: %f MBit/s;   FPS: %f\n", strtok(ctime(&now_time),"\n"), data_rate, FPS);
					//std::cout << "Parameters: " << BUFFER_COUNT << " " << PIXEL_WIDTH << " " << PIXEL_HEIGHT << " " << float(millisec_since_epoch_stop - millisec_since_epoch_start) << endl;
					std::cout << std::flush;


				}
			}
		}//end try
		catch (...){
			/*
			  Clean up.
			  */
			v4l2_close(fd);
			close(server_fd);
			close(new_socket);		

			if(buffers != NULL){
				for (i = 0; i < BUFFER_COUNT; i++) {
					v4l2_munmap(buffers[i].start, buffers[i].length);
				}

			}

			fflush(stdout);
			fflush(stderr);

			sleep(3);
		
			printf("restarting the camera server\n\n");
		}
	}//end while
	return 0;
}
