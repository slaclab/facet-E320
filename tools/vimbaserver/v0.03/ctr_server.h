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
#ifndef __CTR_SERVER_H__
#define __CTR_SERVER_H__


#include "tools.h"

#define	CONTR_BUFFER_SIZE 1024

/******************************************************************************
 * Control Server
 *****************************************************************************/
template <int queue_length>
class CControlServer : public CServer<CControlServer<queue_length>, queue_length>{
	public:
		CControlServer(int port, const string name);
		void main();
};

template <int queue_length>
CControlServer<queue_length>::CControlServer(int port, const string name) : CServer<CControlServer<queue_length>, queue_length>(port, name){
	cout	<< "CControlServer constructed" << endl;
}



template <int queue_length>
void CControlServer<queue_length>::main(){
	uint8_t 	recv_buffer[CONTR_BUFFER_SIZE] 		= {0};

	bool		restart_status						= true;

	int 		socket_status;
	socklen_t	socket_status_size					= sizeof(socket_status);

	while(true){
		// send command promt	
		cout << "control server main" << endl;

		//const string	welcome	= "camserver>";
		//send(this->client_socket, welcome.data(), welcome.length(), 0);


		/*****************************************************/
		// read message from remote client
		/*****************************************************/
		int	buffer_pos		= 0;
		while(true){
			this_thread::sleep_for(10ms);
			int		client_socket	= ((CServer<CControlServer<queue_length>, queue_length>*)this)->get_client_socket();

			// test if we are still running
			int	status	= this->get_status();
			if(status != SERVER_STATUS_RUNNING){
				return;
			}
			
			// read from the server
			ssize_t receive_count	= 0;
			receive_count			= recv(client_socket, &recv_buffer[buffer_pos], CONTR_BUFFER_SIZE-buffer_pos, MSG_DONTWAIT);
			if(receive_count > 0){
				cout << "received: " << receive_count << endl;
				buffer_pos	+= (receive_count-1);
				if(recv_buffer[buffer_pos] == '\n'){
					recv_buffer[buffer_pos] = 0;
					break;
				}
			}// received new data
		}// receive command loop

		/*****************************************************/
		// send message to remote client
		/*****************************************************/
		while(true){
			this_thread::sleep_for(10ms);
			int		client_socket	= ((CServer<CControlServer<queue_length>, queue_length>*)this)->get_client_socket();

			// test if we are still running
			int	status	= this->get_status();
			if(status != SERVER_STATUS_RUNNING){
				return;
			}
			
			// test socket status
			//getsockopt(client_socket, SOL_SOCKET, SO_ERROR, &socket_status, &socket_status_size);
			//cout	<< "socket: " << socket_status << endl;
			//sleep(1);
			
			// test if we got a message from main to send to remote client
			string	message		= buffer_to_string((char*)recv_buffer, buffer_pos) + "\n";

			send(client_socket, message.data(), message.length(), 0);

			break;
		}

		
	}//main loop
}

#endif
