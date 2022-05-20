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

 

using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;

int	global_frame_counter;

/*****************************************************************************/
// Callback
/*****************************************************************************/
class FrameObserver : public IFrameObserver{
	public:
				FrameObserver (CameraPtr camera) : IFrameObserver (camera){ this->camera = camera;};
				~FrameObserver (){cout << "FrameObserver deleted" << endl;};
		void	FrameReceived (const FramePtr frame);
	
	private:
		CameraPtr camera;
};


// Frame callback notifies about incoming frames
void FrameObserver::FrameReceived (const FramePtr frame)
{
	global_frame_counter += 1;
	camera->QueueFrame(frame);
}

/*****************************************************************************/
// Test that frames get properly deleted
/*****************************************************************************/
class MyFrame : public Frame{
	public:
	MyFrame(VmbInt64_t framesize) : Frame(framesize){};
	~MyFrame() { cout << "Frame deleted" << endl;};

};
/*****************************************************************************/



/*****************************************************************************/
// main
/*****************************************************************************/


int main(int argc, char const *argv[]){

	// main vimba instance
	CVimba			vimba;

	while(true){
		/*****************************************************************/
		// start vimba
		/*****************************************************************/	
		try{			
			vimba.startup();
			// read camera list from vimba
			vimba.update_camera_list();
			// get camera count
		}catch(...){
			cerr << "error during startup" << endl;
		}
		
		
		int		camera_count	= vimba.get_camera_count();
		cout	<< "number of cameras: " << camera_count << endl;

		/*****************************************************************/
		// Access camera information
		/*****************************************************************/	
		for(int i = 0; i < camera_count; i++){
			CameraPtr		apicamera		= vimba.get_apicamera(i);	
			CVimbaCamera	camera;

			try{			
				camera.open(apicamera);
				camera.get_info();

				cout	<< endl;
				cout	<< "===== camera " << camera.get_camID() << " =====" << endl;		


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

				cout << endl;		
				for(auto item : featurelist){
					try{
						string	printstring	= item.first + ":";			
						printstring.resize(32,' ');
						cout << printstring;
						if(item.second == "VmbInt64_t"){
							VmbInt64_t	value	= camera.get_feature_value(item.first);
							cout << value << endl;
						}else if(item.second == "double"){
							double		value	= camera.get_feature_value_double(item.first);
							cout << value << endl;
						}
					}catch(...){
						cerr << "error" << endl;
					}
				}
				cout << endl;	
			}catch(...){
				cerr << "error reading information" << endl;
			}
			/*****************************************************************/
			// set certain features
			/*****************************************************************/	

			try{			
				VmbUint64_t		throughput_limit = (VmbUint64_t)(60*1000*1000);
				camera.set_feature_value("DeviceLinkThroughputLimit", throughput_limit);
			}catch(...){
				cerr << "error setting information" << endl;
			}
		}


		/*****************************************************************/
		// Stream data
		/*****************************************************************/	
		vector<IFrameObserverPtr>		fobserver_list;
		vector<FramePtr>				frame_list;

		for(int i = 0; i < camera_count; i++){
			CameraPtr		apicamera		= vimba.get_apicamera(i);	
			CVimbaCamera	camera;
			VmbInt64_t 		payload_size	= 0;

				try{			
							camera.open(apicamera);
							payload_size	=	camera.get_feature_value("PayloadSize");
							
				}catch(...){
					cerr << "error getting the payload_size" << endl;
				}

				int	number_of_frames	= 10;
				
				for (int i = 0; i < number_of_frames; i++){
					FramePtr			frame		= FramePtr(new MyFrame(payload_size));
					IFrameObserverPtr	observer	= IFrameObserverPtr(new FrameObserver(apicamera));

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

					global_frame_counter	= 0;

					camera.run_command("AcquisitionStart"); 			
					cout	<< endl << "===== start =====" << endl;
				
					int		sleeptime	= 5;
					sleep(sleeptime);
				
					cout	<< "captured " << global_frame_counter << " frames" << endl;
					cout 	<< "data speed: " << 8.0 * double(payload_size * global_frame_counter) / (sleeptime * 1000 * 1000) << " MBit / s" << endl;
				
					camera.run_command("AcquisitionStop");
					cout << "===== stop =====" << endl << endl;

				}catch(...){
					cerr << "acquisition failed" << endl;
				}
				try{
					camera.end_capture();
					camera.flush_queue();
					camera.revoke_all_frames();
				}catch(...){
					cerr << "error cleaning up" << endl;
				}
				try{
					for (int i = 0; i < number_of_frames; i++){
						frame_list[i]->UnregisterObserver();
					}
				}catch(...){
					cerr << "error UnregisterObserver" << endl;
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
					cerr << "error clearing lists" << endl;
				}

				/*****************************************************************/
				// close camera
				/*****************************************************************/	
				try{
					camera.close();
				}catch(...){
					cerr << "error camera Close" << endl;
				}


		}

		/*****************************************************************/
		// close vimba
		/*****************************************************************/	
		try{			
			vimba.shutdown();
		}catch(...){
			cerr << "error during shutdown" << endl;
		}
	}
	return	0;
} 
/*****************************************************************************/
 
