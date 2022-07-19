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
void	CVimba::init(){
	VmbErrorType    err;

	// start vimba
	err			= vimba_system.Startup();
	if(err != VmbErrorSuccess){
		perror ("vimba startup failed");
		throw -1;
	}
		
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
	VmbErrorType    err;
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
// Constructor
/*********************************************************/
// minimal initializations, call init()
CVimbaCamera::CVimbaCamera(){
	cout << "construct CVimbaCamera object" << endl;
}

/*********************************************************/
// Destructor
/*********************************************************/

CVimbaCamera::~CVimbaCamera(){
	cout << "delete CVimbaCamera object" << endl;
}

/*********************************************************/
// Close camera
/*********************************************************/

void	CVimbaCamera::close(){

	VmbErrorType    err;	
	// close the camera
	err 				= apicamera->Close();; 
	if(err != VmbErrorSuccess){
		perror ("vimba camera close failed");
		throw -1;
	}

}
/*********************************************************/
// Init
/*********************************************************/
// main initialization, must be called prior to anything else
//

void	CVimbaCamera::init(CameraPtr apicamera){

	cout << "initialize CVimbaCamera object" << endl;
	this->apicamera		= apicamera;

	VmbErrorType    err;	
	//close the camera
	err 				= apicamera->Close(); 
	if(err != VmbErrorSuccess){
		perror ("vimba camera close failed");
		throw -1;
	}


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
		FeaturePtrVector	feature_list;
		VmbErrorType    	err;	
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
// get a vimba feature value
/*********************************************************/

VmbInt64_t	CVimbaCamera::get_feature_value(string feature_name){
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

/*********************************************************/
// set a vimba feature value
/*********************************************************/

void	CVimbaCamera::set_feature_value(string feature_name, VmbInt64_t value){
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


