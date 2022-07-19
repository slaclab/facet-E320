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

#include <unistd.h>

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


// usb
#include <libusb-1.0/libusb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/usbdevice_fs.h>


using namespace std;

// Allied Vision devices
#define	AVT_DEV_ID			"1ab2:0001"

void	avt_usb_reset(){
	/*************************************************************************/
	// Reset AVT usb devices	
	/*************************************************************************/
	/*
	  Reset the USB camera; libusb:
	  https://www.dreamincode.net/forums/topic/148707-introduction-to-using-libusb-10/
	 */
	 
	libusb_device**					usb_dev_list;	//libusb device list
	libusb_device*					usb_device;		//libusb device 	
	vector<libusb_device_handle*>	avt_dev_list;	// all AVT devices	

	libusb_context*	usb_ctx 		= NULL; 		//libusb session information	
	if(libusb_init(&usb_ctx) < 0){
		perror("libusb init error");
		throw -1;
	}

	int devcount	= libusb_get_device_list(NULL, &usb_dev_list);
	if(devcount < 0){
		perror("libusb get device list error");
		throw -1;
	}
	/* No error handling from now on, in order to properly free/exit libusb.
	   This should be improved in the future.
	 */
	printf("Number of usb devices (vendor:device): %i\n", devcount);
	libusb_device_descriptor usb_dev_desc	= {0};
	char dev_id_string [64];
		
	for (int i = 0; i < devcount; i++){
		usb_device	= usb_dev_list[i];
		if(libusb_get_device_descriptor(usb_device, &usb_dev_desc) < 0){
			perror("libusb get device descriptor error");
			throw -1;
		}
		sprintf (dev_id_string, "%04x:%04x", usb_dev_desc.idVendor, usb_dev_desc.idProduct);
		printf("\t%s", dev_id_string);
		if(strcmp(dev_id_string, AVT_DEV_ID) == 0){
			printf(" AVT device\n");
			libusb_device_handle*	avtdevice;
			avtdevice	= libusb_open_device_with_vid_pid(usb_ctx, usb_dev_desc.idVendor, usb_dev_desc.idProduct);
			avt_dev_list.push_back(avtdevice);
		}else{
			printf("\n");
		}
	}
	while(avt_dev_list.empty() == false){
		libusb_device_handle*	avtdevice;
		avtdevice				= avt_dev_list.back();
		avt_dev_list.pop_back();
				
		libusb_reset_device(avtdevice);
		libusb_release_interface(avtdevice, 0);
		printf("device has been reset.\n");			
		sleep(0.1);
	}

	libusb_free_device_list(usb_dev_list, 1);
	libusb_exit(usb_ctx);
	//libusb_reset_device	

}	
	
	
	
