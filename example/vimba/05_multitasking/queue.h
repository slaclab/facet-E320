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
#ifndef __QUEUE_H__
#define __QUEUE_H__


// time
#include <chrono>
#include <ctime>

// multi-threading
#include <thread>
#include <mutex>



using namespace std;

/*****************************************************************************/
// CQueue
/*****************************************************************************/
//
//	provides thread-save access to the std c++ queue
//

template	<class T>
class	CQueue{

	public:
	// constructor
	CQueue();

	// get number of elements
	int				get_size();

	// tests if empty
	bool			is_empty();

	// gets the first element and removes it
	T				pop();

	// pushes a new element to the end of the queue
	void			push(T element);

	private:
		mutex		access_mutex;
		queue<T>	stdqueue;

};
/*****************************************************************************/
// Constructor
/*****************************************************************************/
template	<class T>
CQueue<T>::CQueue(){

}

/*****************************************************************************/
// get_size
/*****************************************************************************/
template	<class T>
int	CQueue<T>::get_size(){

	int	size_copy;

	this->access_mutex.lock();
	size_copy		= stdqueue.size();
	this->access_mutex.unlock();

	return	size_copy;
}

/*****************************************************************************/
// is_empty
/*****************************************************************************/

template	<class T>
bool	CQueue<T>::is_empty(){

	bool	status;

	this->access_mutex.lock();
	status		= stdqueue.empty();
	this->access_mutex.unlock();

	return	status;
}

/*****************************************************************************/
// pop
/*****************************************************************************/

template	<class T>
T	CQueue<T>::pop(){

	T		element;

	bool	isempty		= false;

	this->access_mutex.lock();
	isempty		= stdqueue.empty();
	if(isempty	== false){
		element		= stdqueue.front();
		stdqueue.pop();
	}
	this->access_mutex.unlock();

	if(isempty == true){
		perror("queue was empty");
		throw	-1;
	}
	
	return	element;

}

/*****************************************************************************/
// push
/*****************************************************************************/

template	<class T>
void	CQueue<T>::push(T element){


	this->access_mutex.lock();
	stdqueue.push(element);
	this->access_mutex.unlock();

}

/*****************************************************************************/
#endif
