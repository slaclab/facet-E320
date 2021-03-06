/******************************************************************************
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
 *****************************************************************************/ 


/*
 * Vimba used "shared_ptr" to define pointer and std::vectors for vectors:
 * typedef std::vector<FramePtr> FramePtrVector;
 */


#ifndef __VIMBA_H__
#define __VIMBA_H__

#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>

// Vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"
 


using namespace std;
using namespace AVT;
using namespace VmbAPI;



template <class Observer, int buffer_length> class	CVimbaCamera;

/*****************************************************************************/
// Call-back class: is called when a frame is captured
/*****************************************************************************/
class CVimbaObserver : public IFrameObserver {

	public:	
					CVimbaObserver(CameraPtr apicamera);
					~CVimbaObserver();
	// Function called by the vimba API if a frame was received
	virtual	void	FrameReceived (const FramePtr frame) {};

	// be careful with this
	CameraPtr		apicamera;

};


/*****************************************************************************/
// Vimba Class: manages the vimba interface
/*****************************************************************************/
template <class Observer, int buffer_length>
class	CVimba{
	public:
		// constructor
		CVimba();
		// destructor
		~CVimba();
		//
		int											get_camera_count();
		// get camera by camera index
		CVimbaCamera<Observer, buffer_length>*		get_camera(int index);
		CameraPtr									get_apicamera(int index);

	protected:
		std::vector<CVimbaCamera<Observer, buffer_length>*>		
										camera_list;
		VimbaSystem&					vimba_system;
		CameraPtrVector 				camera_ptr_list;

};


/*****************************************************************************/
// Vimba camera class: control a single camera
/*****************************************************************************/
/*  
 * The observer has to be of type "CVimbaObserver"
 *
 */
template <class Observer, int buffer_length>
class	CVimbaCamera {
	public:
		// constructor
		CVimbaCamera(CameraPtr apicamera);
		// destructor
		~CVimbaCamera();
		//
		
		FeaturePtrVector	get_feature_list();
		void				run_command(string feature_name);
		VmbInt64_t			get_feature_value(string feature_name);
		void				set_feature_value(string feature_name, VmbInt64_t value);

		// camere information, read out from the camera by the class constructor
		string	camID()			{ return camID_private;};
		string	camname()		{ return camname_private;};
		string	cammodel()		{ return cammodel_private;};
		string	camserial()		{ return camserial_private;};
		string	camintID()		{ return camintID_private;};

		// capture images from the camera (stream)
		void					start_capture();
		void					stop_capture();


		
		
	protected:
		// make sure the observer class has access to the variables below
		friend class CVimbaObserver;
		friend class CVimba<Observer, buffer_length>;

		CameraPtr				apicamera;	// camera on which we operate

		// camere information, read out by constructor
		string					camID_private;
		string					camname_private;
		string					cammodel_private;
		string					camserial_private;
		string					camintID_private;
		
		// frame buffer for asynchronous image capturing
		FramePtrVector			frame_buffer;
};


/*****************************************************************************/
// Constructor
/*****************************************************************************/

inline
CVimbaObserver::CVimbaObserver(CameraPtr apicamera) : IFrameObserver(apicamera){
	//cout	<< "observer created" << endl;
	this->apicamera			= apicamera;
}

inline
CVimbaObserver::~CVimbaObserver(){
	//cout	<< "observer deleted" << endl;
}



/*****************************************************************************/
// Constructor
/*****************************************************************************/
// initialize the vimba system
template <class Observer, int buffer_length>
CVimba<Observer, buffer_length>::CVimba() : vimba_system(VimbaSystem::GetInstance()){
	cout << "using vimba " << vimba_system << endl;

	cout << "shutdown vimba" << endl;	
	vimba_system.Shutdown();
	sleep(1);

	// start vimba
	VmbErrorType    err;
	err			= vimba_system.Startup();
	if(err != VmbErrorSuccess){
		perror ("vimba startup failed");
		throw -1;
	}
		
	// get cameras: vector of std::shared_ptr<AVT::VmbAPI::Camera>		
	err			= vimba_system.GetCameras(camera_ptr_list);
	if(err != VmbErrorSuccess){
		perror ("error retrieving camera list");
		throw -1;
	}
	// test if we have found at least one camera
	int	camera_count	= camera_ptr_list.size();
	if(camera_count == 0){
		perror ("no vimba cameras found");
		throw -1;
	}

	cout	<< "camera_ptr_list: " << camera_count << endl;

	// create a camera object for each camera
	for(int i = 0; i < camera_count; i++){
		CVimbaCamera<Observer, buffer_length>*	camera;
		string		cid;
		camera_ptr_list[i]->GetID(cid);
		
		cout << "add new camera: " << i << "\t" << camera_ptr_list[i] << cid << endl;
	
		camera	= new CVimbaCamera<Observer, buffer_length>(camera_ptr_list[i]);
		camera_list.push_back(camera);
	}
	
}

/*****************************************************************************/
// Destructor
/*****************************************************************************/
// initialize the vimba system
template <class Observer, int buffer_length>
CVimba<Observer, buffer_length>::~CVimba(){

	// delete the camera objects
	while (camera_list.empty() == false){
		delete	camera_list.back();
				camera_list.pop_back();
	}

	cout << "stop using vimba " << vimba_system << endl;
	vimba_system.Shutdown();
}


/*****************************************************************************/
// Get number of cameras 
/*****************************************************************************/
template <class Observer, int buffer_length>
int		CVimba<Observer, buffer_length>::get_camera_count(){
		return	 camera_ptr_list.size();
}
/*****************************************************************************/
// Get camera 
/*****************************************************************************/
template <class Observer, int buffer_length>
CVimbaCamera<Observer, buffer_length>*	CVimba<Observer, buffer_length>::get_camera(int index){
	int	camera_count	= camera_ptr_list.size();
	if((index >= 0) and (index < camera_count)){
		return	camera_list[index];
	}else{
		perror("get_camera index error");
		throw	-1;
	}
}

template <class Observer, int buffer_length>
CameraPtr	CVimba<Observer, buffer_length>::get_apicamera(int index){
	int	camera_count	= camera_ptr_list.size();
	if((index >= 0) and (index < camera_count)){
		return	camera_list[index]->apicamera;
	}else{
		perror("get_apicamera index error");
		throw	-1;
	}
}

/*****************************************************************************/
// Constructor
/*****************************************************************************/
// initialize the vimba system
template <class Observer, int buffer_length>
CVimbaCamera<Observer, buffer_length>::CVimbaCamera(CameraPtr apicamera){
	cout << "construct vimba camera object" << endl;
	this->apicamera		= apicamera;

	VmbErrorType    err;	
	// open the camera
	err 				= apicamera->Open(VmbAccessModeFull); 
	if(err != VmbErrorSuccess){
		perror ("vimba camera open VmbAccessModeFull failed");
		throw -1;
	}

	/*********************************/
	// read camera information	
	/*********************************/
	// get camera ID
	err 		= apicamera->GetID(camID_private);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetID failed");
		throw -1;
	}
	
	// get camera name
	err 		= apicamera->GetName(camname_private);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetName failed");
		throw -1;
	}
	
	// get camera model
	err 		= apicamera->GetModel(cammodel_private);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetModel failed");
		throw -1;
	}						

	// get camera serial number
	err 		= apicamera->GetSerialNumber(camserial_private);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetSerialNumber failed");
		throw -1;
	}	

	// get camera model
	err 		= apicamera->GetInterfaceID(camintID_private);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetInterfaceID failed");
		throw -1;
	}

}
/*****************************************************************************/
// Destructor
/*****************************************************************************/
template <class Observer, int buffer_length>
CVimbaCamera<Observer, buffer_length>::~CVimbaCamera(){
	cout << "delete vimba camera object" << endl;
	apicamera->Close();
}



/*****************************************************************************/
// Start asynchronous capture
/*****************************************************************************/
template <class Observer, int buffer_length>
void		CVimbaCamera<Observer, buffer_length>::start_capture(){


	// get payload size
	VmbInt64_t 	payload_size	= this->get_feature_value("PayloadSize");

	// allocate frames for capturing
	for (int i = 0; i < buffer_length; i++){
		Frame*			frame		= new Frame(payload_size);
		FramePtr		frameptr	= FramePtr(frame);
		frame_buffer.push_back(frameptr);
	}

	// anounce the frames to the camera, register observer callback object
	for(auto frame : frame_buffer){
		frame->RegisterObserver(IFrameObserverPtr(new Observer(apicamera)));
		apicamera->AnnounceFrame(frame);
	}
	// start the capture engine
	apicamera->StartCapture();
	// queue all frames in the buffer
	for(auto frame : frame_buffer){
		apicamera->QueueFrame(frame);
	}
	// start the acquisition
	this->run_command("AcquisitionStart");
		
}
/*****************************************************************************/
// Stop asynchronous capture
/*****************************************************************************/
template <class Observer, int buffer_length>
void		CVimbaCamera<Observer, buffer_length>::stop_capture(){

	// stop the acquisition
	this->run_command("AcquisitionStop");
	//cout	<< "acquisition stopped" << endl;
	apicamera->EndCapture();
	//cout	<< "EndCapture" << endl;
	apicamera->FlushQueue();
	//cout	<< "FlushQueue" << endl;
	apicamera->RevokeAllFrames();
	//cout	<< "RevokeAllFrames" << endl;

	// unregister the observer
	for(auto frame : frame_buffer){
		frame->UnregisterObserver();
	}	
	//cout	<< "UnregisterObserver" << endl;
		
	// delete allocated frames: the smart pointer is taking care of this
	//while (frame_buffer.empty() == false){
	//	FramePtr	frameptr	= frame_buffer.back();
	//	Frame*		frame		= frameptr->get();
	//	delete		frame;
	//	frame_buffer.pop_back();
	//}
	//cout	<< "delete frame" << endl;
}




/*****************************************************************************/
// Get a list of all vimba features
/*****************************************************************************/
template <class Observer, int buffer_length>
FeaturePtrVector	CVimbaCamera<Observer, buffer_length>::get_feature_list(){
		FeaturePtrVector	feature_list;
		VmbErrorType    	err;	
		err 				= apicamera->GetFeatures(feature_list);
		if(err != VmbErrorSuccess){
			perror ("vimba camera GetFeatures failed");
			throw -1;
		}
		return	feature_list;
}


/*****************************************************************************/
// run a vimba command
/*****************************************************************************/
template <class Observer, int buffer_length>
void	CVimbaCamera<Observer, buffer_length>::run_command(string feature_name){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= apicamera->GetFeatureByName(cfeature_name, vimba_feature);
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
// get a vimba feature value
/*****************************************************************************/
template <class Observer, int buffer_length>
VmbInt64_t	CVimbaCamera<Observer, buffer_length>::get_feature_value(string feature_name){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= apicamera->GetFeatureByName(cfeature_name, vimba_feature);
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
// set a vimba feature value
/*****************************************************************************/
template <class Observer, int buffer_length>
void	CVimbaCamera<Observer, buffer_length>::set_feature_value(string feature_name, VmbInt64_t value){
	VmbErrorType    err;
	FeaturePtr		vimba_feature;
	string			err_msg;
	const char*		cfeature_name 	= feature_name.c_str();
	
	// get the feature
	err 		= apicamera->GetFeatureByName(cfeature_name, vimba_feature);
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


#endif
