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

#include "camserver.h"
#include "vimba_capture.h"

/*****************************************************************************/
// vimba callback class: a new image was captured
/*****************************************************************************/


/*************************************************/
// constructor
/*************************************************/
MyObserver::MyObserver(CameraPtr apicamera) : CVimbaObserver(apicamera){

	// store pointer
	this->apicamera		= apicamera;
	// initialize high-level camera object
	this->camera.init(apicamera);
	// get camid
	camid				= camera.get_camID();
	// find matching server
	camserver			= NULL;

	global.global_lock.lock();
	int		camcount	= global.camera_id_list.size();
	for(int i = 0; i < camcount; i++){
		if(global.camera_id_list[i] == camid){
			// camera found
			camserver	= global.camserver_list[i];
			break;
		}
	}
	global.global_lock.unlock();

	cout << "constructed MyObserver: " << camid << "\t" << camserver->get_server_name() << endl;

	if(camserver == NULL){
		perror("MyObserver: couldn't find matching camera server");
		throw	-1;
	}
}
/*************************************************/
// new image callback function
/*************************************************/

void	MyObserver::FrameReceived (const FramePtr frame){
	VmbErrorType    err;
	CByteImage*		image	= NULL;
	int 			server_status_copy;

	//cout << "callback" << endl;


	/*************************************************/
	// delete old images
	/*************************************************/
	camserver->server_access_mutex.lock();
	try{
		// delete old images
		while(true){
			if(camserver->image_queue_old.empty() == true){
				break;
			}
			CByteImage*		oldimage	= camserver->image_queue_old.front();
			camserver->image_queue_old.pop();
			delete	oldimage;
		}
	}
	catch(...){
		global.stdout_lock.lock();
		cout << "MyObserver: error deleting used frames " << camserver->get_server_name() << endl;
		global.stdout_lock.unlock();
	}
	camserver->server_access_mutex.unlock();

	/*************************************************/
	// test if we should wait for a different state
	/*************************************************/
	while(true){
		// copy the current state
		camserver->server_access_mutex.lock();
		server_status_copy	= camserver->server_status;
		camserver->server_access_mutex.unlock();			
			
		if((server_status_copy == CAM_SERVER_INIT) or
			(server_status_copy == CAM_SERVER_STOP) or
			(server_status_copy == CAM_SERVER_ERROR)){
			this_thread::sleep_for(500ms);
			continue;
		}else{
			break;
		}
	}
	/*************************************************/
	// test if we should start streaming data
	/*************************************************/
	if(server_status_copy == CAM_SERVER_START_REQUEST){
		// we should start streaming (again)
		
		global.stdout_lock.lock();
		cout << "MyObserver: start streaming requested for " << camserver->get_server_name() << endl;
		global.stdout_lock.unlock();

		/*************************************************/
		// clear the current queue
		/*************************************************/
		// we should (re)start streaming
		camserver->server_access_mutex.lock();
		try{
			// delete images which might still be in the queue
			while(true){
				if(camserver->image_queue_new.empty() == true){
					break;
				}
				CByteImage*		image	= camserver->image_queue_new.front();
				camserver->image_queue_new.pop();
				delete	image;
			}
		}catch(...)
		{
			global.stdout_lock.lock();
			cout << "MyObserver: error deleting unused frames " << camserver->get_server_name() << endl;
			global.stdout_lock.unlock();
		}

		/*************************************************/
		// change status
		/*************************************************/
		camserver->server_status			= CAM_SERVER_STREAMING;
		server_status_copy					= CAM_SERVER_STREAMING;
		camserver->server_access_mutex.unlock();

		// return for now
		apicamera->QueueFrame(frame);
		return;

	}

	/*************************************************/
	// test for non-streaming status
	/*************************************************/
	if(server_status_copy != CAM_SERVER_STREAMING){
		apicamera->QueueFrame(frame);
		return;
	}

	/*************************************************/
	// test for space in queue
	/*************************************************/
	int	queue_size;
	int	max_queue_size;

	auto	timeout_start			= system_clock::now();
	while(true){
		auto	current_time		= system_clock::now();
		int		tdiff				= duration_cast<chrono::seconds>(current_time - timeout_start).count();
		if(tdiff > CAM_SERVER_TIMEOUT){
			/*************************************************/
			// time out: stop streaming
			/*************************************************/
			camserver->server_access_mutex.lock();
			camserver->server_status	= CAM_SERVER_STOP;
			camserver->server_access_mutex.unlock();		

			global.stdout_lock.lock();
			cout << "MyObserver: queue full (time out), stop streaming for " << camserver->get_server_name() << endl;
			global.stdout_lock.unlock();

			apicamera->QueueFrame(frame);
			return;
			/*************************************************/
		}		
		camserver->server_access_mutex.lock();
		queue_size			= camserver->image_queue_new.size();
		max_queue_size		= camserver->image_queue_maxlength;
		camserver->server_access_mutex.unlock();

		// there is space in the queue
		if(queue_size < max_queue_size){
			break;
		}
		this_thread::sleep_for(50ms);
	}

	/*************************************************/
	// put the current frame into the queue
	/*************************************************/
	
	try{
		/*************************************************/
		// create image
		/*************************************************/	
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
		image				= new CByteImage((uint32_t)width,(uint32_t)height,2);
		//cout << "created: " << image << endl;
		// copy buffer
		memcpy(image->data, data, image->data_length);

		/*************************************************/
		// put image into queue
		/*************************************************/	
		camserver->server_access_mutex.lock();
		camserver-> image_queue_new.push(image);
		camserver->server_access_mutex.unlock();

	}catch(...){
		cout << "frame queueing error" << endl;
	}
	apicamera->QueueFrame(frame);
}
/*****************************************************************************/
