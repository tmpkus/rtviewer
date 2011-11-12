/*
 * LUT.h
 *
 *  This file is part of DWMviewport.
 *
 *  Copyright (c) 2011 Jan Rinze Peterzon (janrinze@gmail.com)
 *
 */


/*
 *  Declaration of flexible Lookup Tables
 *
 *  Usage:
 *
 *  	LUT<type> name (size);
 *		LUT<type> name (size, flags);
 *
 *		creates an array which is valid within the normal C/C++ scope "{ ... }"
 *
 *      access to elements is a simple as:
 *
 *      	LUT<float> my_lut (10);
 *      	float value = my_lut[3];
 *          float value = my_lut[2.5]; // this will interpolate
 *
 *      when using a float type index it will interpolate the lookup values
 *
 *      extra setting in flags: (clipping is set by default)
 *      LUT_CLIP_ABOVE
 *      LUT_CLIP_BELOW
 *
 *      example:
 *      	LUT<float> my_lut (10,LUT_CLIP_BELOW);
 *          float value = my_lut[22.5];  // this will extrapolate
 *          float value = my_lut[-22.5]; // this will not extrapolate
 *
 *          LUT<float> my_lut (10,0); // this will extrapolate on either side
 *
 *      shotcuts:
 *
 *      	LUTf stands for LUT<float>
 *          LUTi stands for LUT<int>
 *          LUTu stands for LUT<unsigned int>
 */

#ifndef LUT_H_
#define LUT_H_

// bit representations of flags
#define LUT_CLIP_BELOW 1
#define LUT_CLIP_ABOVE 2
#define LUT_INIT_CLEAR 4
#define LUT_SCALE_FLOAT 8

#define LUT_DEFAULT_FL LUT_CLIP_BELOW|LUT_CLIP_ABOVE

#define LUTf LUT<float>
#define LUTi LUT<int>
#define LUTu LUT<unsigned int>

template<typename T>
class LUT {
private:
	// list of variables ordered to improve cache speed

	T * data;
	unsigned int clip, size,max, owner;
	T (*fn)(float);
	float fscale;
public:

	// reset all LUT data to 0
	void clear(void);
	// creator for LUT with size and optional flags
	LUT(int s, int flags = LUT_DEFAULT_FL);
//	LUT(int s, T (*fn)(float),int flags = LUT_DEFAULT_FL);
	LUT(unsigned int s, T (*fn)(float),float scale=1.0f,int flags = LUT_DEFAULT_FL);
	// copy operator
	explicit LUT(const LUT<T>& copy);
	// call to setup lut size and flags
	void operator ()(int s, int flags = LUT_DEFAULT_FL);
	void operator ()(unsigned int s, T (*fn)(float),float scale=1.0f,int flags = LUT_DEFAULT_FL);
	// initialize LUT with data from an array
	LUT(int s, T * source);
	// default creator with no size
	LUT(void);
	// default destructor
	~LUT();
	// assignment operator
	LUT<T> & operator=(const LUT<T> &rhs);
	// index operator for use with integer indices
	T& operator[](int index)
	{
		if (((unsigned int)index)<size) return data[index];
		else
		{
			if (index < 0)
				return data[0];
			else
				return data[size - 1];
		}
	}
	// index operator for use with float indices
	T operator[](float &indx);
	// to check if LUT has been setup
	operator bool (void);
};

#endif /* LUT_H_ */
