/******************************************************************************
 *
 * Changelog:
 *
 * v0.02:	changed philosophy, CVimba is now only created once, CVimbaCamera
 *			works differently now.
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




/*****************************************************************************/
// Vimba Class: manages the vimba interface
/*****************************************************************************/
class	CVimba{
	public:
		// constructor
		CVimba();
		// destructor
		~CVimba();

		// initialize everything
		void					init();

		// shutdown everything
		void					shutdown();

		// number of cameras
		int						get_camera_count();

		// get camera by camera index
		CameraPtr				get_apicamera(int index);

		void					print_vimba_info(void){ cout << vimba_system; };

	protected:
		VimbaSystem&			vimba_system;
		CameraPtrVector 		apicamera_list;

};


/*****************************************************************************/
// Vimba camera class: control a single camera
/*****************************************************************************/
/*  
 * provides high-level functionality for the vimba api
 *
 */

class	CVimbaCamera {
	public:
		// constructor
		CVimbaCamera();
		// destructor
		~CVimbaCamera();

		// initialize
		void				init(CameraPtr apicamera);
		// close camera
		void				close();
		
		FeaturePtrVector	get_feature_list();
		void				run_command(string feature_name);
		VmbInt64_t			get_feature_value(string feature_name);
		void				set_feature_value(string feature_name, VmbInt64_t value);

		// camere information, read out from the camera by the class constructor
		string				get_camID()			{ return camID;};
		string				get_camname()		{ return camname;};
		string				get_cammodel()		{ return cammodel;};
		string				get_camserial()		{ return camserial;};
		string				get_camintID()		{ return camintID;};

		CameraPtr			get_apicamera()		{ return apicamera;};

		// capture images from the camera (stream)
		//void				start_capture();
		//void				stop_capture();

				
	protected:
		CameraPtr				apicamera;	// camera on which we operate

		// camere information, read out by constructor
		string					camID;
		string					camname;
		string					cammodel;
		string					camserial;
		string					camintID;
		

};

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
#endif
