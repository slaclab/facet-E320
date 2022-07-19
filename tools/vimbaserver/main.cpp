/********************************************************************************
 * Read data from a AVT camera via the Vimba SDK
 *
 *	- resets AVT devices via libusb-1.0
 *	- uses the vimba SDK to communicate with AVT cameras:
 *		- classes implemented in vimba.h
 *		- resets the cameras
 *		- sets communication speed
 *		- uses asynchronous capturing for data transfer
 *	- uses a TCP/IP server to stream the frames
 *		- based on classes defined in server.h
 *
 *
 *	- future: uses seperate server for control commands		
 * 
 * Compile:
 * g++ main.cpp usb.cpp camserver.cpp vimba_capture.cpp 
 *				-I /home/facet/Vimba_6_0/ 
 *				-I /home/facet/Vimba_6_0/VimbaCPP/Examples/ 
 * 				-L /home/facet/Vimba_6_0/VimbaCPP/DynamicLib/arm_64bit/ 
 *				-lVimbaCPP -lusb-1.0 -pthread -o vimbaserver
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
#include <algorithm>
#include <vector>
#include <queue>

// string
#include "string.h"
#include <string>

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
 
 
using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;

/*****************************************************************************/
// Class with global definitions
/*****************************************************************************/

#include "global.h"
// only one object, ever.
// only the main thread has write access.
CGlobal	global;

#include "camserver.h"
#include "vimba_capture.h"

// usb reset function (see usb.cpp)
void	avt_usb_reset();




/*****************************************************************************/
// main
/*****************************************************************************/

// maximum attempts to find the cameras before new reset
#define	VIMBA_MAX_ATTEMPTS	10

int main(int argc, char const *argv[]){
	// ignore a "broken pipe" signal; we catch tcp/ip errors via timeouts
	signal(SIGPIPE, SIG_IGN);

	// vimba. only the main thread is allowed to access
	CVimba*			vimba					= new CVimba();


	// list of cameras 
	std::vector<CameraPtr>				apicamera_list;
	std::vector<CVimbaCamera*>			camera_list;

	// list of streams
	std::vector<CMyVimbaStream*>		vimba_stream_list;



	/*************************************************************************/
	// welcome
	/*************************************************************************/
	global.stdout_lock.lock();
	cout << endl;
	cout << "===== welcome =====" << endl;
	cout << "I hope you are having a great day." << endl;
	fflush(stdout);
	global.stdout_lock.unlock();

	global.stdout_lock.lock();
	cout << "init vimba" << endl;
	fflush(stdout);
	global.stdout_lock.unlock();
	vimba->init();	
	
	/*************************************************************************/
	// Camera list
	/*************************************************************************/
	global.global_lock.lock();
	global.camera_id_list.push_back("DEV_1AB22C011FE5");
	global.camera_id_list.push_back("DEV_1AB22C011FE8");
	global.global_lock.unlock();
	

	/*************************************************************************/
	// Parse command line arguments		
	/*************************************************************************/
	int	control_port		= DEFAULT_SERVER_PORT;

	if(argc == 2){
		stringstream	port_str;
						port_str << argv[1];
						port_str >> control_port;
							
		//cout << "Network port set via command line: " << argv[1] << endl;
	}
	
	global.stdout_lock.lock();
	cout << "Control server port: " << control_port << endl;
	global.stdout_lock.unlock();
	cout << endl;
	

	/*************************************************************************/
	// Create a server and a camera for each camera
	/*************************************************************************/

	int	offset	= 1;
	global.global_lock.lock();
	for(auto item : global.camera_id_list){
		global.stdout_lock.lock();
		cout << "creating server thread for camera: " << item << endl;
		global.stdout_lock.unlock();

		// create a new server
		string			namestring	= "camserver " + item;
		CCamServer*		camserver	= new CCamServer(control_port+offset, namestring);
		// add the server to the list
		global.camserver_list.push_back(camserver);
		offset += 1;

		// create new camera object
		CVimbaCamera*	newcamera	= new CVimbaCamera();
		camera_list.push_back(newcamera);

	}
	global.global_lock.unlock();


	
	/*************************************************************************/
	// Main loop
	/*************************************************************************/
	while(true){
		global.stdout_lock.lock();
		cout << endl;
		cout << "===== main loop =====" << endl;
		global.stdout_lock.unlock();

		/*****************************************************/
		// clear all lists
		/*****************************************************/
		// clear the current camera list
		apicamera_list.clear();

		//while(true){
		//	// don't put this condition in while parentesis
		//	if(camera_list.empty() == true){
		//		break;
		//	}
		//	CVimbaCamera*	camera	= camera_list.back();
		//	camera_list.pop_back();						
		//	delete	camera;
		//}
		// clear the current stream list
		while(true){
			// don't put this condition in while parentesis
			if(vimba_stream_list.empty() == true){
				break;
			}
			CMyVimbaStream*	stream	= vimba_stream_list.back();
			vimba_stream_list.pop_back();						
			delete	stream;
		}

		global.stdout_lock.lock();
		cout << "all lists are empty" << endl;
		global.stdout_lock.unlock();


		/*****************************************************/
		// camera reset loop
		/*****************************************************/
		while(true){
			// shutdown vimba
			//try{
			//	vimba->shutdown();
			//}catch(...){
			//	cout << "vimba shutdown failed" << endl;
			//}
			/*****************************************************/
			// start with USB reset
			/*****************************************************/
			global.stdout_lock.lock();
			cout << endl;
			cout << "===== usb reset =====" << endl;
			global.stdout_lock.unlock();
			avt_usb_reset();
			this_thread::sleep_for(1000ms);

			/*****************************************************/
			// reset camera via vimba
			/*****************************************************/
			global.stdout_lock.lock();
			cout << endl;
			cout << "===== vimba reset =====" << endl;
			global.stdout_lock.unlock();
			
			
			global.stdout_lock.lock();
			cout << "shutdown vimba" << endl;
			fflush(stdout);
			global.stdout_lock.unlock();

			vimba->shutdown();
			this_thread::sleep_for(1000ms);

			global.stdout_lock.lock();
			cout << "init vimba" << endl;
			fflush(stdout);
			global.stdout_lock.unlock();

			vimba->init();

			try{
				// iterate through all cameras
				int camera_count	= vimba->get_camera_count();
				
				global.stdout_lock.lock();
				cout << "number of vimba cameras: " << camera_count << endl;
				cout << "reset cameras" << endl;
				global.stdout_lock.unlock();

				for(int i = 0; i < camera_count; i++){
					// get the fundamental camera object
					CameraPtr	apicamera	= vimba->get_apicamera(i);
					
					/*****************************************************/
					// send reset command via vimba
					/*****************************************************/
					//
					// we use only low-level commands here to avoid 
					// conflicts with the high-level code later
					//
					VmbErrorType    err;
					FeaturePtr		vimba_feature;

					// open the camera
					err 				= apicamera->Open(VmbAccessModeFull); 
					if(err != VmbErrorSuccess){
						perror ("DeviceReset failed: Open VmbAccessModeFull");
						throw -1;
					}

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

					// close the camera
					err 				= apicamera->Close();; 
					if(err != VmbErrorSuccess){
						perror ("DeviceReset failed: Close");
						throw -1;
					}
					
					global.stdout_lock.lock();
					cout << "executed DeviceReset for camera" << endl;
					global.stdout_lock.unlock();
					/*****************************************************/

				}
				
			}catch(...){
				global.stdout_lock.lock();
				cout << "error during vimba camera reset" << endl;
				global.stdout_lock.unlock();
			}

					
			/*****************************************************/
			// restart vimba until all cameras are discovered
			/*****************************************************/
			global.stdout_lock.lock();
			cout << endl;
			cout << "===== find cameras =====" << endl;
			global.stdout_lock.unlock();


			bool	ready_to_stream	= false;
			for(int attempt = 0; attempt < VIMBA_MAX_ATTEMPTS; attempt += 1){
				try{
					/*****************************************************/
					// reset vimba
					/*****************************************************/
					global.stdout_lock.lock();
					cout << endl;
					cout << "===== attempt " << attempt << " =====" << endl;
					fflush(stdout);
					global.stdout_lock.unlock();

					global.stdout_lock.lock();
					cout << "shutdown vimba" << endl;
					fflush(stdout);
					global.stdout_lock.unlock();

					vimba->shutdown();
					this_thread::sleep_for(1000ms);

					global.stdout_lock.lock();
					cout << "init vimba" << endl;
					fflush(stdout);
					global.stdout_lock.unlock();

					vimba->init();
					this_thread::sleep_for(1000ms);

					/*****************************************************/
					// test detected cameras
					/*****************************************************/
					// clear the current camera list
					apicamera_list.clear();

					int camera_count	= vimba->get_camera_count();

					cout << "number of vimba cameras: " << camera_count << endl;
					cout << "number of candidates:    " << global.camera_id_list.size() << endl;

					int		cameras_found	= 0;
					int		id_count		= global.camera_id_list.size();
					
					for(int i = 0; i < id_count; i++){
						string	search_camid_string		= global.camera_id_list[i];
						
						for(int j = 0; j < camera_count; j++){
							CameraPtr	current_apicamera	= vimba->get_apicamera(j);
							
							/*****************************************************/
							// get vimba camera ID
							/*****************************************************/
							//
							// we use only low-level commands here to avoid 
							// conflicts with the high-level code later
							//
							VmbErrorType    err;
							string			camid_string;
							// get camera ID
							err 		= current_apicamera->GetID(camid_string);
							if(err != VmbErrorSuccess){
								perror ("vimba camera GetID failed");
								throw -1;
							}

							/*****************************************************/

							if(search_camid_string	== camid_string){
								//camera found, add it to the list
								global.stdout_lock.lock();
								cout << "camera found: " << search_camid_string << endl; 
								global.stdout_lock.unlock();

								cameras_found		+= 1;
								apicamera_list.push_back(current_apicamera);
							}
						}				
					}
					/*****************************************************/
					// test if we could open all cameras
					/*****************************************************/					
					if(cameras_found == id_count){
						global.stdout_lock.lock();
						cout << "all cameras found." << endl; 
						global.stdout_lock.unlock();
						ready_to_stream	= true;
						break;
					}else{
						global.stdout_lock.lock();
						cout << "not all cameras found." << endl; 
						global.stdout_lock.unlock();
					}
					/*****************************************************/
					// do it again
					/*****************************************************/					
				}catch(...){
					cout << "error during vimba camera setup" << endl;
				}
				this_thread::sleep_for(1000ms);
			}// attempt for loop
			/*****************************************************/
			// test if we are ready to go on
			if(ready_to_stream == true){
				break;
			}
			
		this_thread::sleep_for(1000ms);
		}// reset while


		/*****************************************************/
		// ALL CAMERAS FOUND
		/*****************************************************/

		global.stdout_lock.lock();
		cout << endl;
		cout << "===== assign cameras =====" << endl;
		fflush(stdout);
		global.stdout_lock.unlock();
		
		for(int i = 0; i < apicamera_list.size(); i++){
			CameraPtr		apicamera	= apicamera_list[i];

			/*****************************************************/
			// get vimba camera ID
			/*****************************************************/
			//
			// we use only low-level commands here to avoid 
			// conflicts with the high-level code later
			//
			VmbErrorType    err;
			string			camid_string;
			// get camera ID
			err 		= apicamera->GetID(camid_string);
			if(err != VmbErrorSuccess){
				perror ("vimba camera GetID failed");
				throw -1;
			}

			/*****************************************************/
			// search for this string
			for(int j = 0; j < global.camera_id_list.size(); j++){
				string	current_camid_string	= global.camera_id_list[j];
				if(current_camid_string == camid_string){
					// pass it to the server
					global.camserver_list[j]->server_access_mutex.lock();
					global.camserver_list[j]->apicamera	= apicamera;
					global.camserver_list[j]->server_access_mutex.unlock();

					// high-level camera object
					camera_list[j]->init(apicamera);					

					global.stdout_lock.lock();
					cout << "assinged: " << camid_string << "\t" << j << "\t" << camera_list[j]->get_camID() << " server: " << global.camserver_list[j]->get_server_name() << endl; 
					global.stdout_lock.unlock();
					break;
				}
			}
		}
			
		// add it to the list


		//global.stdout_lock.lock();
		//cout << endl;
		//cout << "===== shutdown vimba =====" << endl;
		//global.stdout_lock.unlock();

		//vimba->shutdown();

		//continue;


		/*****************************************************/
		// Start streaming
		/*****************************************************/
		try{
			global.stdout_lock.lock();
			cout << endl;
			cout << "===== start streaming =====" << endl;
			fflush(stdout);
			global.stdout_lock.unlock();
			/*****************************************************/
			// put the server into "start request" status
			/*****************************************************/
			global.global_lock.lock();
			for(auto server : global.camserver_list){
				server->server_access_mutex.lock();
				server->server_status	= CAM_SERVER_START_REQUEST;
				server->server_access_mutex.unlock();
			}
			global.global_lock.unlock();
			/*****************************************************/
			// start streaming data
			/*****************************************************/


			for(int i = 0; i < camera_list.size(); i++){
				CVimbaCamera*	camera	= camera_list[i];

				/*****************************************************************/
				// open camera			
				/*****************************************************************/
				// get high-level access
				//camera->init(apicamera);

				global.stdout_lock.lock();
				cout << endl;
				cout << "===== stream " << camera->get_camID() << " =====" << endl;
				fflush(stdout);

				cout	<< "camera info: " << camera->get_camname() << '\t' << camera->get_cammodel() 
						<< '\t' << camera->get_camserial() << '\t' << camera->get_camintID() << endl;
				global.stdout_lock.unlock();

				/*****************************************************************/
				// print camera information			
				/*****************************************************************/
				// set pixel format
				camera->set_feature_value("PixelFormat", VmbPixelFormatMono12);
				VmbInt64_t	pixel_format				= camera->get_feature_value("PixelFormat");
				global.stdout_lock.lock();
				cout << "pixel format:                  " << pixel_format << endl;
				global.stdout_lock.unlock();
				
				// get connection speed
				// fake conversion (1024 would be proper) / (1000.0*1000.0)
				VmbInt64_t	speed						= camera->get_feature_value("DeviceLinkSpeed");
				global.stdout_lock.lock();
				cout << "DeviceLinkSpeed:               " << speed/(1000*1000) << " MByte/s" << endl; 			
				global.stdout_lock.unlock();

				// get connection speed
				VmbInt64_t	throughput_limit			= camera->get_feature_value("DeviceLinkThroughputLimit");
				global.stdout_lock.lock();
				cout << "DeviceLinkThroughputLimit:     " << throughput_limit/(1000*1000) << " MByte/s" << endl;
				global.stdout_lock.unlock();

				throughput_limit						= (VmbUint64_t)(60*1000*1000);
				camera->set_feature_value("DeviceLinkThroughputLimit", throughput_limit);
				throughput_limit						= camera->get_feature_value("DeviceLinkThroughputLimit");
				global.stdout_lock.lock();
				cout << "DeviceLinkThroughputLimit:     " << throughput_limit/(1000*1000) << " MByte/s" << endl;
				global.stdout_lock.unlock();


				// get image properties
				double	exposure_time					= camera->get_feature_value_double("ExposureTime");
				global.stdout_lock.lock();
				cout << "ExposureTime:                  " << exposure_time  << " microseconds " << endl;
				global.stdout_lock.unlock();



								
				// get image properties
				VmbInt64_t 	image_height				= camera->get_feature_value("Height");
				global.stdout_lock.lock();
				cout << "height:                        " << image_height  << " pixel " << endl;
				global.stdout_lock.unlock();

				VmbInt64_t 	image_width					= camera->get_feature_value("Width");
				global.stdout_lock.lock();
				cout << "width:                         " << image_width  << " pixel " << endl;
				global.stdout_lock.unlock();

				// get payload size
				VmbInt64_t 	payload_size				= camera->get_feature_value("PayloadSize");
				global.stdout_lock.lock();
				cout << "Payload size:                  " << payload_size  << "\t" << image_height * image_width * 2  << " Byte " << endl;
				global.stdout_lock.unlock();
					
				/*****************************************************************/
				// Create stream
				/*****************************************************************/
				// frame buffer for asynchronous image capturing
				global.stdout_lock.lock();
				cout << endl << "create camera stream" << endl;
				global.stdout_lock.unlock();
				
				CMyVimbaStream*		stream	= new CMyVimbaStream(camera, VIMBA_BUFFER_LENGTH);
				stream->start_streaming();
				vimba_stream_list.push_back(stream);
		
				
	
				global.stdout_lock.lock();
				cout << "capturing started" << endl << endl;
				global.stdout_lock.unlock();
				/*****************************************************************/

			}// cameras are capturing now

			/*****************************************************************/
			// wait for error	
			/*****************************************************************/
			while(true){
				global.stdout_lock.lock();
				cout << "main: error test" << endl;
				global.stdout_lock.unlock();
				this_thread::sleep_for(1000ms);
				
				
				int		server_status_copy		= 0;
				bool	reset					= false;
				/*****************************************************************/
				// test the status of each server
				/*****************************************************************/
				for(auto server : global.camserver_list){
					server->server_access_mutex.lock();
					server_status_copy	=	server->server_status;
					server->server_access_mutex.unlock();
					if(server_status_copy == CAM_SERVER_ERROR){
						global.stdout_lock.lock();
						cout << "Main: server error detected, reset " << server->get_server_name() << endl;
						fflush(stdout);
						global.stdout_lock.unlock();
						reset	= true;
					}
				}
				/*****************************************************************/
				if(reset == true){
					/*****************************************************************/
					// set status to error for all server
					/*****************************************************************/
					for(auto server : global.camserver_list){
						server->server_access_mutex.lock();
						server->server_status	= CAM_SERVER_ERROR;
						server->server_access_mutex.unlock();
					}
					break;
					/*****************************************************************/
				}
				/*****************************************************************/
			}
			/*****************************************************************/
			// error occured, stop streaming
			/*****************************************************************/
			global.stdout_lock.lock();
			cout << "capturing stopped" << endl << endl;
			global.stdout_lock.unlock();

			while(true){
				// don't put this condition in while parentesis
				if(vimba_stream_list.empty() == true){
					break;
				}
				auto	stream		= vimba_stream_list.back();
				vimba_stream_list.pop_back();
				try{
					stream->stop_streaming();				
				}catch(...){
					global.stdout_lock.lock();
					cout << "stop_streaming failed" << endl << endl;
					global.stdout_lock.unlock();
				}					
				global.stdout_lock.lock();
				cout << "delete stream" << endl << endl;
				global.stdout_lock.unlock();
				try{
					delete	stream;
				}catch(...){
					global.stdout_lock.lock();
					cout << "delete stream failed" << endl << endl;
					global.stdout_lock.unlock();
				}					
			}



			/*
			camera_count	= vimba->get_camera_count();
			for(int i = 0; i < camera_count; i++){
				camera						= camera_list[i];
				apicamera					= camera->get_apicamera();


				// stop the acquisition
				camera->run_command("AcquisitionStop");
				//cout	<< "acquisition stopped" << endl;
				apicamera->EndCapture();
				//cout	<< "EndCapture" << endl;
				apicamera->FlushQueue();
				//cout	<< "FlushQueue" << endl;
				apicamera->RevokeAllFrames();
				//cout	<< "RevokeAllFrames" << endl;

				// unregister the observer
				for(auto frame : camera->frame_buffer){
					frame->UnregisterObserver();
				}	
			}
			*/
			/*****************************************************************/
			// shutdown vimba
			/*****************************************************************/
			// shutdown vimba
			//try{
			//	vimba->shutdown();
			//}catch(...){
			//	cout << "vimba shutdown failed" << endl;
			//}
			/*****************************************************************/


		}catch(...){
		
		}
		this_thread::sleep_for(5000ms);
	}// main loop
	/*************************************************************************/

	delete	vimba;
	
	return	0;
} 
/*****************************************************************************/
 
