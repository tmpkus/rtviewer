/*
 * fast_demo.h
 *
 *  Created on: Jun 14, 2011
 *      Author: janrinze
 */

#ifndef FAST_DEMO_H_
#define FAST_DEMO_H_
#include <iostream>
#include "rawimagesource.h"
#include "image.h"

// tile size
#define TS 256
#define TILE_SIZE (TS-32)
enum Demosaic_Method
{
	FAST_DEMOSAIC,
	JRP_DEMOSAIC,
	AMAZE_DEMOSAIC,
	HALFSIZE_DEMOSAIC,
	VARSIZE_DEMOSAIC,
};
static improps defprop;
class fast_demosaic : public RawImageSource
{
private:
	float mat[3][3];
	float channel_mul[4];
	float range;
	array2D<char> Tile_flags;
	HDRImage RGB_converted;
	enum Demosaic_Method method;
public:
	improps props;
	bool success;
	fast_demosaic(char * mfilename):  props(defprop),RawImageSource(mfilename)
	{
		std::cout << "fast_demosaic: load file " << mfilename << endl;
		success=(load(mfilename, 0)==0);
		std::cout << "fast_demosaic: DONE load file " << mfilename << endl;
	};
	fast_demosaic(char * mfilename,improps nprop): props(nprop), RawImageSource(mfilename)
	{
		std::cout << "fast_demosaic: load file " << mfilename << endl;
		success=(load(mfilename, 0)==0);
		std::cout << "fast_demosaic: DONE load file " << mfilename << endl;
	};
	void fast_demo(HDRImage & dest);
	void amaze_demosaic_RT(HDRImage & dest);
	void jrp_demo(HDRImage & dest);
	void naive(HDRImage & dst,array2D<float> &hint,float adjust ,float ladjust );
	void corner(HDRImage & dst,array2D<float> &I);
	void improve_correlation(HDRImage & pass1,HDRImage& pass2,float adjust,float ladjust,int green);
	void color_correct(HDRImage & pass1, HDRImage& pass2);
	int touch_tiles(HDRImage &dest,int &tile_xs,int &tile_xe,int &tile_ys,int &tile_ye);
	void half_size_demo(HDRImage & dest);
	void nth_size_demo(HDRImage & dest,int num);
	float get_ISO(void) {return iso_speed;}
	enum Demosaic_Method demosaic_method() {return method;}
	void linear_interpolate(void);
	void cook_data(void);
};
#endif /* FAST_DEMO_H_ */
