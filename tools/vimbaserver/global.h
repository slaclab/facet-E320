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
#include "vimba.h"
#include "vimba_stream.h"
#include "server.h"

#include <vector>
#include <queue>

#ifndef __GLOBAL_H__
#define __GLOBAL_H__


/*****************************************************************************/
// vimba
/*****************************************************************************/

class MyObserver;


template <int queue_length> class CServer;
class CCamServer;

// define the number of frames the vimba buffer contains
#define		VIMBA_BUFFER_LENGTH		3

using CMyVimbaStream	= CVimbaStream<MyObserver>;
//using CMyVimba	= CVimba<MyObserver,VIMBA_BUFFER_LENGTH>;

#define		DATA_SERVER_QUEUE_LENGTH		1
using CDataServer	= CServer<DATA_SERVER_QUEUE_LENGTH>;

/*****************************************************************************/
// tcp/ip server
/*****************************************************************************/

#define DEFAULT_SERVER_PORT				42000

/*****************************************************************************/
// global definitions
/*****************************************************************************/

class	CGlobal{
	public:
	CGlobal();
	// lock this for reading global
	mutex						global_lock;

	// lock this for accessing stdout
	mutex						stdout_lock;

	// id strings for each camera
	std::vector<string>			camera_id_list;

	// server for each camera
	std::vector<CCamServer*>	camserver_list;

};


inline
CGlobal::CGlobal(){

}

extern CGlobal global;

/*****************************************************************************/
#endif
