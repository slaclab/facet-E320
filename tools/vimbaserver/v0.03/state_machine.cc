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

#include	"state_machine.h"

/*****************************************************************************/
// constructor
/*****************************************************************************/
CStateMachine::CStateMachine(int state){

	this->state			= state;
	this->blocked		= false;

}

/*****************************************************************************/
// get the current state, block machine
/*****************************************************************************/
int CStateMachine::get_state_blocking(){
	int		state_copy;
	bool	blocked_copy;

	while(true){
		this->access_mutex.lock();
		state_copy		= this->state;
		blocked_copy	= this->blocked;
		this->blocked	= true;
		this->access_mutex.unlock();
	
		// we can return the state
		if(blocked_copy == false){
			return	state_copy;
		}
		// we need to wait until it is unblocked
		this_thread::sleep_for(1ms);
	}	
}

/*****************************************************************************/
// get the current state
/*****************************************************************************/
int CStateMachine::get_state(){
	int		state_copy;
	bool	blocked_copy;

	while(true){
		this->access_mutex.lock();
		state_copy		= this->state;
		blocked_copy	= this->blocked;
		this->access_mutex.unlock();
	
		// we can return the state
		if(blocked_copy == false){
			return	state_copy;
		}
		// we need to wait until it is unblocked
		this_thread::sleep_for(1ms);
	}	
}

/*****************************************************************************/
// get the current state
/*****************************************************************************/
void CStateMachine::set_state(int state){

	this->access_mutex.lock();
	this->state		= state;
	this->blocked	= false;
	this->access_mutex.unlock();
}

/*****************************************************************************/
// block the state machine
/*****************************************************************************/
//
void CStateMachine::block(){

	this->access_mutex.lock();
	this->blocked	= true;
	this->access_mutex.unlock();
}

/*****************************************************************************/
// unblock the state machine
/*****************************************************************************/
//
void CStateMachine::unblock(){

	this->access_mutex.lock();
	this->blocked	= false;
	this->access_mutex.unlock();
}

/*****************************************************************************/
// unblock the state machine
/*****************************************************************************/
//
// for error handling only. main operation is with get/set
//
bool CStateMachine::isblocked(){
	bool	blocked_copy;

	this->access_mutex.lock();
	blocked_copy		= this->blocked;
	this->access_mutex.unlock();

	return	blocked_copy;
}


/*****************************************************************************/










