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

static inline float RGBdiff(const rgbHDR &tst,const rgbHDR &ref,const float amount)
{
	float Rref = ref.r*ref.r+ref.b*ref.b+ref.g*ref.g;
	//float Rtst = tst.r*tst.r+tst.b*tst.b+tst.g*tst.g;
	//float RDiff = (Rref>Rtst)? Rref-Rtst:Rtst-Rref;

	float width = amount*(Rref+0.1f);//+(ref.r+ref.g+tst.b))
    //if(width==0.0f) width=

	float rv=(ref.r-tst.r);
	float gv=(ref.g-tst.g);
	float bv=(ref.b-tst.b);
	float f = ((rv*rv+gv*gv+bv*bv)*15.0f/*+RDiff*5.0f*/)/width;

	if (f>9.0f) return 0.000001f;

	float g=f*f;
	float p=f+0.5f*g;
	p=p+0.125f*g*f;
	p=p+g*g*0.08333333333f;
	p=1.0f/(1.0f +p);
	return p;// fast approximation of exp(-t*t);
}

static void inline RGB_reduce(HDRImage &ref, HDRImage &working, HDRImage &res, float amount, float gamma, float antidot,
		int x, int y, int si, int ei, int sj, int ej)
{

	const rgbHDR sL = ref[y][x];
	const rgbHDR refL = working[y][x];
	rgbHDR L,Ln;
	L.r=0.0f;L.g=0.0f;L.b=0.0f;
	float wgt = 0.0f,wgtn = 0.0f;
	int n=0;
	for ( int j = sj ; j < ej ; j++ )
		for ( int i = si ; i < ei ; i++ )
		{
			rgbHDR cL;
			float w,g;
			if (i!=0||j!=0)
			{

				cL= working[y + j][x + i];
				g = RGBdiff(cL,sL,amount)*50.0f / (50.0f + (float)(i * i + j * j));

			//float g = w * RGBdiff(cL,refL,amount);
			if (g>0.4) // correlation found
			{
				n++;
				wgtn += g;
				Ln.r += cL.r * g;
				Ln.g += cL.g * g;
				Ln.b += cL.b * g;
			} else {
				wgt += g;
				L.r += cL.r * g;
				L.g += cL.g * g;
				L.b += cL.b * g;
			}
			}
		}

	if (n>15)
	{
		L.r=(Ln.r/wgtn);//*(1.0-antidot)+antidot*sL.r;//-sL.r)*amount+sL.r;
		L.g=(Ln.g/wgtn);//*(1.0-antidot)+antidot*sL.g;//-sL.g)*amount+sL.g;
		L.b=(Ln.b/wgtn);//*(1.0-antidot)+antidot*sL.b;//-sL.b)*amount+sL.b;
		res[y][x]=L;
		return;
	}
	if (n>2)//(wgt>0.65f) // no salt/pepper
	{
		const float add=antidot;
		wgt+=add+wgtn;
		L.r+=refL.r*add+Ln.r;
		L.g+=refL.g*add+Ln.g;
		L.b+=refL.b*add+Ln.b;



	}/**/
	// salt and pepper filter
/*	if (sL>0.0f)
	{
		float offset = sL*wgt;
		float rdiff = offset-L;
		//float rdiff = diff;
		if (rdiff<0.0) rdiff=-rdiff;
		// small enough difference means no salt/pepper
		if (rdiff<(anti_dot*offset)) { wgt+=0.5f;L+=sL*0.5f;}
	}*/
	L.r=(L.r/wgt)*(1.0-antidot)+antidot*sL.r;//-sL.r)*amount+sL.r;
	L.g=(L.g/wgt)*(1.0-antidot)+antidot*sL.g;//-sL.g)*amount+sL.g;
	L.b=(L.b/wgt)*(1.0-antidot)+antidot*sL.b;//-sL.b)*amount+sL.b;

	res[y][x]=L;//(L / wgt - sL) * amount + sL;

}
static void Bilateral_HDR_Luma(HDRImage &ref, HDRImage &working, HDRImage &res, int radius, float gamma, float amount,
		float anti_dot,volatile int &early)
{
	int W = working.xsize(), H = working.ysize();
	int blocksize = 128 - radius * 2;

#pragma omp parallel for schedule(dynamic)
	for ( int X = 0 ; (X < W * early); X += blocksize ) {

		int w = ((X + blocksize) >= W) ? W : X + blocksize;
		int y, h = H;
		for ( y = 0; y < radius * early; y++ ) {
			if (early) break;
			int s1 = -y;
			int e1 = radius + 1;
			for ( int x = X ; x < w * early; x++ ) {
				int s2 = (x > radius) ? -radius : -x;
				int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
				RGB_reduce(ref, working, res, amount, gamma, anti_dot, x, y, s2, e2, s1, e1);
			}
		}

		for ( ; y < (h - radius - 1)* early ; y++ ) {

			// left edge
			int x = X;
			//
			if (X == 0)
				for ( ; x < radius* early ; x++ ) {
					RGB_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -x, radius + 1, -radius, radius + 1);
				}
			// mid part
			int end = (w == W) ? w - radius - 1 : w;
			for ( ; x < end* early ; x++ ) {
				RGB_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -radius, radius + 1, -radius, radius + 1);
			}
			// right part
			if (w == W)
				for ( ; x < w* early ; x++ ) {
					RGB_reduce(ref, working, res, amount, gamma, anti_dot, x, y, -radius, w - x, -radius, radius + 1);
				}
		}
		for ( ; y < h* early ; y++ ) {

			//cout << "3:line " << y << endl;
			int s1 = (y > radius) ? -radius : -y;
			int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
			for ( int x = X ; x < w* early ; x++ ) {
				int s2 = (x > radius) ? -radius : -x;
				int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
				RGB_reduce(ref, working, res, amount, gamma, anti_dot, x, y, s2, e2, s1, e1);
			}
		}
	}
}


void RGB_denoise(HDRImage & src,improps & props)
{
	if ((bool) props.pp3["[Directional Pyramid Denoising]"]["Enabled"] != true) return;
	float luma = props.pp3["[Directional Pyramid Denoising]"]["Luma"];
	float chroma = props.pp3["[Directional Pyramid Denoising]"]["Chroma"];
	const float gam_in = props.pp3["[Directional Pyramid Denoising]"]["Gamma"];
	if ((luma == 0.0)&&(chroma==0.0)) return;

	luma=luma*0.01f;
	chroma=chroma*0.01f;
	cout << "doing noise reduction: luma " << luma << " chroma " << chroma << "gamma " << gam_in << endl ;
	float lumaw = luma;
	float chromaw = chroma;
	float gammaw = gam_in;
	//cout << " denoise using gamma " << gammaw << endl;

// setup size and temporary storage.
	int W = src.xsize(), H = src.ysize();
	cout << " got size " << W << " x " << H << endl;

	HDRImage temp1(W,H);//,temp2;

	Bilateral_HDR_Luma(src, src, temp1, 12, gammaw / 10.0f, lumaw, 0.01f,props.early);
	if (props.early) src=temp1;
	//Bilateral_HDR_Luma(src,temp1,src ,3, gammaw / 10.0f, lumaw, 0.01f);
	//Bilateral_HDR_Luma(temp1,src,temp1 ,1, gammaw / 10.0f, lumaw, 0.0f);
	//src=temp1;

#if 0
	Bilateral_HDR_Luma(temp1,src,temp1 ,10, gammaw / 10.0f, lumaw, 1.0f);//lumaw * 0.15f, 0.1f);
	src=temp1;
#else

#endif

}

ADD_FILTER( RGB_denoise, HDRim , 12)
