/*
 *  This file is part of RTViewer.
 *
 *	copyright (c) 2011  Jan Rinze Peterzon (janrinze@gmail.com)
 *
 *  RTViewer is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RTViewer.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <cstring>
#include <cmath>
#include "LUT.h"
// reset all LUT data to 0
template<typename T> void LUT<T>::clear(void) {
	if (size > 0 && data)
		memset(data, 0, size * sizeof(T));
}

// creator for LUT with size and optional flags
template<typename T> LUT<T>::LUT(int s, int flags) {
	clip = flags;
	data = new T[s];
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;

	if (flags & LUT_INIT_CLEAR)
		clear();
}
/*
 template<typename T>  LUT<T>::LUT(int s, T (*fn)(float),int flags ) {
 clip = flags;
 data = new T[s];
 float step = 1.0f/(float)s;
 float index = 0.0;
 for (int i=0;i<s;i++)
 {
 index=step*i;
 data[i]=fn(index);

 }
 owner = 1;
 size = s;
 std::cout << "LUT init\n";
 for (int i=0;i<s;i+=s/8)
 std::cout << "LUT i=" << i << " = " << data[i] << "\n";
 //if (flags&LUT_INIT_CLEAR) clear();
 }
 */
// copy operator
template<typename T> LUT<T>::LUT(const LUT<T>& copy) {
	data = new T[copy.size];
	clip = copy.clip;
	owner = 1;
	memcpy(data, copy.data, copy.size * sizeof(T));
	size = copy.size;
	max=(size>0)?size-1:0;

}

// call to setup lut size and flags
template<typename T> void LUT<T>::operator ()(int s, int flags) {
	if (owner && data)
		delete[] data;
	clip = flags;
	data = new T[s];
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;

	if (flags & LUT_INIT_CLEAR)
		clear();
}
template<typename T> LUT<T>::LUT(unsigned int s, T(*func)(float), float scale,
		int flags) {
	fn=func;
	clip = flags | LUT_SCALE_FLOAT;
	if (size < s) {
		delete[] data;
		data = new T[s];
	}
	fscale = (float) s / scale;
	float step = scale / (float) s;
	float index = 0.0;
	for (unsigned int i = 0; i < s; i++) {
		data[i] = fn(step*(float)i);
	}
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;
}
template<typename T> LUT<T>::LUT(unsigned int s, T (*func)(float,float*), float * parms, float scale,
		int flags) {
	fp=func;params=parms;
	clip = flags | LUT_SCALE_FLOAT;
	if (size < s) {
		delete[] data;
		data = new T[s];
	}
	fscale = (float) s / scale;
	float step = scale / (float) s;
	float index = 0.0;
	for (unsigned int i = 0; i < s; i++) {
		data[i] = fp(step*(float)i,params);
	}
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;
}
template<typename T> void LUT<T>::operator ()(unsigned int s, T(*func)(float),
		float scale, int flags) {
	fn=func;
	clip = flags | LUT_SCALE_FLOAT;
	if (size < s) {
		delete[] data;
		data = new T[s];
	}
	fscale = (float) s / scale;
	float step = scale / (float) s;
	float index = 0.0;
	for (unsigned int i = 0; i < s; i++) {
		data[i] = fn(step*(float)i);
	}
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;

	//if (flags&LUT_INIT_CLEAR) clear();
}
template<typename T> void LUT<T>::operator ()(unsigned int s, T (*func)(float,float*), float * parms, float scale,
		int flags) {
	fp=func;params=parms;
	clip = flags | LUT_SCALE_FLOAT;
	if (size < s) {
		delete[] data;
		data = new T[s];
	}
	fscale = (float) s / scale;
	float step = scale / (float) s;
	float index = 0.0;
	for (unsigned int i = 0; i < s; i++) {
		data[i] = fp(step*(float)i,params);
	}
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;
}
// initialize LUT with data from an array
template<typename T> LUT<T>::LUT(int s, T * source) {
	data = new T[s];
	owner = 1;
	size = s;
	max=(size>0)?size-1:0;

	for (int i = 0; i < s; i++) {
		data[i] = source[i];
	}
}

// default creator with no size
template<typename T> LUT<T>::LUT(void) {
	data = NULL;
	owner = 1;
	size = 0;
	max=0;
}

// default destructor
template<typename T> LUT<T>::~LUT() {
	if (owner)
		delete[] data;
}

// assignment operator
template<typename T> LUT<T> & LUT<T>::operator=(const LUT<T> &rhs) {
	if (this != &rhs) {
		if (rhs.size > this->size) {
			delete[] this->data;
			this->data = NULL;
		}
		if (this->data == NULL)
			this->data = new T[rhs.size];
		this->clip = rhs.clip;
		this->owner = 1;
		memcpy(this->data, rhs.data, rhs.size * sizeof(T));
		this->size = rhs.size;
		this->max=(this->size>0)?this->size-1:0;


	}

	return *this;
}

template class LUT<float> ;
template class LUT<int> ;
template class LUT<unsigned int> ;
template class LUT<unsigned char> ;

