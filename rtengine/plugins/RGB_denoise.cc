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



typedef struct
{
	array2D<float> &refI,&srcI,&dstI;
	HDRImage &refHDR,&srcHDR,&dstHDR;
    array2D<float> &lrefI,&lsrcI,&ldstI;
    HDRImage &lrefHDR,&lsrcHDR,&ldstHDR;
} refstorage;


const float eps = 0.00000001f;
float wdt=3.0f;
float offset=0.0001f;
static float mygamma;

// not used anymore.
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

//#define __ROOT_INTENS__
#define epsilon eps 
//(1.0f/100000.0f)

static inline float convI(float I)
{
#if defined( __ROOT_INTENS__ )
  if (mygamma==1) return I;
  if (mygamma==2) return sqrt(I);
  return pow(I,1.0f/mygamma);
#else
  if (I<=0.0f) I=epsilon;
  return I;
#endif
}

static inline float revI(float I)
{
#if defined( __ROOT_INTENS__ )
  if (mygamma==1) return I;
  if (mygamma==2) return I*I;
  return pow(I,mygamma);
#else
  if (I<=0.0f) I=epsilon;
  return I;
#endif
}


static inline void Clamp( float &V , float min ,float max ) { if ( V < min) V=min;else if (V>max) V=max;}


void ClipHDR(rgbHDR &L)
{
	Clamp(L.r , 0.0f , 4.0f );
	Clamp(L.g , 0.0f , 4.0f );
	Clamp(L.b , 0.0f , 4.0f );
}
static inline float getI( rgbHDR & sL)
{
  ClipHDR(sL);
  float r = (0.166666f+.28f) * sL.r;
  float g = (0.166666f+.58f) * sL.g;
  float b = (0.166666f+.14f) * sL.b;
  float sum = 0.75f*(r+g+b);
  return sum;
}

/*
  other methods can be:
    sum = .28f * sL.r + .58f * sL.g + .14f * sL.b;

  or 
  float r = (0.33333f+.28f) * sL.r;
  float g = (0.33333f+.58f) * sL.g;
  float b = (0.33333f+.14f) * sL.b;
  if (r<0.0f)r=0.0f;
  if (g<0.0f)g=0.0f;
  if (b<0.0f)b=0.0f;
  float sum = 0.5f*(r+g+b);

  optionally use square of sum, clipping and log of sum.
  But none improve result.
  //sum=sum*sum;
  //if (sum<0.0f) { sL.r=0.0f;sL.g=0.0f;sL.b=0.0f;return 0.0f;}
  //return sum*sum; //return (sqrt(r*r+ g*g + b*b));
  //logf(sum);
  //return sqrt(sqrt(r*r+ g*g + b*b));//sL.g*sL.g+ sL.b*sL.b );
  //return sqrt( .28f *sL.r*sL.r+.58f *sL.g*sL.g+.14f *sL.b*sL.b );
*/

static inline float colorI(float t)
{
	return 0.75f+0.25f*t;
}
static void inline RGB_reduce(array2D<float> &logIref,HDRImage & lref,HDRImage &ldst ,HDRImage &src, HDRImage &dst, float chroma, int x, int y, int si, int ei, int sj, int ej,int radius)
{

  const rgbHDR sL = lref[y][x];
  float sI =logIref[y][x];
  rgbHDR L;
  double R = 0.0,G=0.0,B=0.0;
  L.r = 0.0f;
  L.g = 0.0f;
  L.b = 0.0f;
  double wgt = 0.0f;
  float rad=radius*radius;
  float rd=radius;
  double rscale = 6.0*(double)chroma*(double)chroma;
  double coldiff = 0.5/(0.1+(double)chroma*(double)chroma);
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
      {
	double fi=(float)i/rd;
	double fj=(float)j/rd;
        double r=fi*fi+fj*fj;
        if (r<=1.0f) // circular
        {
	    double fact=rscale/(rscale+r);
            rgbHDR cL = src    [y + j][x + i]; 	// scaled color
            rgbHDR lL = lref   [y + j][x + i]; 	// log scaled color
            float  cI = logIref[y + j][x + i]; 	// log intensity of cL ..
            double tI = cI-sI;		    		// log(cI/sI) == log(cI)-log(sI)
	    tI=tI*tI;
	    double rdiff = lL.r-sL.r;			// log(cR/sR) == log(cR)-log(sR)
	    double gdiff = lL.g-sL.g;			// log(cG/sG) == log(cG)-log(sG)
	    double bdiff = lL.b-sL.b;			// log(cB/sB) == log(cB)-log(sB)
	    double diff = tI + coldiff*(rdiff*rdiff+gdiff*gdiff+bdiff*bdiff);
	    double l = fact*fact/(fact + diff);

	    wgt +=l;
	    R += (double)cL.r * l;
	    G += (double)cL.g * l;
	    B += (double)cL.b * l;
	    }
      }

	R = R / wgt ;
	G = G / wgt ;
	B = B / wgt ;
	L.r=R;
	L.g=G;
	L.b=B;

	dst[y][x]=L;

	L.r=logf(L.r+eps);
	L.g=logf(L.g+eps);
	L.b=logf(L.b+eps);
	ldst[y][x]=L;
}
static void I_reduce(array2D<float> & lrefI  , array2D<float>& lsrcI , array2D<float>& ldstI ,array2D<float>&ref,array2D<float>&src,array2D<float>&dst,float rdist, int x, int y, int si, int ei, int sj, int ej,int radius)
{
  float rI =lrefI[y][x];// reference intensity.
  float sI =lsrcI[y][x];
  double Iwgt = 0.0f;
  double Isum=0.0f;
  float rad=radius*radius;
  float rd  = radius;
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
    {
	float fi=(float)i/rd;
	float fj=(float)j/rd;
        float r=fi*fi+fj*fj;
        if (r<=1.0f) // circular
        {
            float cI= lsrcI[y + j][x + i];
            double dist = (double)rdist/((double)rdist + (double)r);
            double tI = ((double)cI-(double)rI);
            // use doubles to reduce error because the differences are quite large.
            double g= dist*dist/(dist+tI*tI);
	    Iwgt += g;
	    Isum +=src[y+j][x+i]*g;
	}
    }
    Isum=Isum/Iwgt ;
    if (Isum <=0.0f) Isum=0.0f;
    dst[y][x] = Isum;
    ldstI[y][x]= log(Isum+eps);
}

static void Bilateral_float_Luma( array2D<float> & lrefI  , array2D<float>& lsrcI , array2D<float>& ldstI ,array2D<float>&ref,array2D<float>&src,array2D<float>&dst, int radius, float amount)
{
  int W = ref.width(), H = ref.height();
  int blocksize = 128 - radius * 2;
  amount=0.25f*amount*amount;
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
              I_reduce(lrefI,lsrcI,ldstI,ref,src,dst, amount,x, y, s2,e2, s1, e1,radius);
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
                I_reduce(lrefI,lsrcI,ldstI,ref,src,dst, amount,x, y,-x, radius + 1, -radius, radius + 1,radius);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; x < end; x++)
            {
              I_reduce(lrefI,lsrcI,ldstI,ref,src,dst, amount,x, y,-radius, radius + 1, -radius, radius + 1,radius);
            }
          // right edge
          if (w == W)
            for (; x < w ; x++)
              {
                I_reduce(lrefI,lsrcI,ldstI,ref,src,dst, amount,x, y,-radius, w - x, -radius, radius + 1,radius);
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
              I_reduce(lrefI,lsrcI,ldstI,ref,src,dst, amount,x, y, s2, e2, s1, e1,radius);
            }
        }
    }
} 

static void Bilateral_HDR_chroma(array2D<float> &Iref,HDRImage &lref,HDRImage &ldst, HDRImage &src, HDRImage &dst, int radius, float chroma,bool writeback)

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
              RGB_reduce(Iref,lref,ldst,src,dst, chroma, x, y, s2,
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
                RGB_reduce(Iref,lref,ldst,src,dst, chroma, x, y,
                           -x, radius + 1, -radius, radius + 1,radius);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; x < end ; x++)
            {
              RGB_reduce(Iref,lref,ldst,src,dst, chroma, x, y,
                         -radius, radius + 1, -radius, radius + 1,radius);
            }
          // right edge
          if (w == W)
            for (; x < w ; x++)
              {
                RGB_reduce(Iref,lref,ldst,src,dst, chroma, x, y,
                           -radius, w - x, -radius, radius + 1,radius);
              }
        }
      for (; y < h ; y++)
        {
          int s1 = (y > radius) ? -radius : -y;
          int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
          for (int x = X; x < w ; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_reduce(Iref,lref,ldst,src,dst,chroma, x, y, s2,
                         e2, s1, e1,radius);
            }
        }
    }
}

static void RGB_denoise(HDRImage & src, improps & props)
{
  if ((bool) props.pp3["Directional Pyramid Denoising"]["Enabled"] != true)
    return;
  float luma = props.pp3["Directional Pyramid Denoising"]["Luma"];
  float chroma = props.pp3["Directional Pyramid Denoising"]["Chroma"];
  const float gam_in = props.pp3["Directional Pyramid Denoising"]["Gamma"];
  int radius = props.pp3["Directional Pyramid Denoising"]["Radius"];
  mygamma = gam_in;
  radius = (radius<=0)?6:radius;
  radius=radius/props.scale;
  wdt = 0.01f;
  offset= gam_in*0.02f;
  if (((luma == 0.0) && (chroma == 0.0))||(radius==0))
    return;
  float lumaw = luma*0.01f;// * luma;
  float chromaw = chroma* 0.01f;
  float gammaw = gam_in;
  // setup size and temporary storage.
  int W = src.xsize(), H = src.ysize();
  cout << " got size " << W << " x " << H << endl;

  // pull out all log conversions into separate arrays and update after bilateral filtering.
  // this will speedup calculations by a huge amount.

  array2D<float> intens1(W,H);
  array2D<float> logI(W,H);
  HDRImage  logHDR(W,H);

#pragma omp parallel for
  for (int y=0;y<H;y++)
    for (int x=0;x<W;x++)
    {
	rgbHDR L = src[y][x];
	float I = getI(L);
	float J=(I);
	I=I*I;
	intens1[y][x]=I;
	if (J>0.0f)
	{
	    L.r=L.r/J ; // scale color w.r.t. intensity.
	    L.g=L.g/J; // scale color w.r.t. intensity.
	    L.b=L.b/J; // scale color w.r.t. intensity.
	} else L.r=L.g=L.b=1.0f;
	src[y][x]=L;
	I=logf(I+eps);
	logI[y][x]=I;
	// make available as log() too. (will reduce amount of log() operations by a factor radius*radius which is a lot!
	L.r = logf(L.r+eps);
	L.g = logf(L.g+eps);
	L.b = logf(L.b+eps);
	logHDR[y][x]=L;
	}

    if (lumaw>0.0f)
    {
	array2D<float> tlogI(W,H),t2logI(W,H);
	array2D<float> intens2(W,H);// define within if to ensure memory gets released after operation
	Bilateral_float_Luma( logI,logI,tlogI, intens1, intens1, intens2 ,radius, lumaw);
	Bilateral_float_Luma( logI,tlogI,logI, intens1, intens2, intens1 ,radius, lumaw);
    }

    if (chromaw>0.0f)
    {
	HDRImage tlogHDR(W, H),temp(W,H); // define within if to ensure memory gets released after operation so can be reused when doing luma.
	Bilateral_HDR_chroma( logI,logHDR,tlogHDR,src,temp,radius,chromaw,1);
	Bilateral_HDR_chroma( logI,tlogHDR,logHDR,temp,src,radius,chromaw,0); // no writeback to logHDR needed.
    }

#pragma omp parallel for
  for (int y=0;y<H;y++)
  	for (int x=0;x<W;x++)
	{
		rgbHDR L = src[y][x];
		float sI = (getI(L));
		float I = intens1[y][x];
		// renormalize and scale with intensity.
		if (I<0.0f) I=0.0f;
		I=sqrt(I);
		if (sI>0.0f)
		{
		    I=I/sI;
		    L.r = (L.r)*I;
		    L.g = (L.g)*I;
		    L.b = (L.b)*I;
		} else L.b=L.g=L.r=I;
		src[y][x]=L;
	}
}

static int enabled(improps & props)
{
  cout << "check denoise\n";
  if ((bool) props.pp3["Directional Pyramid Denoising"]["Enabled"] != true)
   {
   // cout << "Denoise is OFF\n";
    return 0;
    }
  float luma = props.pp3["Directional Pyramid Denoising"]["Luma"];
  float chroma = props.pp3["Directional Pyramid Denoising"]["Chroma"];
  if ((luma == 0.0) && (chroma == 0.0))
  {
	// cout << " both zero.\n";
    		return 0;
	}
  // cout << "denoise enabled\n";
  return 1; // yes we have work todo.
}

ADD_FILTER( RGB_denoise, HDRim, 100)
