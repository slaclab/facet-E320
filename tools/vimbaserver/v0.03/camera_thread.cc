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

#include <sstream>
#include <iostream>
#include <fstream>


// thread-save queue
#include "queue.h"

// small helper functions
#include "tools.h"

// vimba include files
#include "VimbaCPP/Include/VimbaCPP.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"

using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;


/*****************************************************************************/
// server
/*****************************************************************************/
#include "server.h"
#include "camserver.h"

using 	CMyCamServer		= CCamServer<CAM_SERVER_QUEUE_LENGTH>;


#include	"global.h"
/*****************************************************************************/
// vimba
/*****************************************************************************/
#include "vimba.h"
extern	CVimba		global_vimba;

/*****************************************************************************/
// xml parsing: config file
/*****************************************************************************/
#include "pugixml.hpp"
extern pugi::xml_document 	xmlconfig;
extern mutex				xmlconfig_mutex;


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
	//cout	<< "callback: new frame" << endl;
	VmbUint64_t		frameid;
	frame->GetFrameID(frameid);
	cout	 << get_current_date_time_string() << " FrameObserver: new frame " << frameid  << endl;

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
// Camera streaming thread main function
/*****************************************************************************/
void	camera_streaming_main(string cameraID, int server_port){

	// camera we are working with
	CameraPtr	apicamera;

	// frame-pointer queue
	CQueue<FramePtr>				callback_to_server_framequeue;
	CQueue<FramePtr>				server_return_framequeue;

	// for streaming data
	vector<IFrameObserverPtr>		fobserver_list;
	vector<FramePtr>				frame_list;


	// wait time after an error was encountered
	auto		error_timeout	= seconds(3s);

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
	featurelist.push_back(make_pair(string("Gain"), string("double")));
	featurelist.push_back(make_pair(string("DeviceLinkSpeed"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("DeviceLinkThroughputLimit"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("AcquisitionFrameRate"), string("double")));
	featurelist.push_back(make_pair(string("Height"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("Width"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("BinningHorizontal"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("BinningVertical"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("PayloadSize"), string("VmbInt64_t")));
	featurelist.push_back(make_pair(string("LineSelector"), string("enum")));
	featurelist.push_back(make_pair(string("LineMode"), string("enum")));
	featurelist.push_back(make_pair(string("TriggerSource"), string("enum")));
	featurelist.push_back(make_pair(string("TriggerMode"), string("enum")));
	

	/***********************************************/
	// start server thread
	/***********************************************/
	
	// create a new server
	string			namestring	= "camserver_" + cameraID;
	CMyCamServer*	camserver	= new CMyCamServer(server_port, namestring, &callback_to_server_framequeue, &server_return_framequeue);
	// start the thread
	thread			camserver_thread(&CMyCamServer::execute, camserver);
	
	
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
		// reset camera
		/***********************************************/
		try{
			VmbErrorType    err;
			FeaturePtr		vimba_feature;

			// get the feature
			err 		= apicamera->GetFeatureByName("DeviceReset", vimba_feature);
			if(err != VmbErrorSuccess){
				perror ("DeviceReset failed: GetFeatureByName");
				throw -1;
			}
			
			// run command
			err			= vimba_feature->RunCommand();
			if(err != VmbErrorSuccess){
				perror ("DeviceReset failed: RunCommand");
				throw -1;
			}
		}catch(...){
			outputfile << "error reseting camera" << endl;
			this_thread::sleep_for(error_timeout);
			continue;
		}
		this_thread::sleep_for(5s);


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


		try{

			/***********************************************/
			// xml parsing
			/***********************************************/
			outputfile << endl;
			outputfile << "parse config file for camera default values" << endl;
			xmlconfig_mutex.lock();
			int	i = 0;
			for (pugi::xml_node xmlcamera : xmlconfig.child("config").children("camera"))
			{
				if(cameraID == xmlcamera.attribute("id").as_string()){
					outputfile << "camera config information found: " << endl;
					outputfile << endl;
					for (pugi::xml_node default_setting : xmlcamera.children("default_setting"))
					{	
						string	attribute		= default_setting.attribute("name").as_string();
						string	value			= default_setting.attribute("value").as_string(); 
						string	method			= default_setting.attribute("method").as_string();
						string	comment			= default_setting.attribute("comment").as_string(); 

						if(method == "VmbInt64_t"){
							stringstream		convert;
							VmbInt64_t			intvalue;
												convert		<< value;
												convert		>> intvalue;

							try{			
								//camera.open(apicamera, VmbAccessModeConfig);
								camera.set_feature_value(attribute, intvalue);
								attribute.resize(40,' ');
								outputfile << "successful set " << attribute << " to " << intvalue << " " << comment << endl;
							}catch(...){
								attribute.resize(40,' ');
								outputfile << "ERROR setting  " << attribute << " to " << intvalue << " " << value << " " << comment << endl;
							}
						}else if(method == "double"){
							stringstream		convert;
							double				doublevalue;
												convert		<< value;
												convert		>> doublevalue;

							try{			
								//camera.open(apicamera, VmbAccessModeConfig);
								camera.set_feature_value_double(attribute, doublevalue);
								attribute.resize(40,' ');
								outputfile << "successful set " << attribute << " to " << doublevalue << " " << comment << endl;
							}catch(...){
								attribute.resize(40,' ');
								outputfile << "ERROR setting  " << attribute << " to " << doublevalue << " " << comment << endl;
							}
						}else if(method == "enum"){
							try{			
								camera.set_feature_value_enum(attribute, value);
								attribute.resize(40,' ');
								outputfile << "successful set " << attribute << " to " << value << " " << comment << endl;
							}catch(...){
								attribute.resize(40,' ');
								outputfile << "ERROR setting  " << attribute << " to " << value << " " << comment << endl;
							}
						}else{
							outputfile << "method unknown: " << method << endl;
						}
					}
				}
			}
			xmlconfig_mutex.unlock();	

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
					}else if(item.second == "enum"){
						string		value	= camera.get_feature_value_enum(item.first);
						outputfile << value << endl;
					}
				}catch(...){
					outputfile << "error reading " << camera.get_feature_value(item.first) << endl;
					throw	-1;
				}
			}
			outputfile << endl;	


			/***********************************************/
			// prepare streaming of data
			/***********************************************/

			// get current payload size
			VmbInt64_t	payload_size	= 0;
			try{			
				//camera.open(apicamera);
				payload_size	=	camera.get_feature_value("PayloadSize");
						
			}catch(...){
				outputfile << "error reading PayloadSize" << endl;
				throw	-1;
			}

			// clear the old frame queue
			while(true){
				if(server_return_framequeue.is_empty() == true){
					break;
				}
				// queue is not empty, remove one frame
				FramePtr		frame			= server_return_framequeue.pop();
			}				

			// create new frames
			try{
				for (int i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++){
					FramePtr			frame		= FramePtr(new MyFrame(payload_size));
					IFrameObserverPtr	observer	= IFrameObserverPtr(new FrameObserver(apicamera, &callback_to_server_framequeue));

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
						//break;
					}
					// process frames
					while(true){
						if(server_return_framequeue.is_empty() == true){
							break;
						}
						// queue is not empty, process frames
						FramePtr		frame			= server_return_framequeue.pop();
						// re-queue the frame to the camera
						VmbUint64_t		frameid;
						frame->GetFrameID(frameid);
						cout	 << get_current_date_time_string() << " camera_thread: queue frame again " << frameid  << endl;
						apicamera->QueueFrame(frame);
						framecounter++;			
					}				
								
					// sleep brievly 
					this_thread::sleep_for(1ms);
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


