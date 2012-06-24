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

#include "rawimage.h"
#include "rawimagesource.h"
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

//extern Settings settings;

#undef ABS
#undef MAX
#undef MIN
#undef DIST

#define ABS(a) ((a)<0?-(a):(a))
#define MAX(a,b) ((a)<(b)?(b):(a))
#define MIN(a,b) ((a)>(b)?(b):(a))
#define DIST(a,b) (ABS(a-b))

#define FCLIP(a) ((a)>65535.0?65535.0:((a)<0.0?0.0:(a)))
#define PIX_SORT(a,b) { if ((a)>(b)) {float temp=(a);(a)=(b);(b)=temp;} }
#if 1
#define med3x3(a0,a1,a2,a3,a4,a5,a6,a7,a8,median) { \
p[0]=a0; p[1]=a1; p[2]=a2; p[3]=a3; p[4]=a4; p[5]=a5; p[6]=a6; p[7]=a7; p[8]=a8; \
PIX_SORT(p[1],p[2]); PIX_SORT(p[4],p[5]); PIX_SORT(p[7],p[8]); \
PIX_SORT(p[0],p[1]); PIX_SORT(p[3],p[4]); PIX_SORT(p[6],p[7]); \
PIX_SORT(p[1],p[2]); PIX_SORT(p[4],p[5]); PIX_SORT(p[7],p[8]); \
PIX_SORT(p[0],p[3]); PIX_SORT(p[5],p[8]); PIX_SORT(p[4],p[7]); \
PIX_SORT(p[3],p[6]); PIX_SORT(p[1],p[4]); PIX_SORT(p[2],p[5]); \
PIX_SORT(p[4],p[7]); PIX_SORT(p[4],p[2]); PIX_SORT(p[6],p[4]); \
PIX_SORT(p[4],p[2]); median=p[4];} //a4 is the median
#else
typedef struct
{
  float median;
  struct dangle *next;
} dangle;

#define pushlo(a) { low[l++]=a; }
#define pushhi(a) { high[h++]=a; }
#define inslo(a) { int i=++l;while((i-1)&&low[i-1]>a){low[i]=low[i-1];i--};low[i]=a;}}
#define inshi(a) { int i=++h;while((i-1)&&high[i-1]<a){high[i]=high[i-1];i--};high[i]=a;}}

#define insert(a) \
	if (m<a) \
	{ \
		if (l>h) inshi(a) else { pushlo(m);m=a;} \
	} else { \
		if (l<h) inslo(a) else { pushhi(m);m=a;} \
	}

//inline void med3x3(float a0,float a1,float a2,float a3,float a4,float a5,float a6,float a7,float a8,float &median)
#define med3x3(a0,a1,a2,a3,a4,a5,a6,a7,a8,median)  \
	{ \
  float m=a0,high[9],low[9]; \
  int l=0,h=0; \
  insert(a1); \
  insert(a2); \
  insert(a3); \
  insert(a4); \
  insert(a5); \
  insert(a6); \
  insert(a7); \
  insert(a8); \
  median=m; \
}
#endif

RawImageSource::RawImageSource(char * mfilename) :
  RawImage(mfilename)
{

#ifdef USE_LCMS2
  camProfile = NULL;
  embProfile = NULL;
#endif
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

RawImageSource::~RawImageSource()
{

  unsigned short t = data[0][0];


#ifdef USE_LCMS2
  if (camProfile)
    cmsCloseProfile(camProfile);
  if (embProfile)
    cmsCloseProfile(embProfile);
#endif
}

int RawImageSource::load(string fname, bool batch)
{
  int res = loadRaw(1, 1);
  cout << "loading file " << fname << "result in " << res << endl;
  if (res)
    return res;
  compress_image();
  W = get_width();
  H = get_height();
//  fuji = get_FujiWidth() != 0;
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      rgb_cam[i][j] = get_rgb_cam(i, j);
  // compute inverse of the color transformation matrix
  // first arg is matrix, second arg is inverse
  inverse33(rgb_cam, cam_rgb);

//  d1x = !get_model().compare("D1X");
//  if (d1x)
//    border = 8;
  if (get_profile())
    {
#ifdef USE_LCMS2
      embProfile = cmsOpenProfileFromMem(get_profile(), get_profileLen());
#endif
    }
  // create profile

  float pre_mul[4];
  get_colorsCoeff(pre_mul, scale_mul, cblack);

  camwb_red = get_pre_mul(0) / pre_mul[0];
  camwb_green = get_pre_mul(1) / pre_mul[1];
  camwb_blue = get_pre_mul(2) / pre_mul[2];
  // this will avoid purple cast in highlights
  initialGain = 1.0 / MIN(MIN(pre_mul[0],pre_mul[1]),pre_mul[2]);

  float cam_r = rgb_cam[0][0] * camwb_red + rgb_cam[0][1] * camwb_green
                + rgb_cam[0][2] * camwb_blue;
  float cam_g = rgb_cam[1][0] * camwb_red + rgb_cam[1][1] * camwb_green
                + rgb_cam[1][2] * camwb_blue;
  float cam_b = rgb_cam[2][0] * camwb_red + rgb_cam[2][1] * camwb_green
                + rgb_cam[2][2] * camwb_blue;


  set_prefilters();


  cout << "loading file DONE" << endl;
  return 0; // OK!
}


  void RawImageSource::inverse33(float(*rgb_cam)[3], float(*cam_rgb)[3])
  {
    float nom = (rgb_cam[0][2] * rgb_cam[1][1] * rgb_cam[2][0] - rgb_cam[0][1]
                 * rgb_cam[1][2] * rgb_cam[2][0] - rgb_cam[0][2] * rgb_cam[1][0]
                 * rgb_cam[2][1] + rgb_cam[0][0] * rgb_cam[1][2] * rgb_cam[2][1]
                 + rgb_cam[0][1] * rgb_cam[1][0] * rgb_cam[2][2] - rgb_cam[0][0]
                 * rgb_cam[1][1] * rgb_cam[2][2]);
    cam_rgb[0][0] = (rgb_cam[1][2] * rgb_cam[2][1] - rgb_cam[1][1]
                     * rgb_cam[2][2]) / nom;
    cam_rgb[0][1] = -(rgb_cam[0][2] * rgb_cam[2][1] - rgb_cam[0][1]
                      * rgb_cam[2][2]) / nom;
    cam_rgb[0][2] = (rgb_cam[0][2] * rgb_cam[1][1] - rgb_cam[0][1]
                     * rgb_cam[1][2]) / nom;
    cam_rgb[1][0] = -(rgb_cam[1][2] * rgb_cam[2][0] - rgb_cam[1][0]
                      * rgb_cam[2][2]) / nom;
    cam_rgb[1][1] = (rgb_cam[0][2] * rgb_cam[2][0] - rgb_cam[0][0]
                     * rgb_cam[2][2]) / nom;
    cam_rgb[1][2] = -(rgb_cam[0][2] * rgb_cam[1][0] - rgb_cam[0][0]
                      * rgb_cam[1][2]) / nom;
    cam_rgb[2][0] = (rgb_cam[1][1] * rgb_cam[2][0] - rgb_cam[1][0]
                     * rgb_cam[2][1]) / nom;
    cam_rgb[2][1] = -(rgb_cam[0][1] * rgb_cam[2][0] - rgb_cam[0][0]
                      * rgb_cam[2][1]) / nom;
    cam_rgb[2][2] = (rgb_cam[0][1] * rgb_cam[1][0] - rgb_cam[0][0]
                     * rgb_cam[1][1]) / nom;
  }

