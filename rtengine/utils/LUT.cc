/*
 * LUT.cc
 *
 *  Created on: Jun 13, 2011
 *      Author: janrinze
 */
#include <iostream>
#include <cstring>
#include <cmath>
#include "../include/LUT.h"
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
template<typename T> LUT<T>::LUT(unsigned int s, T(*fn)(float), float scale,
		int flags) {
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
	std::cout << "LUT init\n";
	 for (int i=0;i<s;i+=s/8)
	 std::cout << "LUT i=" << i << " = " << data[i] << "\n";
	//if (flags&LUT_INIT_CLEAR) clear();
}
template<typename T> void LUT<T>::operator ()(unsigned int s, T(*fn)(float),
		float scale, int flags) {
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
template<typename T> T LUT<T>::operator[](float &indx) {
	if (data==NULL) return (T) 0;
	float index = indx;
	if (clip & LUT_SCALE_FLOAT)
		index = index * fscale;
	int idx = floor(index);
	if ((unsigned int) idx >= max) {
		if (idx < 0) {
			if (clip & LUT_CLIP_BELOW)
				return data[0];
			if (fn)
				return fn(index);
			idx = 0;
		} else {
			if (clip & LUT_CLIP_ABOVE)
				return data[max];
			// use 2nd order derivative too.
			if (fn)
				return fn(index);
			idx = max;
		}
	}
	float diff = index-(float) idx;
	T p1 = data[idx];
	T p2 = data[idx + 1] - p1;
	return (p1 + p2 * diff);
}
template class LUT<float> ;
template class LUT<int> ;
template class LUT<unsigned int> ;
template class LUT<unsigned char> ;

