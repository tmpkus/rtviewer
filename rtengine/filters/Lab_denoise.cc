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

#include <cstddef>
#include <math.h>
#include "../imageformats/image.h"
#include "../processing/improps.h"
#include <iostream>
#include "filtermodule.h"
using namespace std;

#ifdef _OPENMP
#include <omp.h>
#endif


static inline float L_weight(float t1, float t2, float gamma)
{
	float t = (t2 - t1);
	float f;
	f = t * t;
	return gamma / (gamma + f);
}
#define BILDIFF( v1 , v2 ) L_weight( v1 , v2 , gamma )

class Chroma_t
{
public:
	float a, b;
};
class Luma_t
{
public:
	float L;
};
typedef array2D<Chroma_t> ChrImage;

typedef array2D<Luma_t> LumImage;
static inline float Chroma_weight(float t1, float t2, float gamma,float radius)
{
	float t = (t2 - t1)*0.1f;
	float f = t*t;
	if (f>9.0f) return 0.0f;

	float g=f*f;
	float p=f+0.5f*g;
	p=p+0.125f*g*f;
	p=p+g*g*0.08333333333f;
	p=1.0f/(1.0f +p);
	return p;// fast approximation of exp(-t*t);
}
static void inline ab_reduce(LumImage &ref, ChrImage &working, ChrImage &res, float amount, float gamma, int x, int y,
		int si, int ei, int sj, int ej)
{
	float wgt = 0.0f;
	float sa = working[y][x].a;
	float sb = working[y][x].b;
	float refL = ref[y][x].L;
	float a = 0.0f, b = 0.0f;
	for ( int j = sj ; j < ej ; j++ )
		for ( int i = si ; i < ei ; i++ ) {
			float cL = ref[y + j][x + i].L;
			float wa = working[y + j][x + i].a;
			float wb = working[y + j][x + i].b;
			//float f = i * i + j * j; //weight[i+BOX_RADIUS][j+BOX_RADIUS];//1.0f/(1.0f+i*i+j*j);//
			//f = 30.0f / (30.0f + f);
			float g =  Chroma_weight(cL,refL,gamma,0.0f);// * f
			wgt += g;
			a += wa * g;
			b += wb * g;
		}
	a = a / wgt;
	b = b / wgt;
	res[y][x].a = (a - sa) * amount + sa;
	res[y][x].b = (b - sb) * amount + sb;
}
static void inline L_reduce(LumImage &ref, LumImage &working, LumImage &res, float amount, float gamma, float anti_dot,
		int x, int y, int si, int ei, int sj, int ej)
{

	float sL = working[y][x].L;
	float refL = ref[y][x].L;
	float L = 0.0, wgt = 0.0;
	for ( int j = sj ; j < ej ; j++ )
		for ( int i = si ; i < ei ; i++ )
		if (i!=0||j!=0){
			float cL = working[y + j][x + i].L;
			float w = 2.0f / (2.0f + (i * i + j * j));
			float g = w * BILDIFF(cL,refL);
			wgt += g;
			L += cL * g;

		}
	// salt and pepper filter
	if (sL>0.0f)
	{
		float offset = sL*wgt;
		float rdiff = offset-L;
		//float rdiff = diff;
		if (rdiff<0.0) rdiff=-rdiff;
		// small enough difference means no salt/pepper
		if (rdiff<(anti_dot*offset)) { wgt+=1.0;L+=sL;}
	}
	res[y][x].L = (L / wgt - sL) * amount + sL;

}
static void Bilateral_Chroma(LumImage &ref, ChrImage &working, ChrImage &res, int radius, float gamma, float amount)
{
	int W = working.width(), H = working.height();
	int blocksize = 56 - radius * 2;

#pragma omp parallel for
	for ( int X = 0 ; X < W ; X += blocksize )
			{
		int h = H, Y = 0;
		for ( int y = Y ; y < h ; y++ )

		{
			int w = ((X + blocksize) >= W) ? W : X + blocksize;
			// top and bottom edges are special
			if ((y < radius) || (y > (h - radius - 1))) {
				int s1 = (y > radius) ? -radius : -y;
				int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
				for ( int x = X ; x < w ; x++ ) {
					int s2 = (x > radius) ? -radius : -x;
					int e2 = ((w - x) > (radius + 1)) ? radius + 1 : (w - x);
					ab_reduce(ref, working, res, amount, gamma, x, y, s2, e2, s1, e1);

				}
			}
			else {
				// left edge
				if (X == 0)
					for ( int x = 0 ; x < radius ; x++ ) {
						ab_reduce(ref, working, res, amount, gamma, x, y, -x, radius + 1, -radius, radius + 1);

					}
				// mid part
				int start = (X == 0) ? radius : X;
				int end = ((X + blocksize) > (W - radius - 1)) ? W - radius - 1 : X + blocksize;
				for ( int x = start ; x < end ; x++ ) {
					ab_reduce(ref, working, res, amount, gamma, x, y, -radius, radius + 1, -radius, radius + 1);

				}
				// right part
				int w = W;
				if ((X + blocksize) > (W - radius - 1))
					for ( int x = w - radius - 1 ; x < w ; x++ ) {
						ab_reduce(ref, working, res, amount, gamma, x, y, -radius, w - x, -radius, radius + 1);

					}
			}
		}
	}
}
static void Bilateral_Luma(LumImage &ref, LumImage &working, LumImage &res, int radius, float gamma, float amount,
		float anti_dot)
{
	int W = working.width(), H = working.height();
	int blocksize = 128 - radius * 2;

#pragma omp parallel for
	for ( int X = 0 ; X < W ; X += blocksize ) {
		int w = ((X + blocksize) >= W) ? W : X + blocksize;
		int y, h = H;
		for ( y = 0; y < radius ; y++ ) {
			int s1 = -y;
			int e1 = radius + 1;
			for ( int x = X ; x < w ; x++ ) {
				int s2 = (x > radius) ? -radius : -x;
				int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
				L_reduce(ref, working, res, amount, gamma, anti_dot, x, y, s2, e2, s1, e1);
			}
		}

		for ( ; y < h - radius - 1 ; y++ ) {
			// left edge
			int x = X;
			//
			if (X == 0)
				for ( ; x < radius ; x++ ) {
					L_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -x, radius + 1, -radius, radius + 1);
				}
			// mid part
			int end = (w == W) ? w - radius - 1 : w;
			for ( ; x < end ; x++ ) {
				L_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -radius, radius + 1, -radius, radius + 1);
			}
			// right part
			if (w == W)
				for ( ; x < w ; x++ ) {
					L_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -radius, w - x, -radius, radius + 1);
				}
		}
		for ( ; y < h ; y++ ) {
			//cout << "3:line " << y << endl;
			int s1 = (y > radius) ? -radius : -y;
			int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
			for ( int x = X ; x < w ; x++ ) {
				int s2 = (x > radius) ? -radius : -x;
				int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
				L_reduce(ref, working, res, amount, gamma, anti_dot, x, y, s2, e2, s1, e1);
			}
		}
	}
}

void lab_split(LabImage & src, LumImage &L, ChrImage &C)
{
	int w = src.xsize(), h = src.ysize();
#pragma omp parallel for
	for ( int y = 0 ; y < h ; y++ )
		for ( int x = 0 ; x < w ; x++ ) {
			L[y][x].L = src[y][x].L;
			C[y][x].a = src[y][x].a;
			C[y][x].b = src[y][x].b;
		}
}
void join_as_Lab(LabImage & src, LumImage &L, ChrImage &C, float luma, float chroma)
{
	int w = src.xsize(), h = src.ysize();
#pragma omp parallel for
	for ( int y = 0 ; y < h ; y++ )
		for ( int x = 0 ; x < w ; x++ ) {
			if (luma > 0.0f)
				src[y][x].L = L[y][x].L;
			if (chroma > 0.0f) {
				src[y][x].a = C[y][x].a;
				src[y][x].b = C[y][x].b;
			}
		}
}
void Lab_Denoise(LabImage & src,improps & props)
{
	if ((bool) props.pp3["[Directional Pyramid Denoising]"]["Enabled"] != true) return;
	float luma = props.pp3["[Directional Pyramid Denoising]"]["Luma"];
	float chroma = props.pp3["[Directional Pyramid Denoising]"]["Chroma"];
	const float gam_in = props.pp3["[Directional Pyramid Denoising]"]["Gamma"];
	if ((luma == 0.0)&&(chroma==0.0)) return;
	luma=luma*0.01f;
	chroma=chroma*0.01f;
	cout << "doing noise reduction\n";
	float lumaw = luma;
	float chromaw = chroma;
	float gammaw = exp(gam_in * 8.0f) / 1000.0f;
	//cout << " denoise using gamma " << gammaw << endl;

// setup size and temporary storage.
	int W = src.xsize(), H = src.ysize();
	cout << " got size " << W << " x " << H << endl;
	LumImage Luma1(W, H);
	ChrImage Chroma1(W, H);
	LumImage Luma2(W, H);
	ChrImage Chroma2(W, H);

	lab_split(src, Luma1, Chroma1);

	if (luma == 0.0f)
		lumaw = chromaw; //special case if chroma>0 but luma=0

	Bilateral_Luma(Luma1, Luma1, Luma2, 1, gammaw / 10.0f, lumaw, 0.25f);
	Bilateral_Luma(Luma2, Luma2, Luma1, 3, gammaw / 10.0f, lumaw * 0.15f, 0.1f);

	if (chromaw > 0.955f) {
		Bilateral_Chroma(Luma1, Chroma1, Chroma2, 12, gammaw, chromaw);
		Bilateral_Chroma(Luma1, Chroma2, Chroma1, 12, gammaw, chromaw);
	}
	if (chroma > 0.0f) {
		Bilateral_Chroma(Luma1, Chroma1, Chroma2, 9, gammaw, chromaw);	}

	join_as_Lab(src, Luma1, Chroma2, luma, chroma);
}

ADD_FILTER( Lab_Denoise, Labim , 100)
