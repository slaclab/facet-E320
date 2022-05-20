/********************************************************************************
 * Read data from a AVT camera via the Vimba SDK
 *
 * -- usage of the vimba API
 * -- asynchronous data acquisition.
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


int	global_frame_counter	= 0;

/*****************************************************************************/
// execute vimba command
/*****************************************************************************/
void	run_command(CameraPtr	camera, string feature_name){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= camera->GetFeatureByName(cfeature_name, vimba_feature);
	if(err != VmbErrorSuccess){
		err_msg				= "vimba GetFeatureByName " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}
	// get the value of the feature
	VmbInt64_t 	value;
	err			= vimba_feature->RunCommand();
	if(err != VmbErrorSuccess){
		err_msg	= "vimba RunCommand for " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}

}

/*****************************************************************************/
// get value
/*****************************************************************************/
VmbInt64_t	get_feature_value(CameraPtr	camera, string feature_name){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= camera->GetFeatureByName(cfeature_name, vimba_feature);
	if(err != VmbErrorSuccess){
		err_msg				= "vimba GetFeatureByName " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}
	// get the value of the feature
	VmbInt64_t 	value;
	err			= vimba_feature->GetValue(value);
	if(err != VmbErrorSuccess){
		err_msg	= "vimba GetValue of " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}

	return	value;
}

/*****************************************************************************/
// set value
/*****************************************************************************/
void	set_feature_value(CameraPtr	camera, string feature_name, VmbInt64_t value){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= camera->GetFeatureByName(cfeature_name, vimba_feature);
	if(err != VmbErrorSuccess){
		err_msg				= "vimba GetFeatureByName " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}
	// set the value of the feature
	err			= vimba_feature->SetValue(value);
	if(err != VmbErrorSuccess){
		err_msg	= "vimba SetValue of " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}

}

/*****************************************************************************/
// Callback
/*****************************************************************************/
class FrameObserver : public IFrameObserver{
	public:
				FrameObserver (CameraPtr camera) : IFrameObserver (camera){ this->camera = camera;};
				~FrameObserver (){cout << "FrameObserver deleted" << endl;};
		void	FrameReceived (const FramePtr frame);
	
	private:
		CameraPtr camera;
};


// Frame callback notifies about incoming frames
void FrameObserver::FrameReceived (const FramePtr frame)
{
	global_frame_counter += 1;
	camera->QueueFrame(frame);
}

/*****************************************************************************/
// Test that frames get properly deleted
/*****************************************************************************/
class MyFrame : public Frame{
	public:
	MyFrame(VmbInt64_t framesize) : Frame(framesize){};
	~MyFrame() { cout << "Frame deleted" << endl;};

};
/*****************************************************************************/


/*****************************************************************************/
// Not used anymore
/*****************************************************************************/
FramePtr	init_frameptr(Frame* frame){
	FramePtr	newptr;
	newptr.reset(frame);
	return	newptr;
}
/*****************************************************************************/

 

/*****************************************************************************/
// main
/*****************************************************************************/
int	main(){


	// get vimba
	VmbErrorType    err;
	VimbaSystem&    sys		= VimbaSystem::GetInstance();

	/*****************************************************************/
	// test loop
	/*****************************************************************/	
	int	testcount		= 0;
	while(true){
	
		cout	<< "===== test " << testcount << " =====" << endl;
		testcount	+= 1;
		try{	
			/*****************************************************************/
			// start vimba
			/*****************************************************************/	
			std::cout << "vimba: " << sys << endl;
			// start vimba
			err			= sys.Startup();
			if(err != VmbErrorSuccess){
				perror ("vimba startup failed");
				throw -1;
			}
			// get cameras: vector of std::shared_ptr<AVT::VmbAPI::Camera>		
			CameraPtrVector cameras;                           
			err			= sys.GetCameras(cameras);
			if(err != VmbErrorSuccess){
				perror ("error retrieving camera list");
				throw -1;
			}
			// test if we have found at least one camera
			int	camera_count	= cameras.size();
			if(camera_count == 0){
				perror ("no vimba cameras found");
				throw -1;
			}
			// iterate through all cameras
			cout << "cameras found: " << camera_count << endl;
			for(int i = 0; i < camera_count; i++){
				// look at next camera in the list
				CameraPtr	camera		= cameras[i];
		
				/*****************************************************************/
				// open camera
				/*****************************************************************/	
				// open the camera
				err 		= camera->Open(VmbAccessModeFull); 
				if(err != VmbErrorSuccess){
					perror ("vimba camera open VmbAccessModeFull failed");
					throw -1;
				}

				// get camera ID
				string	camID;
				err 		= camera->GetID(camID);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetID failed");
					throw -1;
				}
				
				// get camera name
				string	camname;
				err 		= camera->GetName(camname);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetName failed");
					throw -1;
				}
				
				// get camera model
				string	cammodel;
				err 		= camera->GetModel(cammodel);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetModel failed");
					throw -1;
				}						

				// get camera serial number
				string	camserial;
				err 		= camera->GetSerialNumber(camserial);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetSerialNumber failed");
					throw -1;
				}	

				// get camera model
				string	camintID;
				err 		= camera->GetInterfaceID(camintID);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetInterfaceID failed");
					throw -1;
				}

				cout	<< camID << '\t' 
						<< camname << '\t' << cammodel 
						<< '\t' << camserial << '\t' << camintID << endl;

				//CameraPtr	vimbacam	= CameraPtr(); 
				
				FeaturePtrVector    features;
				err 		= camera->GetFeatures(features);
				if(err != VmbErrorSuccess){
					perror ("vimba camera GetFeatures failed");
					throw -1;
				}
				
							
				// set pixel format
				set_feature_value(camera, "PixelFormat", VmbPixelFormatMono12);
				VmbInt64_t	pixel_format		= get_feature_value(camera, "PixelFormat");
				cout << "pixel format: " << pixel_format << endl;
				
				// get connection speed
				VmbInt64_t	speed				= get_feature_value(camera, "DeviceLinkSpeed");
				cout << "DeviceLinkSpeed: " << speed/(1000*1000) << " MByte/s" << endl; 			// fake conversion (1024 would be proper) / (1000.0*1000.0)

				// get connection speed
				VmbInt64_t	throughput_limit	= get_feature_value(camera, "DeviceLinkThroughputLimit");
				cout << "DeviceLinkThroughputLimit: " << throughput_limit/(1000*1000) << " MByte/s" << endl;
				//throughput_limit				= (VmbUint64_t)(200*1000*1000);
				//set_feature_value(camera, "DeviceLinkThroughputLimit", throughput_limit);
				//throughput_limit				= get_feature_value(camera, "DeviceLinkThroughputLimit");
				//cout << "DeviceLinkThroughputLimit: " << throughput_limit/(1000*1000) << " MByte/s" << endl;


								
				/*****************************************************************/
				// Capture images			
				/*****************************************************************/

				// get image properties
				VmbInt64_t 	image_height	= get_feature_value(camera, "Height");
				cout << "height: " << image_height  << " pixel " << endl;
				VmbInt64_t 	image_width		= get_feature_value(camera, "Width");
				cout << "width: " << image_width  << " pixel " << endl;



				// get payload size
				VmbInt64_t 	payload_size	= get_feature_value(camera, "PayloadSize");
				cout << "Payload size: " << payload_size  << "\t" << image_height * image_width * 2  << " Byte " << endl;
				
				FramePtrVector frames (10); 	// Frame array

				vector<IFrameObserverPtr>		fobserver_list;
				vector<FramePtr>				frame_list;
				
				for (int i = 0; i < 10; i++){
					FramePtr			frame		= FramePtr(new MyFrame(payload_size));

					IFrameObserverPtr	observer	= IFrameObserverPtr(new FrameObserver(camera));

					frame_list.push_back(frame);
					fobserver_list.push_back(observer);

					frame->RegisterObserver(observer);
					camera->AnnounceFrame(frame);
				}
				
				// start the capture engine
				camera->StartCapture();
				
				for (int i = 0; i < 10; i++){
					camera->QueueFrame(frame_list[i]);	
				}

				global_frame_counter	= 0;
				run_command(camera,"AcquisitionStart"); 			
				cout	<< endl << "===== start =====" << endl;
				
				int	sleeptime	= 3;
				sleep(sleeptime);
				cout	<< "captured " << global_frame_counter << " frames" << endl;
				cout 	<< "data speed: " << 8.0 * double(payload_size * global_frame_counter) / (sleeptime * 1000 * 1000) << " MBit / s" << endl;
				
				run_command(camera,"AcquisitionStop");
				cout << "===== stop =====" << endl << endl;

				try{
					camera->EndCapture();
					camera->FlushQueue();
					camera->RevokeAllFrames();
				}catch(...){
					cerr << "error cleaning up" << endl;
				}
				try{
					for (int i = 0; i < 10; i++){
						frame_list[i]->UnregisterObserver();
					}
				}catch(...){
					cerr << "error UnregisterObserver" << endl;
				}
				try{
					// clear lists
					while(true){
						if(frame_list.empty() == true){
							break;
						}
						FramePtr	frame	= frame_list.back();
						frame_list.pop_back();
					}

					// clear lists
					while(true){
						if(fobserver_list.empty() == true){
							break;
						}
						IFrameObserverPtr	observer	= fobserver_list.back();
						fobserver_list.pop_back();
					}
				}catch(...){
					cerr << "error clearing lists" << endl;
				}

				/*****************************************************************/
				// close camera
				/*****************************************************************/	
				try{
					camera->Close();
				}catch(...){
					cerr << "error camera Close" << endl;
				}

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
			cout << "ERROR" << endl;
		}
		/*****************************************************************/
		// close vimba
		/*****************************************************************/	
		// close vimba
		try{
			sys.Shutdown();
		}catch(...){
			cerr << "error shutting down vimba" << endl;
		}

		
	}
} 
 
 
