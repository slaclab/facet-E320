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
// thread-save queue
#include "queue.h"


#include "global.h"

// string
#include <string>

// time
#include <chrono>
#include <ctime>

using namespace std;
using namespace std::chrono;

#ifndef __CAMSERVER_H__
#define __CAMSERVER_H__

/*****************************************************************************/
// CCamServer
/*****************************************************************************/


template <int queue_length>
class CCamServer : public CServer<CCamServer<queue_length>, queue_length>{
	public:
		CCamServer(int port, const string name, CQueue<FramePtr>* callback_to_server, CQueue<FramePtr>* server_return);
		void main();
		
		int						port;
		CQueue<FramePtr>*		callback_to_server_framequeue;
		CQueue<FramePtr>*		server_return_framequeue;
		
};

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
template <int queue_length>
CCamServer<queue_length>::CCamServer(int port, const string name, CQueue<FramePtr>* callback_to_server_framequeue, CQueue<FramePtr>* server_return_framequeue) : CServer<CCamServer<queue_length>, queue_length>(port, name){
	this->port								= port;

	// frame queue: read from here
	this->callback_to_server_framequeue		= callback_to_server_framequeue;

	// frame queue: return here
	this->server_return_framequeue			= server_return_framequeue;

}

/*************************************************/
// Server main loop
/*************************************************/
template <int queue_length>
void CCamServer<queue_length>::main(){


	string	server_name			= ((CServer<CCamServer<queue_length>, queue_length>*)this)->get_server_name();


	//global.stdout_lock.lock();
	cout << "CCamServer: main loop started "  << "\t" << server_name << endl;
	//global.stdout_lock.unlock();


	// clear the frame queue
	/*
	while(true){
		if(callback_to_server_framequeue->is_empty() == true){
			break;
		}
		// queue is not empty, remove one frame
		cout	<< "return frame" << endl;
		FramePtr		frame			= callback_to_server_framequeue->pop();
		server_return_framequeue->push(frame);
	}				
	*/

	
	/*************************************************/
	// main loop
	/*************************************************/
	bool	stop_streaming	= false;
	while(true){
		/*************************************************/
		// send frames via tcp/ip
		/*************************************************/
		cout	<< "waiting for new frame" << endl;
		while(true){
			if(callback_to_server_framequeue->is_empty() == false){
				break;
			}
			// sleep brievly 
			this_thread::sleep_for(1ms);
		}	

		// take the next frame from the queue
		FramePtr		frame			= callback_to_server_framequeue->pop();
		VmbUint64_t		frameid;
		frame->GetFrameID(frameid);
		cout	 << get_current_date_time_string() << " camserver: new frame " << frameid  << endl;

		/*************************************************/
		// process new frame
		/*************************************************/
		VmbErrorType    err;
		VmbUint32_t		buffer_size		= 0;

		VmbUint32_t		width			= 0;
		VmbUint32_t		height			= 0;
		VmbUint32_t		offset_x		= 0;
		VmbUint32_t		offset_y		= 0;
		
		VmbUint32_t		pixel_format	= 0;

		VmbUint64_t		time_stamp		= 0;
		VmbUint64_t		frame_id		= 0;

		VmbUchar_t*		data			= NULL;
		try{
			err					= frame->GetBufferSize(buffer_size);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetBufferSize error");
				throw -1;
			}

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

			err					= frame->GetOffsetX(offset_x);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetOffsetX error");
				throw -1;
			}

			err					= frame->GetOffsetY(offset_y);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetOffsetY error");
				throw -1;
			}

			err					= frame->GetPixelFormat((VmbPixelFormatType&)pixel_format);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetPixelFormat error");
				throw -1;
			}

			err					= frame->GetFrameID(frame_id);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetFrameID error");
				throw -1;
			}

			err					= frame->GetTimestamp(time_stamp);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetTimeStamp error");
				throw -1;
			}

			err					= frame->GetImage(data);
			if(err != VmbErrorSuccess){
				perror("MyObserver GetImage error");
				throw -1;
			}
		}catch(...){
			data	= NULL;
		}			
		/*************************************************/
		// send the image we just got
		/*************************************************/
		if(data != NULL){
			int	bytesperpixel	= int(buffer_size / (height*width));
		
			int	client_socket	= ((CServer<CCamServer<queue_length>, queue_length>*)this)->get_client_socket();
			try{
				// assemble header: width, height, bytes per pixel
				const size_t	header_length	= 40;
				uint8_t			header[header_length];
				ssize_t			sent_length;	

				int32_to_buffer(header,0, buffer_size);
				int32_to_buffer(header,4, width);
				int32_to_buffer(header,8, height);
				int32_to_buffer(header,12, offset_x);
				int32_to_buffer(header,16, offset_y);
				int32_to_buffer(header,20, pixel_format);
				int64_to_buffer(header,24, time_stamp);
				int64_to_buffer(header,32, frame_id);

				// send header			
				sent_length	=	send(client_socket, header, header_length, 0);
				if(sent_length != header_length){
					cerr	<< "socket error: " << client_socket << "\t" << port << endl;
					perror("Socket send error: header");
					throw -1;
				}			
				
				// send data
				sent_length	=	send(client_socket, data, buffer_size, 0);
				if(sent_length != buffer_size){
					cerr	<< "socket error: " << client_socket << "\t" << port << endl;
					perror("Socket send error: data");
					throw -1;
				}			

			}catch(...){
				/*************************************************/
				// stop the server
				/*************************************************/
				//global.stdout_lock.lock();
				cout << "CCamServer send error, stop streaming " << server_name << endl;
				//global.stdout_lock.unlock();

				stop_streaming	= true;
				/*************************************************/
			}	
		}
		/*************************************************/
		// return frame
		/*************************************************/
		// return the frame, such that it can be filled with new data

		frame->GetFrameID(frameid);
		cout	 << get_current_date_time_string() << " camserver: return frame " << frameid  << endl;
		server_return_framequeue->push(frame);
		/*************************************************/
		if(stop_streaming == true){
			break;
		}
	}//main loop
}


/*****************************************************************************/
#endif
