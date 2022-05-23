/********************************************************************************
 * Read data from an AVT camera via the Vimba SDK
 *
 *	- uses the vimba SDK to communicate with AVT cameras:
 *		- high-level functionality implemented in vimba.h
 *		- uses asynchronous capturing for data transfer
 *
 * 
 * Compile:
 * make
 *
 * Library access:
 * export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:
 *        /home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/
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
#include <unistd.h>
#include <stdio.h>

#include <sstream>
#include <iostream>
#include <fstream>

#include <iomanip>

#include <algorithm>
#include <vector>
#include <queue>

// string
#include "string.h"
#include <string>
#include <regex>

// time
#include <chrono>
#include <ctime>

// multi-threading
#include <thread>
#include <mutex>


// vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"

#include "vimba.h"

// thread-save queue
#include "queue.h"
 

using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;


// number of frames in the queue
#define		NUMBER_OF_FRAMES_IN_BUFFER 5
// maximum attempts
#define		CAMERA_MAX_ATTEMPTS		5
/*****************************************************************************/
// vimba main
/*****************************************************************************/

// main vimba instance, see "vimba.h"
CVimba		global_vimba;
mutex		CVimba::access_mutex;



/*****************************************************************************/
// callback: camera provides a new frame
/*****************************************************************************/
class FrameObserver : public IFrameObserver{
	public:
		// constructor
		FrameObserver (CameraPtr apicamera, CQueue<FramePtr>* framequeue);
		// destructor
		~FrameObserver ();
		// callback
		void	FrameReceived (const FramePtr frame);
	
	private:
		// camera which is sending the frames
		CameraPtr 			apicamera;
		// queue to which we should return the received frames
		CQueue<FramePtr>*	framequeue;
		
};

/***********************************************/
// constructor
/***********************************************/
FrameObserver::FrameObserver (CameraPtr apicamera, CQueue<FramePtr>* framequeue) : IFrameObserver (apicamera){ 
	this->apicamera 	= apicamera;
	this->framequeue	= framequeue;
}

/***********************************************/
// destructor
/***********************************************/
FrameObserver::~FrameObserver(){ 
	//cout << "FrameObserver deleted" << endl;
}


/***********************************************/
// callback: camera provides a new frame
/***********************************************/
// Frame callback notifies about incoming frames
void FrameObserver::FrameReceived (const FramePtr frame)
{

	// put the frame into the queue
	framequeue->push(frame);
	//apicamera->QueueFrame(frame);
}


/*****************************************************************************/
// frame class: overloaded destructor to make sure frames get deleted
/*****************************************************************************/
class MyFrame : public Frame{
	public:
	MyFrame(VmbInt64_t framesize) : Frame(framesize){};
	~MyFrame() {
	//cout << "Frame deleted" << endl;
	};
};

/*****************************************************************************/
// Get current date + time
/*****************************************************************************/
string	get_current_date_time_string(){

	// https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
	stringstream		datetimestring;
	auto 				now 		= std::chrono::system_clock::now();
	auto 				now_time_t	= std::chrono::system_clock::to_time_t(now);
	datetimestring	<< put_time(localtime(&now_time_t), "%a %b %d %H:%M:%S, %Y");

	return	datetimestring.str();
}

/*****************************************************************************/
// Camera streaming thread main function
/*****************************************************************************/
void	camera_streaming_main(string cameraID){

	// camera we are working with
	CameraPtr	apicamera;

	// frame-pointer queue
	CQueue<FramePtr>	framequeue;

	// for streaming data
	vector<IFrameObserverPtr>		fobserver_list;
	vector<FramePtr>				frame_list;


	// wait time after an error was encountered
	auto			error_timeout	= 3s;

	// open a file for "cout" logging
	string		outputfile_name	= cameraID + ".log";
	ofstream	outputfile;
	// we open the file once to empty it
	outputfile.open(outputfile_name, ios::trunc | ios::out);
	outputfile.close();
	// open it again for logging
	outputfile.open(outputfile_name, ios::app | ios::out);

	/***********************************************/
	// feature list
	/***********************************************/
	// vimba string		vimba data type		short (for user interface)
	//
	//
	vector<pair<string,string>> 	featurelist;
	
	featurelist.push_back(make_pair(string("PixelFormat"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("ExposureTime"), string("double")));
	featurelist.push_back(make_pair(string("DeviceLinkSpeed"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("DeviceLinkThroughputLimit"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("Height"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("Width"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("PayloadSize"), string("VmbInt64_t")));


	
	/***********************************************/
	// main loop
	/***********************************************/
	while(true){
		// make sure current stream is written to file
		outputfile.flush();		
		/***********************************************/
		// find camera
		/***********************************************/
		while(true){
			try{
				outputfile << "trying to open camera: " << cameraID << " " << get_current_date_time_string() << endl;
				apicamera	= global_vimba.get_apicamera_by_id(cameraID);	
			}catch(...){
				outputfile << "vimba get_apicamera_by_id failed" << endl;
				this_thread::sleep_for(error_timeout);
				continue;
			}
			break;
		}
		// high-level vimba camera access
		CVimbaCamera	camera(&global_vimba);
		/***********************************************/
		// close camera
		/***********************************************/
		try{
			camera.close();
		}catch(...){
			outputfile << "error closing camera" << endl;
			this_thread::sleep_for(error_timeout);
			continue;
		}
		/***********************************************/
		// open camera
		/***********************************************/
		try{			
			// open the camera for access
			camera.open(apicamera);
		}catch(...){
			outputfile << "error opening camera" << endl;
			this_thread::sleep_for(error_timeout);
			continue;
		}
		/***********************************************/
		try{
			/***********************************************/
			// retrieve basic camera info
			/***********************************************/
			try{			
				camera.get_info();
				outputfile	<< endl;
				outputfile	<< "===== camera " << camera.get_camID() << " =====" << endl;		
				outputfile << endl;		
			}catch(...){
				outputfile << "error get_info" << endl;
				throw	-1;
			}
			/***********************************************/
			// get & print current camera parameters
			/***********************************************/
			for(auto item : featurelist){
				try{
					string	printstring	= item.first + ":";			
					printstring.resize(40,' ');
					outputfile << printstring;
					if(item.second == "VmbInt64_t"){
						VmbInt64_t	value	= camera.get_feature_value(item.first);
						outputfile << value << endl;
					}else if(item.second == "double"){
						double		value	= camera.get_feature_value_double(item.first);
						outputfile << value << endl;
					}
				}catch(...){
					outputfile << "error reading " << camera.get_feature_value(item.first) << endl;
					throw	-1;
				}
			}
			outputfile << endl;	
			/***********************************************/
			// set default values
			/***********************************************/
			VmbUint64_t		throughput_limit 	= (VmbUint64_t)(30*1000*1000);
			bool			success				= false;
			try{			
				//camera.open(apicamera, VmbAccessModeConfig);
				camera.set_feature_value("DeviceLinkThroughputLimit", throughput_limit);
			}catch(...){
				outputfile << "error setting DeviceLinkThroughputLimit "  << endl;
			}

			/***********************************************/
			// prepare streaming of data
			/***********************************************/

			// get current payload size
			VmbInt64_t	payload_size	= 0;
			try{			
				camera.open(apicamera);
				payload_size	=	camera.get_feature_value("PayloadSize");
						
			}catch(...){
				outputfile << "error reading PayloadSize" << endl;
				throw	-1;
			}

			// clear the old frame queue
			while(true){
				if(framequeue.is_empty() == true){
					break;
				}
				// queue is not empty, remove one frame
				FramePtr		frame			= framequeue.pop();
				throw	-1;
			}				

			// create new frames
			try{
				for (int i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++){
					FramePtr			frame		= FramePtr(new MyFrame(payload_size));
					IFrameObserverPtr	observer	= IFrameObserverPtr(new FrameObserver(apicamera, &framequeue));

					frame_list.push_back(frame);
					fobserver_list.push_back(observer);
				}
			}catch(...){
				outputfile << "error reading PayloadSize" << endl;
				throw	-1;
			}
			
			// register observer, announce frames
			try{
				for (int i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++){
					frame_list[i]->RegisterObserver(fobserver_list[i]);
					camera.announce_frame(frame_list[i]);
				}
			}catch(...){
				outputfile << "error RegisterObserver / announce_frame" << endl;
				throw	-1;
			}
			
			// start the capture engine
			try{
				camera.start_capture();
			}catch(...){
				outputfile << "error start_capture" << endl;
				throw	-1;
			}

			// queue frames
			try{
				for (int i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++){
					camera.queue_frame(frame_list[i]);	
				}
			}catch(...){
				outputfile << "error queue_frame" << endl;
				throw	-1;
			}

			/***********************************************/
			// start data acquisition
			/***********************************************/
			// duration of the acquisition (in seconds)
			int		acquisition_time		= 5;
			auto	timeout_start			= system_clock::now();
			int		framecounter			= 0;

			try{
				camera.run_command("AcquisitionStart"); 			
				outputfile	<< endl << "===== start " << get_current_date_time_string() << " =====" << endl;
			}catch(...){
				outputfile << "error AcquisitionStart" << endl;
				throw	-1;
			}

			/***********************************************/
			// acquisition main loop
			/***********************************************/
			try{
				while(true){
					// test for timeout
					auto	current_time		= system_clock::now();
					int		tdiff				= duration_cast<chrono::seconds>(current_time - timeout_start).count();			
					if(tdiff > acquisition_time){
						// time elapsed
						break;
					}
					// process frames
					while(true){
						bool			queue_empty		= framequeue.is_empty();
						if(queue_empty == true){
							break;
						}
						// queue is not empty, process frames
						FramePtr		frame			= framequeue.pop();
						// re-queue the frame to the camera
						apicamera->QueueFrame(frame);
						framecounter++;			
					}				
								
					// sleep brievly 
					this_thread::sleep_for(0.5ms);
				}
			}catch(...){
				outputfile << "error during acquisition loop" << endl;
				throw	-1;
			}
			/***********************************************/
			// end of data acquisition
			/***********************************************/
			try{
				camera.run_command("AcquisitionStop");
				outputfile	<< "captured " << double(framecounter)/double(acquisition_time) << " FPS" << endl;
				outputfile	<< "data speed: " << 8.0 * double(payload_size * framecounter) / (acquisition_time * 1000 * 1000) << " MBit / s" << endl;
				outputfile << "===== stop " << get_current_date_time_string() << " =====" << endl << endl;
			}catch(...){
				outputfile << "error AcquisitionStop" << endl;
				throw	-1;
			}			

			// clean up after the acquisition
			try{
				camera.end_capture();
				camera.flush_queue();
				camera.revoke_all_frames();
			}catch(...){
				outputfile << "error cleaning up after acquisition" << endl;
				throw	-1;
			}
			
			// unregister observers
			try{
				for (int i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++){
					frame_list[i]->UnregisterObserver();
				}
			}catch(...){
				outputfile << "error UnregisterObserver" << endl;
				throw	-1;
			}

			// empty the frame list
			try{
				while(true){
					if(frame_list.empty() == true){
						break;
					}
					FramePtr	frame	= frame_list.back();
					frame_list.pop_back();
				}
			}catch(...){
				outputfile << "error emtpying frame_list" << endl;
				throw	-1;
			}
			
			// empty the observer list
			try{
				while(true){
					if(fobserver_list.empty() == true){
						break;
					}
					IFrameObserverPtr	observer	= fobserver_list.back();
					fobserver_list.pop_back();
				}
			}catch(...){
				outputfile << "error emtpying fobserver_list" << endl;
				throw	-1;
			}
			/***********************************************/
			// close camera
			/***********************************************/
			try{
				camera.close();
			}catch(...){
				outputfile << "error closing camera" << endl;
				throw	-1;
			}
			/***********************************************/
		}catch(...){
			/***********************************************/
			// error occured
			/***********************************************/
			try{
				camera.close();
			}catch(...){
				outputfile << "error closing camera" << endl;
				this_thread::sleep_for(error_timeout);
			}
			this_thread::sleep_for(error_timeout);
			/***********************************************/
		}
	}
}





/*****************************************************************************/
// main
/*****************************************************************************/


int main(int argc, char const *argv[]){

	// changes the rdbuf, now we are on our own
	//CStream(tout).rdbuf(tout.rdbuf());
	
	
	// camera list
	
	vector<string>	camID_list;
	
	camID_list.push_back("DEV_1AB22C011FE5");
	camID_list.push_back("DEV_1AB22C011FE8");


	/*****************************************************************/
	// start vimba
	/*****************************************************************/	
	try{			
		global_vimba.startup();
		// read camera list from vimba
		global_vimba.update_camera_list();
		// get camera count
	}catch(...){
		cerr << "error during startup" << endl;
	}
		
	int		camera_count	= global_vimba.get_camera_count();
	cout	<< "number of cameras: " << camera_count << endl;

	/*****************************************************************/
	// start the camera threads
	/*****************************************************************/	

	vector<thread>	thread_list;
		
	for (int camera_index = 0; camera_index < camID_list.size(); camera_index++){
		thread		newthread(camera_streaming_main, camID_list[camera_index]);
		thread_list.push_back(move(newthread));
	}

	for (int thread_index = 0; thread_index < thread_list.size(); thread_index++){
		thread_list[thread_index].join();
	}



	
	/*****************************************************************/
	// close vimba
	/*****************************************************************/	
	try{			
		global_vimba.shutdown();
	}catch(...){
		cerr << "error during shutdown" << endl;
	}

	return	0;
} 
/*****************************************************************************/
 
