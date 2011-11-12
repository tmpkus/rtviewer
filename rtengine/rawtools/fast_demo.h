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

#ifndef FAST_DEMO_H_
#define FAST_DEMO_H_
#include <iostream>
#include "rawimagesource.h"
#include "../imageformats/image.h"

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
