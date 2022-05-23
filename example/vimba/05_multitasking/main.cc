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

/*****************************************************************************/
// vimba main
/*****************************************************************************/

// main vimba instance
CVimba		global_vimba;
mutex		CVimba::access_mutex;




/*****************************************************************************/
// callback: camera provides a new frame
/*****************************************************************************/
class FrameObserver : public IFrameObserver{
	public:
				FrameObserver (CameraPtr apicamera, CQueue<FramePtr>* framequeue) : IFrameObserver (apicamera){ 
						this->apicamera 	= apicamera;
						this->framequeue	= framequeue;
				};

				~FrameObserver (){cout << "FrameObserver deleted" << endl;};
		void	FrameReceived (const FramePtr frame);
	
	private:
		CameraPtr 			apicamera;
		CQueue<FramePtr>*	framequeue;
		
};


// Frame callback notifies about incoming frames
void FrameObserver::FrameReceived (const FramePtr frame)
{

	// put the frame into the queue
	framequeue->push(frame);
	//apicamera->QueueFrame(frame);
}


/*****************************************************************************/
// Test that frames get properly deleted
/*****************************************************************************/
class MyFrame : public Frame{
	public:
	MyFrame(VmbInt64_t framesize) : Frame(framesize){};
	~MyFrame() {cout << "Frame deleted" << endl;};

};
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

	// open a file for logging of what is happening
	string		outputfile_name	= cameraID + ".log";
	ofstream	outputfile;
	// open the file for output, overwrite old content
	outputfile.open(outputfile_name, ios::trunc | ios::out);
	outputfile.close();

	// frame-pointer queue
	CQueue<FramePtr>	framequeue;
	
	auto			error_timeout	= 3s;
	
	// main loop
	while(true){

		outputfile.open(outputfile_name, ios::app | ios::out);						
		outputfile << "trying to open camera: " << cameraID << endl;
			
		try{
			apicamera	= global_vimba.get_apicamera_by_id(cameraID);	
		}catch(...){
			cerr << "failed" << endl;
			outputfile.close();
			this_thread::sleep_for(error_timeout);
			continue;
		}
		
		CVimbaCamera	camera(&global_vimba);

		try{			
			camera.open(apicamera);
			camera.get_info();

			outputfile	<< endl;
			outputfile	<< "===== camera " << camera.get_camID() << " =====" << endl;		

			/*****************************************************************/
			// print certain features
			/*****************************************************************/	
			vector<pair<string,string>> 	featurelist;
			
			featurelist.push_back(make_pair(string("PixelFormat"), string("VmbInt64_t")));
			featurelist.push_back(make_pair(string("ExposureTime"), string("double")));
			featurelist.push_back(make_pair(string("DeviceLinkSpeed"), string("VmbInt64_t")));
			featurelist.push_back(make_pair(string("DeviceLinkThroughputLimit"), string("VmbInt64_t")));
			featurelist.push_back(make_pair(string("Height"), string("VmbInt64_t")));
			featurelist.push_back(make_pair(string("Width"), string("VmbInt64_t")));
			featurelist.push_back(make_pair(string("PayloadSize"), string("VmbInt64_t")));

			outputfile << endl;		
			for(auto item : featurelist){
				try{
					string	printstring	= item.first + ":";			
					printstring.resize(32,' ');
					outputfile << printstring;
					if(item.second == "VmbInt64_t"){
						VmbInt64_t	value	= camera.get_feature_value(item.first);
						outputfile << value << endl;
					}else if(item.second == "double"){
						double		value	= camera.get_feature_value_double(item.first);
						outputfile << value << endl;
					}
				}catch(...){
					outputfile << "error" << endl;
					this_thread::sleep_for(error_timeout);
				}
			}
			outputfile << endl;	
		}catch(...){
			outputfile << "error reading information" << endl;
			this_thread::sleep_for(error_timeout);
		}
		/*****************************************************************/
		// set certain features
		/*****************************************************************/	

		try{			
			VmbUint64_t		throughput_limit = (VmbUint64_t)(40*1000*1000);
			camera.set_feature_value("DeviceLinkThroughputLimit", throughput_limit);
		}catch(...){
			outputfile << "error setting information" << endl;
			this_thread::sleep_for(error_timeout);
		}

		/*****************************************************************/
		// Stream data
		/*****************************************************************/	
		vector<IFrameObserverPtr>		fobserver_list;
		vector<FramePtr>				frame_list;

		VmbInt64_t	payload_size	= 0;
		try{			
			camera.open(apicamera);
			payload_size	=	camera.get_feature_value("PayloadSize");
					
		}catch(...){
			outputfile << "error getting the payload_size" << endl;
			this_thread::sleep_for(error_timeout);
		}

		// clear the old frame queue
		while(true){
			bool			queue_empty		= framequeue.is_empty();
			if(queue_empty == true){
				break;
			}
			// queue is not empty, remove one frame
			FramePtr		frame			= framequeue.pop();
		}				


		// create new frames
		int	number_of_frames	= 10;
		for (int i = 0; i < number_of_frames; i++){
			FramePtr			frame		= FramePtr(new MyFrame(payload_size));
			IFrameObserverPtr	observer	= IFrameObserverPtr(new FrameObserver(apicamera, &framequeue));

			frame_list.push_back(frame);
			fobserver_list.push_back(observer);
		}

		try{

			for (int i = 0; i < number_of_frames; i++){
				frame_list[i]->RegisterObserver(fobserver_list[i]);
				camera.announce_frame(frame_list[i]);
			}
		
			// start the capture engine
			camera.start_capture();
		
			for (int i = 0; i < number_of_frames; i++){
				camera.queue_frame(frame_list[i]);	
			}


			camera.run_command("AcquisitionStart"); 			
			outputfile	<< endl << "===== start " << get_current_date_time_string() << " =====" << endl;

			/*****************************************************************/
			// start of data acquisition
			/*****************************************************************/

			// specify the duration of the acquisition (in seconds)
			int		acquisition_time		= 5;
			auto	timeout_start			= system_clock::now();
			int		framecounter			= 0;
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
				this_thread::sleep_for(1ms);
			}
			/*****************************************************************/
			// end of data acquisition
			/*****************************************************************/
			outputfile	<< "captured " << double(framecounter)/double(acquisition_time) << " FPS" << endl;
			outputfile	<< "data speed: " << 8.0 * double(payload_size * framecounter) / (acquisition_time * 1000 * 1000) << " MBit / s" << endl;
		
			camera.run_command("AcquisitionStop");
			outputfile << "===== stop " << get_current_date_time_string() << " =====" << endl << endl;

		}catch(...){
			outputfile << "acquisition failed" << endl;
			this_thread::sleep_for(error_timeout);
		}
		try{
			camera.end_capture();
			camera.flush_queue();
			camera.revoke_all_frames();
		}catch(...){
			outputfile << "error cleaning up" << endl;
			this_thread::sleep_for(error_timeout);
		}
		try{
			for (int i = 0; i < number_of_frames; i++){
				frame_list[i]->UnregisterObserver();
			}
		}catch(...){
			outputfile << "error UnregisterObserver" << endl;
			this_thread::sleep_for(error_timeout);
		}
		try{
			// clear lists
			while(true){
				if(frame_list.empty() == true){
					break;
				}
				FramePtr	frame	= frame_list.back();
				frame_list.pop_back();
			}

			// clear lists
			while(true){
				if(fobserver_list.empty() == true){
					break;
				}
				IFrameObserverPtr	observer	= fobserver_list.back();
				fobserver_list.pop_back();
			}
		}catch(...){
			outputfile << "error clearing lists" << endl;
			this_thread::sleep_for(error_timeout);
		}

		/*****************************************************************/
		// close camera
		/*****************************************************************/	
		try{
			camera.close();
		}catch(...){
			outputfile << "error camera Close" << endl;
			this_thread::sleep_for(error_timeout);
		}
		/*****************************************************************/	
		// close the file again
		/*****************************************************************/	
		outputfile.close();

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
 
