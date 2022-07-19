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

// thread-save queue
#include "queue.h"

// small helper functions
#include "tools.h"

using namespace std;
using namespace AVT;
using namespace VmbAPI;
using namespace std::chrono;


#include	"global.h"
/*****************************************************************************/
// vimba
/*****************************************************************************/
#include "vimba.h"

// main vimba instance, see "vimba.h"
CVimba		global_vimba;
mutex		CVimba::access_mutex;

/*****************************************************************************/
// server
/*****************************************************************************/
#include "server.h"
#include "ctr_server.h"

using 		CMyControlServer	= CControlServer<10>;



/*****************************************************************************/
// xml parsing: config file
/*****************************************************************************/
#include "pugixml.hpp"
pugi::xml_document 			xmlconfig;
mutex						xmlconfig_mutex;


/*****************************************************************************/
// main
/*****************************************************************************/
void	camera_streaming_main(string cameraID, int server_port);

int main(int argc, char const *argv[]){
	
	// camera list
	vector<string>	camID_list;
	//camID_list.push_back("DEV_1AB22C011FE5");
	//camID_list.push_back("DEV_1AB22C011FE8");
	//camID_list.push_back("DEV_1AB22C014125");

	/***********************************************/
	// welcome
	/***********************************************/
	cout << endl;
	cout << "===== welcome =====" << endl;
	cout << "I hope you are having a great day." << endl;
	cout << endl;	

	/***********************************************/
	// starting vimba
	/***********************************************/
	cout << endl;
	cout << "===== vimba =====" << endl;
	try{			
		global_vimba.startup();
		// read camera list from vimba
		global_vimba.update_camera_list();
		// get camera count
	}catch(...){
		cerr << "error during startup" << endl;
	}
		
	int		camera_count	= global_vimba.get_camera_count();
	cout	<< "number of cameras detected: " << camera_count << endl;

	for (int camera_index = 0; camera_index < camera_count; camera_index++){
		CameraPtr		apicamera				= global_vimba.get_apicamera(camera_index);
		// high-level vimba camera access
		CVimbaCamera	camera(&global_vimba);
		camera.open(apicamera);
		camera.get_info();
		cout << "camera " << camera_index << ": " << camera.get_camID() << endl;
		
	}



	/***********************************************/
	// load xml config file
	/***********************************************/
	cout << endl;
	cout << "===== config.xml =====" << endl;
	cout	<< "reading config.xml to get camera information" << endl;
	pugi::xml_parse_result		result	= xmlconfig.load_file("config.xml");
    if (result != 1){
    	cerr << "pugi_xmldoc.load_file error" << result << endl;
        return -1;
    }
	/***********************************************/
	xmlconfig_mutex.lock();
    int	i = 0;
	for (pugi::xml_node camera : xmlconfig.child("config").children("camera"))
	{
		string	camID		= camera.attribute("id").as_string();
		camID_list.push_back(camID);
		cout << i << " camera id: " << camID << endl;
		i++;
	}
	cout << endl;	
	xmlconfig_mutex.unlock();
	/***********************************************/
	// Parse command line arguments		
	/***********************************************/
	cout << endl;
	cout << "===== control server =====" << endl;
	int	control_port		= DEFAULT_SERVER_PORT;

	if(argc == 2){
		stringstream	port_str;
						port_str << argv[1];
						port_str >> control_port;
	}
	
	cout << "Control server port: " << control_port << endl;
	cout << endl;

	// servers
	const string			ctrlname			= "CTRL_SERV";
	CMyControlServer*		ctrlserv			= new CMyControlServer(control_port, ctrlname);
	// start the thread
	thread					ctrlserv_thread(&CMyControlServer::execute, ctrlserv);




	/*****************************************************************/
	// start the camera threads
	/*****************************************************************/	
	cout << endl;
	cout << "===== camera threads =====" << endl;

	vector<thread>	thread_list;
		
	for (int camera_index = 0; camera_index < camID_list.size(); camera_index++){
		thread		newthread(camera_streaming_main, camID_list[camera_index], control_port + camera_index + 1);
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
 
