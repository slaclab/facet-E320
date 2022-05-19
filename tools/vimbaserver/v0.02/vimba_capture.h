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

#ifndef __VIMBA_CAPTURE_H__
#define __VIMBA_CAPTURE_H__

/*****************************************************************************/
// vimba callback class: a new image was captured
/*****************************************************************************/
// Call-back class: is called when a frame is captured
class MyObserver : public CVimbaObserver {

	public:	
	MyObserver(CameraPtr apicamera);
		
	void	FrameReceived (const FramePtr frame);

	private:
	
		// used to find the camera ID
		CameraPtr		apicamera;
		CVimbaCamera	camera;
		// device id
		string			camid;
		
		// associated camera server
		CCamServer*		camserver;	
	
};
/*****************************************************************************/
#endif
