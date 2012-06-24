/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
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
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _RAWIMAGESOURCE_
#define _RAWIMAGESOURCE_

#ifdef USE_LCMS2
#include <lcms2.h>
#endif

#include "../utils/array2D.h"
#include "rawimage.h"
#define HR_SCALE 2

class RawImageSource: public RawImage
{

protected:

  int W, H;
  float scale_mul[4]; // multiplier for each color
  int cblack[4]; // black offsets
  float camwb_red;
  float camwb_green;
  float camwb_blue;
  float rgb_cam[3][3];
  float cam_rgb[3][3];
//  float xyz_cam[3][3];
//  float cam_xyz[3][3];
  bool fuji;
//  bool d1x;
//  int border;
//  array2D<char> hpmap;
//  array2D<float> hrmap[3]; // for color propagation
//  array2D<char> needhr; // for color propagation
//  int max[3];
  float initialGain; // initial gain calculated after scale_colors
  float defGain;

//  bool full;

 // RawImage* ri; // Copy of raw pixels

  // to accelerate CIELAB conversion:
  float lc00, lc01, lc02, lc10, lc11, lc12, lc20, lc21, lc22;
 // float* cache;
  int threshold;

  array2D<float> rawData;

public:
  RawImageSource(char * new_filename);
  ~RawImageSource();

  int load(string fname, bool batch = false);


  void scaleColors(int winx, int winy, int winw, int winh);

//  float getDefGain()
//  {
 //   return defGain;
 // }
 // float getGamma()
 // {
 //   return 2.2;
 // };

  static void inverse33(float(*coeff)[3], float(*icoeff)[3]);

protected:
  typedef unsigned short ushort;
 // void preprocess(void);
  friend class fast_demosaic;
};

#endif
