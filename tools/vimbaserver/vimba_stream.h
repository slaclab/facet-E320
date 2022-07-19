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
#ifndef __VIMBA_STREAM_H__
#define __VIMBA_STREAM_H__

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
// CVimbaStream: manages asynchronous image capturing
/*****************************************************************************/
/*  
 * provides high-level functionality for the vimba api
 *
 */

template	<class Observer>
class	CVimbaStream {
	public:
		// constructor
		CVimbaStream(CVimbaCamera* camera, size_t buffer_size);
		// destructor
		~CVimbaStream();		
		
		void	start_streaming();
		void	stop_streaming();
		
	protected:
		// camera object for which we stream
		CVimbaCamera*		camera;	
		// frames for streaming
		FramePtrVector 		frame_buffer;
		

};


/*****************************************************************************/
// constructor
/*****************************************************************************/
template	<class Observer>
CVimbaStream<Observer>::CVimbaStream(CVimbaCamera*	camera, size_t buffer_size){

	this->camera		= camera;
	this->frame_buffer	= FramePtrVector(buffer_size);
}
/*****************************************************************************/
// destructor
/*****************************************************************************/
template	<class Observer>
CVimbaStream<Observer>::~CVimbaStream(){
	cout << "CVimbaStream deleted" << endl;
}
/*****************************************************************************/
// start streaming
/*****************************************************************************/
template	<class Observer>
void CVimbaStream<Observer>::start_streaming(){

	VmbErrorType    err;
	CameraPtr		apicamera	= camera->get_apicamera();	

	// get payload size
	VmbInt64_t 	payload_size	= camera->get_feature_value("PayloadSize");

	// anounce the frames to the camera, register observer callback object
	for(FramePtrVector::iterator iter=frame_buffer.begin();
		iter	!= frame_buffer.end(); ++iter){
		(*iter).reset(new Frame(payload_size)); 	
		(*iter)->RegisterObserver(IFrameObserverPtr(new Observer(apicamera)));

		err		= apicamera->AnnounceFrame(*iter);
		if(err != VmbErrorSuccess){
			perror ("vimba camera AnnounceFrame failed");
			throw -1;
		}

	}
	// start the capture engine
	err			= apicamera->StartCapture();
	if(err != VmbErrorSuccess){
		perror ("vimba camera StartCapture failed");
		throw -1;
	}


	// queue all frames in the buffer
	for(FramePtrVector::iterator iter=frame_buffer.begin();
		iter	!= frame_buffer.end(); ++iter){

		err		= apicamera->QueueFrame(*iter);
		if(err != VmbErrorSuccess){
			perror ("vimba camera QueueFrame failed");
			throw -1;
		}
	}
	// start the acquisition
	camera->run_command("AcquisitionStart");
	
}
/*****************************************************************************/
// stop streaming
/*****************************************************************************/
template	<class Observer>
void CVimbaStream<Observer>::stop_streaming(){
	CameraPtr		apicamera	= camera->get_apicamera();	
	VmbErrorType    err;

	// stop the acquisition
	camera->run_command("AcquisitionStop");

	err			= apicamera->EndCapture();
	if(err != VmbErrorSuccess){
		perror ("vimba camera EndCapture failed");
		throw -1;
	}

	err			= apicamera->FlushQueue();
	if(err != VmbErrorSuccess){
		perror ("vimba camera FlushQueue failed");
		throw -1;
	}

	err			= apicamera->RevokeAllFrames();
	if(err != VmbErrorSuccess){
		perror ("vimba camera RevokeAllFrames failed");
		throw -1;
	}

	// unregister the observer
	for(FramePtrVector::iterator iter=frame_buffer.begin();
		iter	!= frame_buffer.end(); ++iter){

		err		= (*iter)->UnregisterObserver();

		if(err != VmbErrorSuccess){
			perror ("vimba camera UnregisterObserver failed");
			throw -1;
		}
	}

	apicamera->Close();

}
/*****************************************************************************/

#endif


