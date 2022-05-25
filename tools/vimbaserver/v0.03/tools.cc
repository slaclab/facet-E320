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


// time
#include <chrono>
#include <ctime>
#include <iomanip>


using namespace std;
using namespace std::chrono;


/*****************************************************************************/
// buffer to string
/*****************************************************************************/
string buffer_to_string(char* buffer, int buffer_length)
{
    string newstring(buffer, buffer_length);
    return newstring;
}


/*****************************************************************************/
// Get current date + time
/*****************************************************************************/
string	get_current_date_time_string(){

	// https://stackoverflow.com/questions/17223096/outputting-date-and-time-in-c-using-stdchrono
	stringstream		datetimestring;
	auto 				now 		= std::chrono::system_clock::now();
	auto 				now_time_t	= std::chrono::system_clock::to_time_t(now);
	datetimestring	<< put_time(localtime(&now_time_t), "%a %b %d %H:%M:%S, %Y");

	return	datetimestring.str();
}


/*****************************************************************************/

