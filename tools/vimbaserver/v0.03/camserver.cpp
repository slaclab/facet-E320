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

#include "camserver.h"


/******************************************************************************
 * Image class
 *****************************************************************************/
/*
 * The vimba callback function creates an image of this type and does a 
 * memcpy of the data. Then, this image is passed to the tcp/ip server
 * to send it over the network.
 *
 */
 
/*********************
 * Constructor
 *********************/
CByteImage::CByteImage(uint32_t width, uint32_t height, uint32_t bytesperpixel){
	this->width				= width;
	this->height			= height;
	this->bytesperpixel		= bytesperpixel;
	this->data_length		= width*height*bytesperpixel;
	data					= new uint8_t[data_length];
}

/*********************
 * Destructor
 *********************/
CByteImage::~CByteImage(){
	delete	data;
}



