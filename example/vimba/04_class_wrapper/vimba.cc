/*****************************************************************************/
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License 
// for more details.
// 
// You should have received a copy of the GNU General Public License along
// with this program. If not, see <https://www.gnu.org/licenses/>.
//
// Copyright Sebastian Meuren, 2022
//
/*****************************************************************************/ 

#include "vimba.h"

/*****************************************************************************/
// CVimba
/*****************************************************************************/

/*********************************************************/
// Constructor
/*********************************************************/
// minimal initialization, call init() later
CVimba::CVimba() : vimba_system(VimbaSystem::GetInstance()){

	cout << "start using vimba " << vimba_system << endl;
	// no error so far
	err		= VmbErrorSuccess;
}

/*********************************************************/
// Destructor
/*********************************************************/
// initialize the vimba system
CVimba::~CVimba(){

	cout << "stop using vimba " << vimba_system << endl;
	vimba_system.Shutdown();
}


/*********************************************************/
// Initialize
/*********************************************************/
// initialize the vimba system
void	CVimba::startup(){

	// start vimba
	err			= vimba_system.Startup();
	if(err != VmbErrorSuccess){
		perror ("vimba startup failed");
		throw -1;
	}
		
}

/*********************************************************/
// Update the camera list
/*********************************************************/
void	CVimba::update_camera_list(){

	// get cameras: vector of std::shared_ptr<AVT::VmbAPI::Camera>		
	err			= vimba_system.GetCameras(apicamera_list);
	if(err != VmbErrorSuccess){
		perror ("error retrieving camera list");
		throw -1;
	}

}


/*********************************************************/
// Shutdown
/*********************************************************/
// initialize the vimba system
void	CVimba::shutdown(){

	// shutdown vimba
	err			= vimba_system.Shutdown();
	if(err != VmbErrorSuccess){
		perror ("vimba shutdown failed");
		throw -1;
	}

}


/*********************************************************/
// Get number of cameras 
/*********************************************************/
int		CVimba::get_camera_count(){
		return	 apicamera_list.size();
}

/*********************************************************/
// Get camera 
/*********************************************************/

CameraPtr	CVimba::get_apicamera(int index){

	int	camera_count	= apicamera_list.size();
	if((index >= 0) and (index < camera_count)){
		return	apicamera_list[index];
	}else{
		perror("get_apicamera index error");
		throw	-1;
	}
}

/*****************************************************************************/
// CVimbaCamera
/*****************************************************************************/


/*********************************************************/
// constructor
/*********************************************************/
// minimal initializations, call init()
CVimbaCamera::CVimbaCamera(){
	cout << "construct CVimbaCamera object" << endl;
	
	this->camera_opened	= false;
}

/*********************************************************/
// destructor
/*********************************************************/

CVimbaCamera::~CVimbaCamera(){
	cout << "delete CVimbaCamera object" << endl;

	// close the camera if needed
	if(camera_opened == true){
		close();
	}
}


/*********************************************************/
// open camera
/*********************************************************/

void	CVimbaCamera::open(CameraPtr apicamera){

	// test if current camera is open
	if(camera_opened	== true){
		close();
	}

	this->apicamera		= apicamera;

	// open camera
	err 				= apicamera->Open(VmbAccessModeFull); 
	if(err != VmbErrorSuccess){
		perror ("vimba camera open VmbAccessModeFull failed");
		throw -1;
	}

	this->camera_opened	= true;
	
}


/*********************************************************/
// close camera
/*********************************************************/

void	CVimbaCamera::close(){


	if(camera_opened	== true){
		camera_opened	= false;
		// close the camera
		cout	<< "close camera" << endl;
		err 				= apicamera->Close();; 
		if(err != VmbErrorSuccess){
			perror ("vimba camera close failed");
			throw -1;
		}
	}

}

/*********************************************************/
// assert open camera
/*********************************************************/

void	CVimbaCamera::assert_open_camera(){

	// test if the camera is open
	if(camera_opened	== false){
		perror ("no open camera");
		throw -1;
	}

}

/*********************************************************/
// announce_frame
/*********************************************************/

void	CVimbaCamera::announce_frame(FramePtr frame){

	assert_open_camera();

	// execute command
	err 				= apicamera->AnnounceFrame(frame); 
	if(err != VmbErrorSuccess){
		perror ("vimba AnnounceFrame failed");
		throw -1;
	}

}

/*********************************************************/
// start_capture
/*********************************************************/

void	CVimbaCamera::start_capture(){

	assert_open_camera();

	// execute command
	err 				= apicamera->StartCapture(); 
	if(err != VmbErrorSuccess){
		perror ("vimba StartCapture failed");
		throw -1;
	}

}

/*********************************************************/
// queue_frame
/*********************************************************/

void	CVimbaCamera::queue_frame(FramePtr frame){

	assert_open_camera();

	// execute command
	err 				= apicamera->QueueFrame(frame); 
	if(err != VmbErrorSuccess){
		perror ("vimba QueueFrame failed");
		throw -1;
	}

}

/*********************************************************/
// end_capture
/*********************************************************/

void	CVimbaCamera::end_capture(){

	assert_open_camera();

	// execute command
	err 				= apicamera->EndCapture(); 
	if(err != VmbErrorSuccess){
		perror ("vimba EndCapture failed");
		throw -1;
	}

}

/*********************************************************/
// flush_queue
/*********************************************************/

void	CVimbaCamera::flush_queue(){

	assert_open_camera();

	// execute command
	err 				= apicamera->FlushQueue(); 
	if(err != VmbErrorSuccess){
		perror ("vimba FlushQueue failed");
		throw -1;
	}

}

/*********************************************************/
// revoke_all_frames
/*********************************************************/

void	CVimbaCamera::revoke_all_frames(){

	assert_open_camera();

	// execute command
	err 				= apicamera->RevokeAllFrames(); 
	if(err != VmbErrorSuccess){
		perror ("vimba RevokeAllFrames failed");
		throw -1;
	}

}



/*********************************************************/
// read camera information	
/*********************************************************/

void	CVimbaCamera::get_info(){

	assert_open_camera();

	// get camera ID
	err 		= apicamera->GetID(camID);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetID failed");
		throw -1;
	}
	
	// get camera name
	err 		= apicamera->GetName(camname);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetName failed");
		throw -1;
	}
	
	// get camera model
	err 		= apicamera->GetModel(cammodel);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetModel failed");
		throw -1;
	}						

	// get camera serial number
	err 		= apicamera->GetSerialNumber(camserial);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetSerialNumber failed");
		throw -1;
	}	

	// get camera model
	err 		= apicamera->GetInterfaceID(camintID);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetInterfaceID failed");
		throw -1;
	}

}


/*********************************************************/
// Get a list of all vimba features
/*********************************************************/

FeaturePtrVector	CVimbaCamera::get_feature_list(){

	assert_open_camera();

	FeaturePtrVector	feature_list;
	err 				= apicamera->GetFeatures(feature_list);
	if(err != VmbErrorSuccess){
		perror ("vimba camera GetFeatures failed");
		throw -1;
	}
	return	feature_list;

}

/*********************************************************/
// run a vimba command
/*********************************************************/

void	CVimbaCamera::run_command(string feature_name){

	assert_open_camera();

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
	// run command
	VmbInt64_t 	value;
	err			= vimba_feature->RunCommand();
	if(err != VmbErrorSuccess){
		err_msg	= "vimba RunCommand for " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}

}

/*********************************************************/
// get a vimba feature value: VmbInt64_t
/*********************************************************/

VmbInt64_t	CVimbaCamera::get_feature_value(string feature_name){

	assert_open_camera();

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

/*********************************************************/
// get a vimba feature value: double
/*********************************************************/

double	CVimbaCamera::get_feature_value_double(string feature_name){

	assert_open_camera();

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
	double 		value;
	err			= vimba_feature->GetValue(value);
	if(err != VmbErrorSuccess){
		err_msg	= "vimba GetValue of " + feature_name + " failed";
		const char*	cerr 	= err_msg.c_str();
		perror (cerr);
		throw -1;
	}

	return	value;
}

/*********************************************************/
// set a vimba feature value: VmbInt64_t
/*********************************************************/

void	CVimbaCamera::set_feature_value(string feature_name, VmbInt64_t value){

	assert_open_camera();

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

/*********************************************************/
// set a vimba feature value: double
/*********************************************************/

void	CVimbaCamera::set_feature_value_double(string feature_name, double value){

	assert_open_camera();

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


