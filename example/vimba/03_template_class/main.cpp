/********************************************************************************
 * Read data from a AVT camera via the Vimba SDK
 *
 * -- usage of the vimba API via the template classes provided in vimba.h
 * -- asynchronous data acquisition.
 * 
 * Compile:
 * g++ main.cpp 
 * -I /home/facet/Vimba_6_0/ 
 * -I /home/facet/Vimba_6_0/VimbaCPP/Examples/ 
 * -L /home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/ -lVimbaCPP
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


 
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>

// Vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"
 
 
#define	VIMBA_ACQUIRE_TIME_OUT 5000
 
using namespace std;
using namespace AVT;
using namespace VmbAPI;

int	captured_frame_counter;

#include "vimba.h" 

/*****************************************************************************/
// Call-back class: is called when a frame is captured
/*****************************************************************************/
class MyObserver : public CVimbaObserver {

	public:	
	MyObserver(CameraPtr camera) : CVimbaObserver(camera) {};

	void	FrameReceived (const FramePtr frame);
};


void	MyObserver::FrameReceived (const FramePtr frame){

	//cout << "frame captured" << endl;
	captured_frame_counter += 1;
	camera->QueueFrame(frame);

}




// use these types from now on
using CMyCamera	= CVimbaCamera<MyObserver,10>;
using CMyVimba	= CVimba<MyObserver,10>;

/*****************************************************************************/
int	main(){


	CMyVimba*			vimba	= NULL;	
	CMyCamera*			camera	= NULL;
	VmbErrorType    err;
	try{	
		// open vimba
		vimba	= new CMyVimba();
		// iterate through all cameras
		int camera_count	= vimba->get_camera_count();
		
		cout << "cameras found: " << camera_count << endl;
		for(int i = 0; i < camera_count; i++){
			// look at next camera in the list
			cout	<< "get camera: " << i << endl;
			camera		= vimba->get_camera(i);
	


			cout	<< camera->camID() << '\t' 
					<< camera->camname() << '\t' << camera->cammodel() 
					<< '\t' << camera->camserial() << '\t' << camera->camintID() << endl;

			//CameraPtr	vimbacam	= CameraPtr(); 
			
			FeaturePtrVector    features	= camera->get_feature_list();

			// set pixel format
			camera->set_feature_value("PixelFormat", VmbPixelFormatMono12);
			VmbInt64_t	pixel_format		= camera->get_feature_value("PixelFormat");
			cout << "pixel format: " << pixel_format << endl;
			
			// get connection speed
			VmbInt64_t	speed				= camera->get_feature_value("DeviceLinkSpeed");
			cout << "DeviceLinkSpeed: " << speed/(1000*1000) << " MByte/s" << endl; 			// fake conversion (1024 would be proper) / (1000.0*1000.0)

			// get connection speed
			VmbInt64_t	throughput_limit	= camera->get_feature_value("DeviceLinkThroughputLimit");
			cout << "DeviceLinkThroughputLimit: " << throughput_limit/(1000*1000) << " MByte/s" << endl;
			//throughput_limit				= (VmbUint64_t)(200*1000*1000);
			//set_feature_value(camera, "DeviceLinkThroughputLimit", throughput_limit);
			//throughput_limit				= get_feature_value(camera, "DeviceLinkThroughputLimit");
			//cout << "DeviceLinkThroughputLimit: " << throughput_limit/(1000*1000) << " MByte/s" << endl;


							
			/*****************************************************************/
			// Capture images			
			/*****************************************************************/

			// get image properties
			VmbInt64_t 	image_height	= camera->get_feature_value("Height");
			cout << "height: " << image_height  << " pixel " << endl;
			VmbInt64_t 	image_width		= camera->get_feature_value("Width");
			cout << "width: " << image_width  << " pixel " << endl;
			// get payload size
			VmbInt64_t 	payload_size	= camera->get_feature_value("PayloadSize");
			cout << "Payload size: " << payload_size  << "\t" << image_height * image_width * 2  << " Byte " << endl;


			cout << endl << endl;	
			
			captured_frame_counter	= 0;		
			
			cout << "start capture" << endl;
			camera->start_capture();
			cout << "sleep" << endl;
			int	sleeptime	= 1;
			sleep(sleeptime);
			cout << "stop capture" << endl;
			camera->stop_capture();


			cout	<< "captured " << captured_frame_counter << " frames" << endl;
			cout 	<< "data speed: " << 8.0 * double(payload_size * captured_frame_counter) / (sleeptime * 1000 * 1000) << " MBit / s" << endl;
			

			delete	camera;


			// take an image
			/*
			for(int i=0;i<20;i++){
				cout << "take image: " << i << endl;
			FramePtr 	image_frame;
				err 		= camera->AcquireSingleImage(image_frame, VIMBA_ACQUIRE_TIME_OUT);   
				if(err != VmbErrorSuccess){
					perror ("vimba AcquireSingleImage failed");
					throw -1;
				}
			}
			*/
		}
	

	}
	catch(...){
		
	}
	// close vimba
	delete vimba;
} 
 
 
