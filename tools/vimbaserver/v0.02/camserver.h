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
#include	"global.h"

// string
#include "string.h"
#include <string>

// time
#include <chrono>
#include <ctime>

using namespace std;
using namespace std::chrono;

#ifndef __CAMSERVER_H__
#define __CAMSERVER_H__
/*****************************************************************************/
// Image class
/*****************************************************************************/
class CByteImage{
	public:
		CByteImage(uint32_t width, uint32_t height, uint32_t bytesperpixel);
		~CByteImage();

		uint32_t		width;
		uint32_t		height;
		uint32_t		bytesperpixel;
		
		uint8_t*		data;
		size_t			data_length;
};
/*****************************************************************************/
// CCamServer
/*****************************************************************************/


#define	CAM_SERVER_DEFAULT_QUEUE_LENGTH	3
#define	CAM_SERVER_TIMEOUT				3


// server has just been initialized, no camera yet. Wait for camera asignment.
#define	CAM_SERVER_INIT					0

// camera has been asigned, streaming shoud start.
// once the first frame arrives the buffer will be 
// reset and then streaming starts
#define	CAM_SERVER_START_REQUEST		10
// server is streaming
#define	CAM_SERVER_STREAMING			20
// server stopped streaming. Once a new
// connection is established, streaming continues
#define	CAM_SERVER_STOP					30


// communication has timed out, etc. we need to reset the server and the cameras
#define	CAM_SERVER_ERROR				-1


class CCamServer : public CDataServer{
	public:
		CCamServer(int port, const string name);
		void main();
		
		CameraPtr 						apicamera;
		int								port;
		
		// mutex to control access
		mutex							server_access_mutex;
		// state machine for the server
		int								server_status;

		// maximum image_queue length
		int								image_queue_maxlength;
		// buffer with new images from camera
		std::queue<CByteImage*>			image_queue_new;
		// images that have been processed and can be deleted
		std::queue<CByteImage*>			image_queue_old;

};
/*****************************************************************************/
#endif
