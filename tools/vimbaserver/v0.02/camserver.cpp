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


/******************************************************************************
 * Image class
 *****************************************************************************/
/*
 * The vimba callback function creates an image of this type and does a 
 * memcpy of the data. Then, this image is passed to the tcp/ip server
 * to send it over the network.
 *
 */
 
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
/*
 * The server provides a queue that is filled with new images by the vimba
 * callback function executed in a different thread. This callback function
 * is doing all the active part (creating new images, delete old ones, etc.)
 * The main loop of the server just streams data via tcp/ip as soon as they
 * are available.
 *
 */


/*********************
 * Constructor
 *********************/
CCamServer::CCamServer(int port, const string name) : CServer(port, name){
	this->port						= port;

	// status: server has just been initialized, no camera yet
	this->server_status				= CAM_SERVER_INIT;

	// buffer
	this->image_queue_maxlength		= CAM_SERVER_DEFAULT_QUEUE_LENGTH;

}

/*************************************************/
// Server main loop
/*************************************************/
void CCamServer::main(){
	CByteImage*		image	= NULL;
	int count				= 0;

	/*************************************************/
	// test for error
	/*************************************************/
	int	server_status_copy		= 0;

	// read the current status
	server_access_mutex.lock();
	server_status_copy	= server_status;
	server_access_mutex.unlock();

	if((server_status_copy == CAM_SERVER_INIT)
		or (server_status_copy == CAM_SERVER_ERROR)){
		global.stdout_lock.lock();
		cout << "CCamServer: wait for proper status " << get_server_name() << "\t" << server_status_copy << endl;
		fflush(stdout);
		global.stdout_lock.unlock();

		sleep(15);
		return;
	}

	/*************************************************/
	// request to start streaming
	/*************************************************/

	global.stdout_lock.lock();
	cout << "CCamServer: request to start streaming " << get_server_name() << endl;
	fflush(stdout);
	global.stdout_lock.unlock();

	// change the status
	server_access_mutex.lock();
	server_status	= CAM_SERVER_START_REQUEST;
	server_access_mutex.unlock();

	/*************************************************/
	// wait for frame to send
	/*************************************************/
	while(true){
		/*************************************************/
		// wait for CAM_SERVER_STREAMING + new image
		/*************************************************/
		try{
			// save time for timeout
			auto	timeout_start			= system_clock::now();
			while(true){
				/*************************************************/
				// test for timeout 
				/*************************************************/
				auto	current_time		= system_clock::now();
				int		tdiff				= duration_cast<chrono::seconds>(current_time - timeout_start).count();
				if(tdiff > CAM_SERVER_TIMEOUT){
				
					global.stdout_lock.lock();
					cout << "CCamServer: timeout " << get_server_name() << endl;
					fflush(stdout);
					global.stdout_lock.unlock();

					// change the status
					server_access_mutex.lock();
					server_status	= CAM_SERVER_ERROR;
					server_access_mutex.unlock();
					return;
				}
				/*************************************************/
				// wait for status CAM_SERVER_STREAMING
				/*************************************************/
				// read the current status
				server_access_mutex.lock();
				server_status_copy	= server_status;
				server_access_mutex.unlock();

				if(server_status_copy != CAM_SERVER_STREAMING){
					//global.stdout_lock.lock();
					//cout << "CCamServer: not streaming " << get_server_name() << "\t" << server_status_copy << endl;
					//fflush(stdout);
					//global.stdout_lock.unlock();
					continue;
				}
						
				/*************************************************/
				// test for new frame
				/*************************************************/
				image	= NULL;
				server_access_mutex.lock();
					if(image_queue_new.empty() == false){
						image	= image_queue_new.front();
						image_queue_new.pop();
					}								
				server_access_mutex.unlock();
				if(image != NULL){
					break;
				}			
				/*************************************************/
				sleep(0.01);
			}
		}catch(...){
			global.stdout_lock.lock();
			cout << "CCamServer admin error " << get_server_name() << endl;
			fflush(stdout);
			global.stdout_lock.unlock();

		}		
		/*************************************************/
		// send the image we just got
		/*************************************************/
		//global.stdout_lock.lock();
		//cout << "CCamServer image processed " << get_server_name() << "port: " << port << endl;
		//fflush(stdout);
		//global.stdout_lock.unlock();

		count	+= 1;
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
			/*************************************************/
			// stop streaming
			/*************************************************/
			global.stdout_lock.lock();
			cout << "CCamServer send error, stop streaming " << get_server_name() << endl;
			fflush(stdout);
			global.stdout_lock.unlock();

			// change the status
			server_access_mutex.lock();
			server_status	= CAM_SERVER_STOP;
			server_access_mutex.unlock();
			return;
			/*************************************************/
		}	
		/*************************************************/
		// image processed, delete it
		/*************************************************/
		// put the image into the delete list
		if(image != NULL){
			server_access_mutex.lock();
			image_queue_old.push(image);
			server_access_mutex.unlock();
		}
	}//main loop
}

