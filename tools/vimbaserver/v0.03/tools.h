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
#ifndef __TOOLS_H__
#define __TOOLS_H__

// time
#include <chrono>
#include <ctime>
#include <iomanip>


using namespace std;
using namespace std::chrono;


string buffer_to_string(char* buffer, int buffer_length);

string	get_current_date_time_string();

/*****************************************************************************/
// integer to buffer
/*****************************************************************************/

inline void	int32_to_buffer(uint8_t* buffer, int offset, uint32_t value)
{
	buffer[offset+0]	=	(value >> 0)  & 0xFF;
	buffer[offset+1]	=	(value >> 8)  & 0xFF;
	buffer[offset+2]	=	(value >> 16) & 0xFF;
	buffer[offset+3]	=	(value >> 24) & 0xFF;	
}

inline void	int64_to_buffer(uint8_t* buffer, int offset, uint64_t value)
{
	buffer[offset+0]	=	(value >> 0)  & 0xFF;
	buffer[offset+1]	=	(value >> 8)  & 0xFF;
	buffer[offset+2]	=	(value >> 16) & 0xFF;
	buffer[offset+3]	=	(value >> 24) & 0xFF;	
	buffer[offset+4]	=	(value >> 32) & 0xFF;
	buffer[offset+5]	=	(value >> 40) & 0xFF;
	buffer[offset+6]	=	(value >> 48) & 0xFF;
	buffer[offset+7]	=	(value >> 56) & 0xFF;	

}

/*****************************************************************************/
#endif
