/*
 *  This file is part of DWMviewport.
 *
 *  Copyright (c) 2011 Jan Rinze Peterzon (janrinze@gmail.com)
 *
 */
/*
 * image.h
 *
 *  Created on: Jun 11, 2011
 *      Author: janrinze
 */

#ifndef IMAGE_H_
#define IMAGE_H_
#include <string>
#include <math.h>
#include "utils/array2D.h"
#include "utils/LUT.h"
#include "improps.h"

class Lab {
public:
	float L, a, b;
	Lab & operator=(class argb8 & rhs) ;
	Lab & operator=(class rgbHDR & rhs);
	Lab & operator=(unsigned int & rhs);
	Lab  operator/(const class Lab&rhs)
	{
		Lab t;
		t.L=this->L/rhs.L;
		t.a=this->a/rhs.a;
		t.b=this->b/rhs.b;
		return t;
	}
	Lab  operator+(const class Lab&rhs)
	{
		Lab t;
		t.L=this->L+rhs.L;
		t.a=this->a+rhs.a;
		t.b=this->b+rhs.b;
		return t;
	}
	Lab  operator/(float & rhs)
		{
			Lab t;
			t.L=this->L/rhs;
			t.a=this->a/rhs;
			t.b=this->b/rhs;
			return t;
		}
	Lab  operator*(float & rhs)
		{
			Lab t;
			t.L=this->L*rhs;
			t.a=this->a*rhs;
			t.b=this->b*rhs;
			return t;
		}

};
class LBrBb {
public:
	float L, Br, Bb;
	LBrBb & operator=(class argb8 & rhs) ;
	LBrBb & operator=(class rgbHDR & rhs);
	LBrBb & operator=(unsigned int & rhs);
	LBrBb  operator/(const class LBrBb&rhs)
	{
		LBrBb t;
		t.L=this->L/rhs.L;
		t.Br=this->Br/rhs.Br;
		t.Bb=this->Bb/rhs.Bb;
		return t;
	}
	LBrBb  operator+(const class LBrBb&rhs)
	{
		LBrBb t;
		t.L=this->L+rhs.L;
		t.Br=this->Br+rhs.Br;
		t.Bb=this->Bb+rhs.Bb;
		return t;
	}
	LBrBb  operator/(float & rhs)
		{
		LBrBb t;
			t.L=this->L/rhs;
			t.Br=this->Br/rhs;
			t.Bb=this->Bb/rhs;
			return t;
		}
	LBrBb  operator*(float & rhs)
		{
		LBrBb t;
			t.L=this->L*rhs;
			t.Br=this->Br*rhs;
			t.Bb=this->Bb*rhs;
			return t;
		}

};
class argb8 {
public:
	unsigned char b, g, r, a;
	argb8 & operator=(Lab & rhs);
	argb8 & operator=(LBrBb & rhs);

	argb8 & operator=(unsigned int & rhs)
	{
		*((unsigned int*)this)=rhs;
		return *this;
	}
	argb8 & operator=(rgbHDR & rhs);
};

class rgbHI {
public:
	unsigned short a, r, g, b;
	rgbHI & operator=(argb8 & rhs);
	rgbHI & operator=(unsigned int & rhs)
	{
		class argb8 t;
		t= rhs;
		a=(t.a<<8)|t.a;
		r=(t.r<<8)|t.r;
		g=(t.g<<8)|t.g;
		b=(t.b<<8)|t.b;
		return *this;
	};

};

class rgbHDR {
public:
	float r, g, b;
	rgbHDR & operator=(Lab & rhs);
	rgbHDR & operator=(LBrBb & rhs);
	rgbHDR & operator=(argb8 & rhs);
	rgbHDR & operator=(unsigned int & rhs)
	{
		class argb8 t;
		t=rhs;
		*this=t;
		return *this;
	}
	rgbHDR operator/(class rgbHDR &rhs)
	{
		rgbHDR t;
		t.r=this->r/rhs.r;
		t.g=this->g/rhs.g;
		t.b=this->b/rhs.b;
		return t;
	}
	rgbHDR operator*(class rgbHDR &rhs)
	{
		rgbHDR t;
		t.r=this->r*rhs.r;
		t.g=this->g*rhs.g;
		t.b=this->b*rhs.b;
		return t;
	}
	rgbHDR operator+(class rgbHDR &rhs)
	{
		rgbHDR t;
		t.r=this->r+rhs.r;
		t.g=this->g+rhs.g;
		t.b=this->b+rhs.b;
		return t;
	}

	rgbHDR operator/(float &rhs)
	{
		rgbHDR t;
		t.r=this->r/rhs;
		t.g=this->g/rhs;
		t.b=this->b/rhs;
		return t;
	}
	rgbHDR operator*(float &rhs)
	{
		rgbHDR t;
		t.r=this->r*rhs;
		t.g=this->g*rhs;
		t.b=this->b*rhs;
		return t;
	}
	rgbHDR operator+(float &rhs)
	{
		rgbHDR t;
		t.r=this->r+rhs;
		t.g=this->g+rhs;
		t.b=this->b+rhs;
		return t;
	}
};


class hsv {
public:
	float H, S, V;
	hsv & operator=(unsigned int & rhs)
			{
				return *this;
			}
} ;

template <typename color>
class Image {
private:
	array2D<color> data;
	color bgcolor;
public:
	color* operator[](int index) {
		return data[index];
	}
    void set_pixel(int x,int y,unsigned int c);
    void set_pixel(int x,int y,color c);

	Image() {};
	Image(int w, int h); // create new
	Image(int w, int h, unsigned int * orig, unsigned int flgs = 0); // create new by reference or copying
	void set_ref(int w, int h, color * orig, unsigned int flgs = 0);
	void set_ref(int w, int h, unsigned int *  orig, unsigned int flgs = 0);
	void operator()(int w, int h, color * orig, unsigned int flgs = 0);
	void operator()(int w, int h, unsigned int *  orig, unsigned int flgs = 0);
	void operator()(int w, int h);
	Image<color> & operator=(Image<color> & rhs);
	template <typename othercolor>
		Image<color> & operator=(Image<othercolor> & rhs){
		if (this != &rhs) {
			data = rhs.data;
		}
		return *this;
	};
	template <typename othercolor>
		Image<color> & operator<<=(Image<othercolor> & rhs){
		data <<= rhs.data; // this should auto-magically do color conversion
		return *this;
	}
	void clear() { data.clear(); };
	int load(std::string name);
	int save(std::string name);
	int xsize(void) {return data.width(); };
	int ysize(void) {return data.height(); };
	int xoff(void) {return data.xoffset();};
	int yoff(void) {return data.yoffset();};

	void Lab_denoise(const float luma, const float chroma, float gam_in);
	void moveto(int nx,int ny) {data.moveto(nx,ny); };
	void move(int nx,int ny) { data.move(nx,ny); };
	void pos(int &xnpos,int &ynpos){ xnpos=data.xoffset();ynpos=data.yoffset();}
	void pos(volatile int &xnpos,volatile int &ynpos){ xnpos=data.xoffset();ynpos=data.yoffset();}
	friend class DWM_viewport;
	friend class Image<argb8>;
	friend class Image<Lab>;
	friend class Image<hsv>;
	friend class Image<rgbHI>;
	friend class Image<rgbHDR>;
	friend class Image<LBrBb>;
};

#define ARGBImage Image<argb8>
#define LabImage Image<Lab>
#define HSVImage Image<hsv>
#define HIImage Image<rgbHI>
#define HDRImage Image<rgbHDR>
#define LBrBbImage Image<LBrBb>

#endif /* IMAGE_H_ */
