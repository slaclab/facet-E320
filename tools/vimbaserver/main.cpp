/********************************************************************************
 * Read data from a AVT camera via the Vimba SDK
 *
 *	- resets AVT devices via libusb-1.0
 *	- uses the vimba SDK to communicate with AVT cameras:
 *		- classes implemented in vimba.h
 *		- resets the cameras
 *		- sets communication speed
 *		- uses asynchronous capturing for data transfer
 *	- uses a TCP/IP server to stream the frames
 *		- based on classes defined in server.h
 *
 *
 *	- future: uses seperate server for control commands		
 * 
 * Compile:
 * g++ main.cpp -I /home/facet/Vimba_6_0/ 
 *				-I /home/facet/Vimba_6_0/VimbaCPP/Examples/
 *              -L /home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/
 *              -lVimbaCPP -lusb-1.0 -pthread
 *
 * Library access:
 * export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:
 *        /home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/
 * 
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
 * for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright Sebastian Meuren, 2022
 *
 ********************************************************************************/ 
#include <unistd.h>

#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <queue>

// string
#include "string.h"

// time
#include <chrono>
#include <ctime>

// multi-threading
#include <thread>
#include <mutex>

// usb
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/usbdevice_fs.h>

// vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"
 
 
using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;

/*****************************************************************************/
// Class with global definitions
/*****************************************************************************/

#include "global.h"
// only one object, ever.
CGlobal	global;


/******************************************************************************
 * Image class
 *****************************************************************************/
class CByteImage{
	public:
		CByteImage(uint32_t width, uint32_t height, uint32_t bytesperpixel);
		~CByteImage();

		uint32_t		width;
		uint32_t		height;
		uint32_t		bytesperpixel;
		
		uint8_t*		data;
		size_t			data_length;
};

/*********************
 * Constructor
 *********************/
CByteImage::CByteImage(uint32_t width, uint32_t height, uint32_t bytesperpixel){
	this->width				= width;
	this->height			= height;
	this->bytesperpixel		= bytesperpixel;
	this->data_length		= width*height*bytesperpixel;
	data					= new uint8_t[data_length];
}

/*********************
 * Destructor
 *********************/
CByteImage::~CByteImage(){
	delete	data;
}


/******************************************************************************
 * Data streaming server
 *****************************************************************************/
#define	CAM_SERVER_DEFAULT_QUEUE_LENGTH	3

class CCamServer : public CDataServer{
	public:
		CCamServer(int port, const string name, CameraPtr apicamera);
		void main();
		
		CameraPtr 	apicamera;
		int			port;
		
		// mutex to control access
		mutex							image_queue_access_mutex;
		mutex							image_queue_sync_mutex;
		// condition variable to sync
		//std::condition_variable 		image_queue_cv;

		// maximum image_queue length
		int								image_queue_maxlength;
		// buffer with images
		std::queue<CByteImage*>			image_queue_new;
		std::queue<CByteImage*>			image_queue_old;

	
};
/*********************
 * Constructor
 *********************/
CCamServer::CCamServer(int port, const string name, CameraPtr apicamera) : CServer(port, name){
	this->apicamera					= apicamera;
	this->port						= port;
	this->image_queue_maxlength		= CAM_SERVER_DEFAULT_QUEUE_LENGTH;
}

/*********************
 * Server main loop
 *********************/
void CCamServer::main(){
	int count	= 0;
	while(true){
		// wait until we have a new image to send
		CByteImage*		image	= NULL;
		while(true){
			image	=	NULL;
			image_queue_access_mutex.lock();
				if(image_queue_new.empty() == false){
					image	= image_queue_new.front();
					image_queue_new.pop();
				}								
			image_queue_access_mutex.unlock();
			if(image != NULL){
				break;
			}else{
				sleep(0.01);
			}
		}		
		// notify that we have removed one image
		
		// send command promt	
		cout << "image taken from the queue: " << count << " port: " << port << endl;
		count	+= 1;
		const string	welcome	= "camserver>\n";
		int	client_socket	= get_client_socket();
		try{
			// assemble header: width, height, bytes per pixel
			const size_t	header_length	= 12;
			uint8_t			header[header_length];
			ssize_t			sent_length;	

			header[0]	=	(image->width >> 0) & 0xFF;
			header[1]	=	(image->width >> 8) & 0xFF;
			header[2]	=	(image->width >> 16) & 0xFF;
			header[3]	=	(image->width >> 24) & 0xFF;

			header[4]	=	(image->height >> 0) & 0xFF;
			header[5]	=	(image->height >> 8) & 0xFF;
			header[6]	=	(image->height >> 16) & 0xFF;
			header[7]	=	(image->height >> 24) & 0xFF;
	
			header[8]	=	(image->bytesperpixel >> 0) & 0xFF;
			header[9]	=	(image->bytesperpixel >> 8) & 0xFF;
			header[10]	=	(image->bytesperpixel >> 16) & 0xFF;
			header[11]	=	(image->bytesperpixel >> 24) & 0xFF;

			// send header			
			sent_length	=	send(client_socket, header, header_length, 0);
			if(sent_length != header_length){
				perror("Socket send error");
				throw -1;
			}			
			
			// send data
			sent_length	=	send(client_socket, image->data, image->data_length, 0);
			if(sent_length != image->data_length){
				perror("Socket send error");
				throw -1;
			}			

		}catch(...){
			cout << "CCamServer error" << endl;
			fflush(stdout);
			return;
		}	
		// put the image on the delete list
		image_queue_access_mutex.lock();
			image_queue_old.push(image);
		image_queue_access_mutex.unlock();

	}//main loop
}
/*****************************************************************************/
// vimba callback class: a new image was captured
/*****************************************************************************/
// Call-back class: is called when a frame is captured
class MyObserver : public CVimbaObserver {

	public:	
	MyObserver(CameraPtr apicamera);
		
	void	FrameReceived (const FramePtr frame);

	CCamServer*		camserver;
};
/*********************
 * Constructor
 *********************/
MyObserver::MyObserver(CameraPtr apicamera) : CVimbaObserver(apicamera){

	camserver		= NULL;
	for(auto server : global.camserver_list){
		if(server->apicamera == apicamera){
			camserver	= server;
			break;
		}
	}
	if(camserver == NULL){
		perror("MyObserver: couldn't find matching camera server");
		throw	-1;
	}
}
/*********************
 * Callback function
 *********************/
void	MyObserver::FrameReceived (const FramePtr frame){
	VmbErrorType    err;

	// read frame information
	
	VmbUint32_t	width	= 0;
	VmbUint32_t	height	= 0;
	VmbUchar_t*	data	= NULL;

	err					= frame->GetWidth(width);
	if(err != VmbErrorSuccess){
		perror("MyObserver GetWidth error");
		throw -1;
	}
	err					= frame->GetHeight(height);
	if(err != VmbErrorSuccess){
		perror("MyObserver GetHeight error");
		throw -1;
	}
	err					= frame->GetImage(data);
	if(err != VmbErrorSuccess){
		perror("MyObserver GetImage error");
		throw -1;
	}
		
	// create new image
	CByteImage*		image		= new CByteImage((uint32_t)width,(uint32_t)height,2);
	//cout << "created: " << image << endl;

	// copy buffer
	memcpy(image->data, data, image->data_length);

	bool	wait	= true;
	while(true){
		camserver->image_queue_access_mutex.lock();
			// delete old images
			while(camserver->image_queue_old.empty() == false){
				CByteImage*		oldimage	= camserver->image_queue_old.front();
				camserver->image_queue_old.pop();
				delete	oldimage;
			}
			// push new image if possible
			if(camserver->image_queue_new.size() < camserver->image_queue_maxlength){
				wait = false;
				camserver-> image_queue_new.push(image);
			}
		camserver->image_queue_access_mutex.unlock();
		if(wait == true){
			sleep(0.01);
		}else{
			break;
		}
	}	
	//cout << "frame captured" << endl;
	apicamera->QueueFrame(frame);
}

/*****************************************************************************/
// main
/*****************************************************************************/

// Allied Vision devices
#define	AVT_DEV_ID	"1ab2:0001"

int main(int argc, char const *argv[]){


	signal(SIGPIPE, SIG_IGN);

	/*************************************************************************/
	// Parse command line arguments		
	/*************************************************************************/
	int	control_port		= DEFAULT_SERVER_PORT;

	global.stdout_lock.lock();
	cout << "===== welcome =====" << endl;
	cout << "I hope you are having a great day." << endl;
	fflush(stdout);
	global.stdout_lock.unlock();

	if(argc == 2){
		stringstream	port_str;
						port_str << argv[1];
						port_str >> control_port;
							
		//cout << "Network port set via command line: " << argv[1] << endl;
	}
	
	global.stdout_lock.lock();
	cout << "Control server port: " << control_port << endl;
	global.stdout_lock.unlock();
	cout << endl;
	
	/*************************************************************************/
	// Reset AVT usb devices	
	/*************************************************************************/
	/*
	  Reset the USB camera; libusb:
	  https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
	 */
	 
	cout << "===== usb reset =====" << endl;	
	libusb_device**					usb_dev_list;	//libusb device list
	libusb_device*					usb_device;		//libusb device 	
	vector<libusb_device_handle*>	avt_dev_list;	// all AVT devices	

	libusb_context*	usb_ctx 		= NULL; 		//libusb session information	
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
	   This should be improved in the future.
	 */
	printf("Number of usb devices (vendor:device): %i\n", devcount);
	libusb_device_descriptor usb_dev_desc	= {0};
	char dev_id_string [64];
		
	for (int i = 0; i < devcount; i++){
		usb_device	= usb_dev_list[i];
		if(libusb_get_device_descriptor(usb_device, &usb_dev_desc) < 0){
			perror("libusb get device descriptor error");
			throw -1;
		}
		sprintf (dev_id_string, "%04x:%04x", usb_dev_desc.idVendor, usb_dev_desc.idProduct);
		printf("\t%s", dev_id_string);
		if(strcmp(dev_id_string, AVT_DEV_ID) == 0){
			printf(" AVT device\n");
			libusb_device_handle*	avtdevice;
			avtdevice	= libusb_open_device_with_vid_pid(usb_ctx, usb_dev_desc.idVendor, usb_dev_desc.idProduct);
			avt_dev_list.push_back(avtdevice);
		}else{
			printf("\n");
		}
	}
	while(avt_dev_list.empty() == false){
		libusb_device_handle*	avtdevice;
		avtdevice				= avt_dev_list.back();
		avt_dev_list.pop_back();
				
		libusb_reset_device(avtdevice);
		libusb_release_interface(avtdevice, 0);
		printf("device has been reset.\n");			
		sleep(0.1);
	}

	libusb_free_device_list(usb_dev_list, 1);
	libusb_exit(usb_ctx);
	//libusb_reset_device	
	cout << endl;
	
	/*************************************************************************/
	// Open vimba, reset cameras via vimba	
	/*************************************************************************/
	cout << "===== vimba reset =====" << endl;
	// create new instance
	global.vimba			= new CMyVimba();
	// iterate through all cameras
	int camera_count	= global.vimba->get_camera_count();
	
	cout << "number of vimba cameras: " << camera_count << endl;
	cout << "reset cameras" << endl;
	for(int i = 0; i < camera_count; i++){
		CMyCamera* 		camera		= global.vimba->get_camera(i);
		camera->run_command("DeviceReset"); 
	}
	delete	global.vimba;	


	while(true){		
		sleep(1);
		// open vimba
		try{
			global.vimba			= new CMyVimba();
			// iterate through all cameras
			camera_count			= global.vimba->get_camera_count();
			if(camera_count == 0){
				cout << "waiting for vimba cameras... " << endl;
				fflush(stdout);
				delete	global.vimba;
			}else{
				break;
			}
		}catch(...){
			cout << "could not create vimba api" << endl;
		}
	}
	cout << endl;

	/*************************************************************************/
	// Open cameras, start capturing frames
	/*************************************************************************/
	try{
		cout << "===== start streaming =====" << endl;	
		cout << "number of vimba cameras: " << camera_count << endl;
		cout << endl;
		for(int i = 0; i < camera_count; i++){
			/*****************************************************************/
			// Open camera			
			/*****************************************************************/
			cout	<< "===== vimba camera " << i << " =====" << endl;
			
			CMyCamera*		camera			= global.vimba->get_camera(i);
			CameraPtr 		apicamera		= global.vimba->get_apicamera(i);
	
			cout	<< "camera: " << camera << "\t" << apicamera << endl;
			
			cout	<< camera->camID() << '\t'  << camera->camname() << '\t' << camera->cammodel() 
					<< '\t' << camera->camserial() << '\t' << camera->camintID() << endl;

			//FeaturePtrVector    features	= camera->get_feature_list();

			// set pixel format
			camera->set_feature_value("PixelFormat", VmbPixelFormatMono12);
			VmbInt64_t	pixel_format				= camera->get_feature_value("PixelFormat");
			cout << "pixel format:                  " << pixel_format << endl;
			
			// get connection speed
			// fake conversion (1024 would be proper) / (1000.0*1000.0)
			VmbInt64_t	speed						= camera->get_feature_value("DeviceLinkSpeed");
			cout << "DeviceLinkSpeed:               " << speed/(1000*1000) << " MByte/s" << endl; 			

			// get connection speed
			VmbInt64_t	throughput_limit			= camera->get_feature_value("DeviceLinkThroughputLimit");
			cout << "DeviceLinkThroughputLimit:     " << throughput_limit/(1000*1000) << " MByte/s" << endl;
			throughput_limit				= (VmbUint64_t)(60*1000*1000);
			camera->set_feature_value("DeviceLinkThroughputLimit", throughput_limit);
			throughput_limit						= camera->get_feature_value("DeviceLinkThroughputLimit");
			cout << "DeviceLinkThroughputLimit: " << throughput_limit/(1000*1000) << " MByte/s" << endl;

			/*****************************************************************/
			// Start camera server	
			/*****************************************************************/
			// create a new server
			string			namestring	= "camserver " + to_string(i);
			CCamServer*		camserver	= new CCamServer(control_port+i+1, namestring, apicamera);
	
			// add the server to the list
			global.camserver_list.push_back(camserver);
							
			/*****************************************************************/
			// Start capturing images		
			/*****************************************************************/

			// get image properties
			VmbInt64_t 	image_height	= camera->get_feature_value("Height");
			cout << "height: " << image_height  << " pixel " << endl;
			VmbInt64_t 	image_width		= camera->get_feature_value("Width");
			cout << "width: " << image_width  << " pixel " << endl;
			// get payload size
			VmbInt64_t 	payload_size	= camera->get_feature_value("PayloadSize");
			cout << "Payload size: " << payload_size  << "\t" << image_height * image_width * 2  << " Byte " << endl;
				
			
			cout << "start capture" << endl;
			camera->start_capture();
			cout << endl << endl;	


		}// cameras are capturing now
		
		while(true){
			//cout << "sleep" << endl;
			int	sleeptime	= 1;
			sleep(sleeptime);
		}

			//cout << "stop capture" << endl;
			//camera->stop_capture();


			//cout	<< "captured " << captured_frame_counter << " frames" << endl;
			//cout 	<< "data speed: " << 8.0 * double(payload_size * captured_frame_counter) / (sleeptime * 1000 * 1000) << " MBit/s" << endl;

		
	}
	catch(...){
		
	}
	// close vimba
	delete global.vimba;
} 
/*****************************************************************************/
 
