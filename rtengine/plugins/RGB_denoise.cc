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
#include "../pluginhandler/plugin.h"
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

const float eps = 0.00000001f;
float wdt=3.0f;
float offset=0.0001f;
static inline float RGBdiff(const rgbHDR &tst, const rgbHDR &ref, const float amount,const float chroma)
{
  float rr=1.0f-ref.r,rb=1.0f-ref.b,rg=1.0f-ref.g;
  float tr=1.0f-tst.r,tb=1.0f-tst.b,tg=1.0f-tst.g;
//  float rr=1.0f-ref.r,rb=1.0f-ref.b,rg=1.0f-ref.g;
//  float tr=1.0f-tst.r,tb=1.0f-tst.b,tg=1.0f-tst.g;
  rr=rr*rr;rb=rb*rb;rg=rg*rg;
  tr=tr*tr;tb=tb*tb;tg=tg*tg;
#if 0
  const float Rref = .28f * ref.r + .14f * ref.b + .58f * ref.g ;
  const float Rtst = .28f * tst.r + .14f * tst.b + .58f * tst.g ;
#else
  //const float Rref =  sqrt(0.0784f *rr * rr + 0.0196f * rb * rb + 0.3364f *rg * rg);
  //const float Rtst =  sqrt(0.0784f *tr * tr + 0.0196f * tb * tb + 0.3364f *tg * tg);
  const float Rref =  sqrt(rr * rr + rb * rb + rg * rg);
  const float Rtst =  sqrt(tr * tr + tb * tb + tg * tg);
  const float Iref = .28f * ref.r + .14f * ref.b + .58f * ref.g ;
  const float Itst = .28f * tst.r + .14f * tst.b + .58f * tst.g ;
  const float Rdiff = ref.r*Itst-tst.r*Iref;
  const float Gdiff = ref.g*Itst-tst.g*Iref;
  const float Bdiff = ref.b*Itst-tst.b*Iref;
  const float Cdiff = Rdiff*Rdiff+Gdiff*Gdiff+Bdiff*Bdiff;
  const float Idiff = 1.0f - (Iref-Itst)*(Iref-Itst)*(1.0f-amount);
  return Idiff*(1.0f-Cdiff*(1.0f-chroma));
  //const float Rref =  rr+rb+rg;//(rr * rr + rb * rb + rg * rg);
  //const float Rtst =  (tr * tr + tb * tb + tg * tg);
#endif
  //const float RDiff = (Rref-Rtst)*(Rref-Rtst);//(Rref > Rtst) ? Rref - Rtst :Rtst - Rref;//
  const float RDiff =(Rref > Rtst) ? Rref - Rtst :Rtst - Rref ;
  const float width =  wdt*(offset+(Rref)); //+(ref.r+ref.g+tst.b))3.0f* (Rref+ Rtst)+
  //float width =  Rref +Rtst+ 0.000005f;
  //if(width==0.0f) width=
  //width=width*width;
#if 0
  float rv = (ref.r*Rtst - tst.r*Rref) /(Rtst*Rref+0.25f);
  float gv = (ref.g*Rtst - tst.g*Rref) /(Rtst*Rref+0.25f);
  float bv = (ref.b*Rtst - tst.b*Rref) /(Rtst*Rref+0.25f);
#else
  const float rv = (rr - tr);
  const float gv = (rg - tg);
  const float bv = (rb - tb);
  //const float Dv = .28f * ((rv>0)?rv:-rv)+.14f *((bv>0)?bv:-bv)+.58f *((gv>0)?gv:-gv);//.28f * rv + .14f * bv + .58f * gv ;
  //const float Dv = ((rv>0)?rv:-rv)+((bv>0)?bv:-bv)+((gv>0)?gv:-gv);//.28f * rv + .14f * bv + .58f * gv ;
#endif
//  const float tf = 1.0f- 1.0f*((rv*rv+gv*gv+bv*bv)*(1.0f-chroma)+chroma*Dv*Dv)/(width);//sqrt
  float tf = 1.0f- ((10.1f)-amount*10.0f)*((rv*rv+gv*gv+bv*bv)*(1.0f-chroma)+chroma*RDiff*RDiff)/(width);//sqrt
  tf=tf*tf*tf;
  if (tf<eps) return eps;
  return tf;
  float f =   width*tf;// (amount)*RDiff*RDiff +
  f=f*f;
  if (f > 9.0f)
    {
      const float f=9.0f;
      const float g = f * f;
      float p = f + 0.5f * g;
      p = p + 0.125f * g * f;
      p = p + g * g * 0.08333333333f;
      p = 1.0f / (1.0f + p);
      return p; // fast approximation of exp(-t*t);

    }

  const float g = f * f;
  float p = f + 0.5f * g;
  p = p + 0.125f * g * f;
  p = p + g * g * 0.08333333333f;
  p = 1.0f / (1.0f + p);
  return p; // fast approximation of exp(-t*t);
}
static inline float getI(const rgbHDR & sL)
{
  return .28f * sL.r + .14f * sL.b + .58f * sL.g;
  //return sqrt( .28f *sL.r*sL.r+.58f *sL.g*sL.g+.14f *sL.b*sL.b );
}
static inline float colorI(float t)
{
	return 0.75f+0.25f*t;
}

static void inline RGB_reduce(array2D<float> &Isrc,HDRImage &src, HDRImage &dst, float chroma, int x, int y, int si, int ei, int sj, int ej,int radius)
{

  const rgbHDR sL = src[y][x];
  const float sI = Isrc[y][x]; // adjusted reference intensity.
  //const float Vlen = 0.659090282f;
  rgbHDR L;//, Ln;
  L.r = 0.0f;
  L.g = 0.0f;
  L.b = 0.0f;
  float wgt = 0.0f;
  int rad=radius*radius;
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
      {
        int r=i*i+j*j;
        if ((i|j)&&(r<=rad))
          {
            rgbHDR cL= src[y + j][x + i];
            //float w, l;
            float cI = Isrc[y+j][x+i]; // intensity of cL ..
		    float tI = (sI-cI); // difference i.r.t. refpixel
		    float dI= tI*tI;//abs(tI);
		    //tI=dI*dI;
		    if ((cI>0.0f)&&(sI>0.0f))
		    {
			float dist = 1.0f;//colorI(cI);
			//float I = dI;// /(sI*cI);
			float rdiff = (cL.r-sL.r);
			float gdiff = (cL.g-sL.g);
			float bdiff = (cL.b-sL.b);
			float l = dist *(1.0f - (rdiff*rdiff+gdiff*gdiff+bdiff*bdiff)*dI);//
			if (l>0.0f)
			{
				wgt +=l;
				L.r += cL.r * l;
				L.g += cL.g * l;
				L.b += cL.b * l;
			}
	    }
	  }
  }

  //if(sI>0.0f)
  {
	//float tI = colorI(sI);
	L.r += sL.r*(1.0f-chroma);//*tI;
	L.g += sL.g*(1.0f-chroma);//*tI;
	L.b += sL.b*(1.0f-chroma);//*tI;
	wgt +=(1.0f-chroma);//*tI;
  }
  if (wgt>0.0f)
  {
	L.r = ((L.r / wgt));
	L.g = ((L.g / wgt));
	L.b = ((L.b / wgt));
  }
  dst[y][x]=L;
  /*
  float Ires = getI(L);
  if ((Ires>0.0)&&(sI>0.0))
  {
    L.r = L.r * sI / (Ires);
    L.g = L.g * sI / (Ires);
    L.b = L.b * sI / (Ires);
  
    dst[y][x] = L;
  } else dst[y][x] = sL;*/
}

static void I_reduce(array2D<float> &src, array2D<float>& dst, HDRImage & ref,float amount, int x, int y, int si, int ei, int sj, int ej,int radius)
{
  const float sI = src[y][x]; // reference intensity.
  float luma=amount;
  float Iwgt = 0.0f;
  float Isum=0.0f;
  int rad=radius*radius;
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
      {
        int r=i*i+j*j;
        if ((i|j)&&(r<=rad))
          {
            float cI= src[y + j][x + i];
            //float w, l;
            float dist =luma/(luma + (float) (r));
            float tI = (sI-cI);
	    float dI=tI*tI;
	    tI=dI*dI;//abs(tI);
            // vector length of normalization is 0.659090282
            float g = dist *(1.0f- tI*tI); // intensity difference for filter
	    Iwgt += g;
	    Isum +=cI*g;
          }
      }
    Iwgt +=1.0f-luma;
    Isum +=(1.0f-luma)*sI;
    Isum = dst[y][x] =(Iwgt>0.0f)?( Isum/Iwgt):sI;
    /*
    if (sI>0.0f) {
	rgbHDR T = ref[y][x];
	T.r = Isum*T.r/sI;
	T.g = Isum*T.g/sI;
	T.b = Isum*T.b/sI;
	ref[y][x]=T;
	// adjust pixel with proper intensity.
	
    }*/
}

static void Bilateral_float_Luma( array2D<float> & src,array2D<float>& dest,HDRImage& ref, int radius, float amount)
{
  int W = ref.xsize(), H = ref.ysize();
  int blocksize = 128 - radius * 2;
#pragma omp parallel for 
  for (int X = 0; (X < W); X += blocksize)
    {

      int w = ((X + blocksize) >= W) ? W : X + blocksize;
      int y, h = H;
      for (y = 0; y < radius; y++)
        {
          int s1 = -y;
          int e1 = radius + 1;
          for (int x = X; x < w; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              I_reduce(src, dest,ref, amount,x, y, s2,e2, s1, e1,radius);
            }
        }

      for (; y < (h - radius - 1); y++)
        {

          // left edge
          int x = X;
          //
          if (X == 0)
            for (; x < radius ; x++)
              {
                I_reduce(src, dest,ref, amount,x, y,-x, radius + 1, -radius, radius + 1,radius);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; x < end; x++)
            {
              I_reduce(src, dest,ref, amount,x, y,-radius, radius + 1, -radius, radius + 1,radius);
            }
          // right edge
          if (w == W)
            for (; x < w ; x++)
              {
                I_reduce(src, dest,ref, amount,x, y,-radius, w - x, -radius, radius + 1,radius);
              }
        }
      for (; y < h; y++)
        {

          //cout << "3:line " << y << endl;
          int s1 = (y > radius) ? -radius : -y;
          int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
          for (int x = X; x < w ; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              I_reduce(src, dest,ref, amount,x, y, s2, e2, s1, e1,radius);
            }
        }
    }
} 

static void Bilateral_HDR_chroma(array2D<float> &Isrc,HDRImage &src, HDRImage &dst, int radius, float chroma)
{
  int W = src.xsize(), H = src.ysize();
  int blocksize = 128 - radius * 2;
#pragma omp parallel for 
  for (int X = 0; (X < W); X += blocksize)
    {

      int w = ((X + blocksize) >= W) ? W : X + blocksize;
      int y, h = H;
      for (y = 0; y < radius; y++)
        {
          int s1 = -y;
          int e1 = radius + 1;
          for (int x = X; x < w ; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_reduce(Isrc,src,dst, chroma, x, y, s2,
                         e2, s1, e1,radius);
            }
        }

      for (; y < (h - radius - 1); y++)
        {

          // left edge
          int x = X;
          //
          if (X == 0)
            for (; x < radius; x++)
              {
                RGB_reduce(Isrc,src,dst, chroma, x, y,
                           -x, radius + 1, -radius, radius + 1,radius);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; x < end ; x++)
            {
              RGB_reduce(Isrc,src,dst, chroma, x, y,
                         -radius, radius + 1, -radius, radius + 1,radius);
            }
          // right edge
          if (w == W)
            for (; x < w ; x++)
              {
                RGB_reduce(Isrc,src,dst, chroma, x, y,
                           -radius, w - x, -radius, radius + 1,radius);
              }
        }
      for (; y < h ; y++)
        {

          //cout << "3:line " << y << endl;
          int s1 = (y > radius) ? -radius : -y;
          int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
          for (int x = X; x < w ; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_reduce(Isrc,src,dst, chroma, x, y, s2,
                         e2, s1, e1,radius);
            }
        }
    }
}

static void inline RGB_gauss(HDRImage &ref, HDRImage &working, HDRImage &res, float amount, float gamma, float antidot, int x, int y, int si, int ei, int sj, int ej)
{
  rgbHDR L;
  L.r = 0.0f;
  L.g = 0.0f;
  L.b = 0.0f;
  float wgt = 0.0f;
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
      {
        rgbHDR cL = working[y + j][x + i];
        float g = gamma / (gamma + (float) (i * i + j * j));
        wgt += g;
        L.r += cL.r * g;
        L.g += cL.g * g;
        L.b += cL.b * g;

      }

  L.r = L.r / wgt;
  L.g = L.g / wgt;
  L.b = L.b / wgt;

  res[y][x] = L;

}


static void RGB_denoise(HDRImage & src, improps & props)
{
  if ((bool) props.pp3["Directional Pyramid Denoising"]["Enabled"] != true)
    return;
  float luma = props.pp3["Directional Pyramid Denoising"]["Luma"];
  float chroma = props.pp3["Directional Pyramid Denoising"]["Chroma"];
  const float gam_in = props.pp3["Directional Pyramid Denoising"]["Gamma"];
  int radius = props.pp3["Directional Pyramid Denoising"]["Radius"];
  radius = (radius<=0)?6:radius;
  radius=radius/props.scale;
  wdt = 0.01f;
  offset= gam_in*0.02f;
  if (((luma == 0.0) && (chroma == 0.0))||(radius==0))
    return;

  // luma = luma * 0.01f;
  // chroma = chroma * 0.01f;
  cout << "doing noise reduction: luma " << luma << " chroma " << chroma << "gamma " << gam_in << endl;
  float lumaw = luma*0.01f;// * luma;
  float chromaw = chroma* 0.01f;
  float gammaw = gam_in;
  //cout << " denoise using gamma " << gammaw << endl;

// setup size and temporary storage.
  int W = src.xsize(), H = src.ysize();
  cout << " got size " << W << " x " << H << endl;

  array2D<float> intens1(W,H),intens2(W,H);
#pragma omp parallel for
  for (int y=0;y<H;y++)
  	for (int x=0;x<W;x++)
	{
		rgbHDR L = src[y][x];
		float I = getI(L);
		intens1[y][x]=I;
		if (I>0.0f)
		{
			L.r = L.r/I;
			L.g = L.g/I;
			L.b = L.b/I;
			src[y][x]=L;
		}
	}

	HDRImage temp(W, H);
    if (lumaw>0.0f)
    { 
		Bilateral_float_Luma(  intens1, intens2 ,temp ,radius/3, lumaw);
		Bilateral_float_Luma(  intens2, intens1 ,temp ,radius/3, lumaw);
	}
    //temp <<= src;
    if (chromaw>0.0f)
    {
		Bilateral_HDR_chroma( intens1,src,temp,radius,chromaw);
		Bilateral_HDR_chroma( intens1,temp,src,radius,chromaw);
	}
#pragma omp parallel for
  for (int y=0;y<H;y++)
  	for (int x=0;x<W;x++)
	{
		rgbHDR L = src[y][x];
		float sI = getI(L);
		float I = intens1[y][x];
		
		if ((I>0.0f)&&(sI>0.0f))
		{
			I=I/sI;
			L.r = L.r*I;
			L.g = L.g*I;
			L.b = L.b*I;
		} else L.b=L.g=L.r=0.0f;
		
		src[y][x]=L;	
	}
	

}

static int enabled(improps & props)
{
  cout << "check denoise\n";
  if ((bool) props.pp3["Directional Pyramid Denoising"]["Enabled"] != true)
   { cout << "Denoise is OFF\n"; return 0;}
  float luma = props.pp3["Directional Pyramid Denoising"]["Luma"];
  float chroma = props.pp3["Directional Pyramid Denoising"]["Chroma"];
  if ((luma == 0.0) && (chroma == 0.0))
  {
	cout << " both zero.\n";
    		return 0;
	}
  cout << "denoise enabled\n";
  return 1; // yes we have work todo.
}

ADD_FILTER( RGB_denoise, HDRim, 100)
