/********************************************************************************
 * Read data from a AVT camera via the Vimba SDK
 *
 * -- usage of the vimba API
 * -- synchronous data acquisition.
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

// Vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"
 
 
#define	VIMBA_ACQUIRE_TIME_OUT 5000
 
using namespace std;
using namespace AVT;
using namespace VmbAPI;

 
int	main(){

	// get vimba
	VmbErrorType    err;
	VimbaSystem&    sys		= VimbaSystem::GetInstance();
	try{	
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
			FeaturePtr	vimba_feature;
			err 		= camera->GetFeatureByName("PixelFormat", vimba_feature);
			if(err != VmbErrorSuccess){
				perror ("vimba GetFeatureByName PixelFormat failed");
				throw -1;
			}
			//VmbPixelFormatMono8
			err			= vimba_feature->SetValue(VmbPixelFormatMono12);
			if(err != VmbErrorSuccess){
				perror ("vimba SetValue VmbPixelFormatMono12 failed");
				throw -1;
			}			
			
			// take an image
			for(int i=0;i<20;i++){
				cout << "take image: " << i << endl;
			FramePtr 	image_frame;
				err 		= camera->AcquireSingleImage(image_frame, VIMBA_ACQUIRE_TIME_OUT);   
				if(err != VmbErrorSuccess){
					perror ("vimba AcquireSingleImage failed");
					throw -1;
				}
			}
		}
	

	}
	catch(...){
		
	}
	// close vimba
	sys.Shutdown();
} 
 
 
