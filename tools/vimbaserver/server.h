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
#ifndef __SERVER_H__
#define __SERVER_H__

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <thread>
#include <mutex>


#include "global.h"
extern		CGlobal global;


using namespace std;

/*****************************************************************************/ 
// Definitions
/*****************************************************************************/

#define	SERVER_STATUS_RUNNING			0
#define	SERVER_STATUS_STARTING			10
#define	SERVER_STATUS_STOP_REQUESTED	20
#define	SERVER_STATUS_STOPPED			30
#define SERVER_STATUS_ERROR				-1

/*****************************************************************************/
// Generic server class
/*****************************************************************************/
template <int queue_length>
class CServer{
	public:
		CServer(int port, const string name);
		~CServer();

		// Overload to implement actual server code		
		virtual	void	main();
		int		get_status();
		void	stop();

		int		get_client_socket(){return	client_socket;};

		string	get_server_name(){return servername;};

	protected:
		string			servername;
		int				port;
		int				server_socket;
		int				server_socket_opt;
		int				client_socket;
		thread			serverthread;

		// server control lock
		mutex 			control_mutex;
		int				server_status;

		// thread main function
		static void 	server_main_internal(void* server);
		
		void			set_status(int new_status);
};

/*****************************************************************************/


/*********************
 * Stop server
 *********************/
template <int queue_length>
void	CServer<queue_length>::stop(void){
	//only stop the server if it is still running
	int	status	= this->get_status();

	if(status != SERVER_STATUS_STOPPED and status != SERVER_STATUS_ERROR){
		// indicate that we want to stop the server
		control_mutex.lock();
		this->server_status	= SERVER_STATUS_STOP_REQUESTED;
		control_mutex.unlock();

		cout << "requested to stop server" << endl;
		// wait for the server thread to finish
		this->serverthread.join();
	}
}


/*********************
 * Get server status
 *********************/
template <int queue_length>
int	CServer<queue_length>::get_status(void){

	int	server_status_copy;		

	this->control_mutex.lock();
	server_status_copy	= this->server_status;
	this->control_mutex.unlock();
	
	return	server_status_copy;

}


/*********************
 * Set server status
 *********************/
template <int queue_length>
void	CServer<queue_length>::set_status(int new_status){

	this->control_mutex.lock();
	this->server_status = new_status;
	this->control_mutex.unlock();

}


/*********************
 * Constructor
 *********************/
template <int queue_length>
CServer<queue_length>::CServer(int port, const string name){
	// start initializing
	this->control_mutex.lock();
		// socket initialization (such that close will work)
		this->server_socket		= 0;
		this->client_socket		= 0;
		
		// set name
		this->servername		= name;

		// set port
		this->port				= port;

		// server state machine
		this->server_status		= SERVER_STATUS_STARTING;		//we are starting the server

		// done initializing
	this->control_mutex.unlock();

	// start the server thread
	this->serverthread		= thread(&CServer::server_main_internal, this);

}


/*********************
 * Main server loop
 *********************/
template <int queue_length>
void CServer<queue_length>::server_main_internal(void* server_voidptr){
	CServer*	server	= (CServer*)server_voidptr;
	global.stdout_lock.lock();
	cout << "start " << server->servername << " on port: " << server->port << endl; 
	global.stdout_lock.unlock();


	try{
		// Open a socket
		server->server_socket 	= socket(AF_INET, SOCK_STREAM, 0);
		if (server->server_socket == 0){
			perror("creating socket failed");
			throw -1;
		}
		// Set properties
		server->server_socket_opt	= 1;
		if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &server->server_socket_opt, sizeof(server->server_socket_opt)) != 0){
			perror("setting options for socket failed");
			throw -1;
		}
		// bind to address
		struct	sockaddr_in	address;
		int 	addrlen 			= sizeof(address);

		address.sin_family			= AF_INET;
		address.sin_addr.s_addr		= INADDR_ANY;
		address.sin_port 			= htons(server->port);

		if (bind(server->server_socket, (struct sockaddr*)&address, sizeof(address)) < 0){
			perror("binding socket failed");
			throw -1;
		}
		// listen for incoming connection
		if (listen(server->server_socket, queue_length) < 0){
			perror("listening on port failed");
			throw -1;
		}
		global.stdout_lock.lock();
		cout << server->servername << " listening on port: " << server->port << endl; 
		global.stdout_lock.unlock();
	
		// server is not running
		server->control_mutex.lock();
			if(server->server_status == SERVER_STATUS_STARTING){
				server->server_status = SERVER_STATUS_RUNNING;
			}
		server->control_mutex.unlock();

		// Main loop
		while(true){
			//cout << "while" << endl;
			int				retval;
			struct timeval	tv;
			tv.tv_sec		= 1;
			tv.tv_usec		= 0;

			fd_set 			rfds;
			FD_ZERO(&rfds);
			FD_SET(server->server_socket, &rfds);
			retval	 		= select(server->server_socket + 1, &rfds, NULL, NULL, &tv);
			//cout << "select returned: " << retval << endl;
			if(retval == 1){
				// we accepted a connection
				server->client_socket  = accept(server->server_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
				// get the IP of the client:
				char ipstr[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &address.sin_addr, ipstr, INET_ADDRSTRLEN);

				global.stdout_lock.lock();
				cout << "=== " << server->servername << " accepted " << ipstr << " ===" << endl;
				global.stdout_lock.unlock();

				// call the main function of the server

				server->main();

				// close connection
				global.stdout_lock.lock();
				cout << "=== " << server->servername << " closed " << ipstr << " ===" << endl;
				global.stdout_lock.unlock();
				close(server->client_socket);			

			}
			// test if we should stop the server

			
			int	server_status	= server->get_status();		
			//cout << "status: " << server_status << endl;

			if(server_status == SERVER_STATUS_STOP_REQUESTED){
				cout << "stop server now" << endl;
				server->set_status(SERVER_STATUS_STOPPED);
				return;
			}
		}
	}
	catch(...){
		cout << "CServer error" << endl;
		fflush(stdout);
		server->set_status(SERVER_STATUS_ERROR);

		close(server->client_socket);
		close(server->server_socket);
		return;
	}

}

/*********************
 * Main function
 *********************/
template <int queue_length>
void CServer<queue_length>::main(){
			global.stdout_lock.lock();
			cout << "default main called" << endl; 
			global.stdout_lock.unlock();
}



/*********************
 * Destructor
 *********************/
template <int queue_length>
CServer<queue_length>::~CServer(void){
	// stop if it is not already stopped
	this->stop();

	// clean up	
	close(server_socket);
	close(client_socket);

	global.stdout_lock.lock();
	cout << "server " << this->servername << " deleted" << endl; 
	global.stdout_lock.unlock();

}
/*****************************************************************************/
#endif
