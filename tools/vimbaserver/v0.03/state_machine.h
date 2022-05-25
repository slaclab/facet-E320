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
#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__


// time
#include <chrono>
#include <ctime>

// multi-threading
#include <thread>
#include <mutex>



using namespace std;

//#define	STATE_MACHINE_ALERT_TIMEOUT

/*****************************************************************************/
// CStateMachine
/*****************************************************************************/
//
//	a call to "get_state" blocks the state machine, set_state unblocks.
//

class	CStateMachine{

	public:
	// constructor
	CStateMachine(int state);

	// get the current state, block state machine
	int				get_state_blocking();


	// get the current state
	int				get_state();

	// sets the current state
	void			set_state(int state);

	// block
	void			block();


	// releases the lock
	void			unblock();

	// returnes status
	bool			isblocked();


	private:
		bool		blocked;
		int			state;
	
		mutex		access_mutex;
};
/*****************************************************************************/
#endif
