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


#ifndef __GLOBAL_H__
#define __GLOBAL_H__


// tcp/ip port for the control server. The cameras are streamed on subsequent
// port numbers
#define 	DEFAULT_SERVER_PORT				42000

// number of frames in the queue
#define		NUMBER_OF_FRAMES_IN_BUFFER 		10
// maximum attempts
#define		CAMERA_MAX_ATTEMPTS				5

// cam server queue length
#define		CAM_SERVER_QUEUE_LENGTH			1


/*****************************************************************************/ 
#endif
