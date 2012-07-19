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
  const float Dv = ((rv>0)?rv:-rv)+((bv>0)?bv:-bv)+((gv>0)?gv:-gv);//.28f * rv + .14f * bv + .58f * gv ;
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

static void inline RGB_reduce(HDRImage &ref, HDRImage &working, HDRImage &res, float amount, float gamma, float chroma, int x, int y, int si, int ei, int sj, int ej,int radius)
{

  const rgbHDR sL = ref[y][x];
  const rgbHDR refL = working[y][x];
  rgbHDR L, Ln;
  L.r = 0.0f;
  L.g = 0.0f;
  L.b = 0.0f;
  Ln.r = 0.0f;
  Ln.g = 0.0f;
  Ln.b = 0.0f;
  float lum=/*1.05f-*/amount;
  float chrom=chroma;//(11.0f-sqrt(chroma))*2.0f;// 2 .. 20 non linear
  float thresh = gamma;//(chrom*chrom);
  amount = amount *50.0f;
  float wgt = 0.0f, wgtn = 0.0f;
  int n = 0,rad=radius*radius;
  for (int j = sj; j < ej; j++)
    for (int i = si; i < ei; i++)
      {
        int r=i*i+j*j;
        if (((i != 0) || (j != 0))&&(r<=rad))
          {
            rgbHDR cL;
            float w, g;
            //cL = ref[y + j][x + i];
            cL = working[y + j][x + i];
            g = RGBdiff(cL, sL, lum,chrom) * amount
                / (amount + (float) (i * i + j * j));

            //float g = w * RGBdiff(cL,refL,amount);
            if (g > thresh) // strong correlation found
              {
                n++;
              }
            wgt += g;
            L.r += cL.r * g;
            L.g += cL.g * g;
            L.b += cL.b * g;
#ifdef twice
            cL = ref[y + j][x + i];
            g = RGBdiff(cL, sL, amount) * chroma
                / (50.0f + (float) (i * i + j * j));

            //float g = w * RGBdiff(cL,refL,amount);
            if (g > 0.2) // strong correlation found
              {
                n++;
              }
            wgt += g;
            L.r += cL.r * g;
            L.g += cL.g * g;
            L.b += cL.b * g;
#endif
          }
      }

  /*	 if (n>15)
  	 {
  	 wgtn+=1.0f;
  	 Ln.r+=refL.r;
  	 Ln.g+=refL.g;
  	 Ln.b+=refL.b;
  	 L.r=(Ln.r/wgtn);//*(1.0-antidot)+antidot*sL.r;//-sL.r)*amount+sL.r;
  	 L.g=(Ln.g/wgtn);//*(1.0-antidot)+antidot*sL.g;//-sL.g)*amount+sL.g;
  	 L.b=(Ln.b/wgtn);//*(1.0-antidot)+antidot*sL.b;//-sL.b)*amount+sL.b;
  	 res[y][x]=L;
  	 return;
  	 }*/
#ifdef account_count
  if (n > 1) //(wgt>0.65f) // no salt/pepper
    {
      const float add = 1.0f;
      wgt += add ;
      L.r += sL.r * add ;
      L.g += sL.g * add;
      L.b += sL.b * add ;

    }
  else
    {
      const float add = 0.00000001f ;
      wgt += add ;
      L.r += sL.r * add;
      L.g += sL.g * add;
      L.b += sL.b * add;
    }
#endif

  /**/
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
  if(n>1)
    {
      L.r += refL.r;
      L.g += refL.g;
      L.b += refL.b;
      wgt +=1.0f;
    }
  else
    {
      L.r += refL.r*0.1f;
      L.g += refL.g*0.1f;
      L.b += refL.b*0.1f;
      wgt +=0.1f;
    }
  L.r = ((L.r / wgt));//-sL.r)*amount+sL.r; //*(1.0-antidot)+chroma*sL.r;//;
  L.g = ((L.g / wgt));//-sL.g)*amount+sL.g; //*(1.0-antidot)+chroma*sL.g;//;
  L.b = ((L.b / wgt));//-sL.b)*amount+sL.b; //*(1.0-antidot)+chroma*sL.b;//;

  res[y][x] = L; //(L / wgt - sL) * amount + sL;

}
static void Bilateral_HDR_Luma(HDRImage &ref, HDRImage &working, HDRImage &res, int radius, float gamma, float amount, float chroma, volatile int &early)
{
  int W = working.xsize(), H = working.ysize();
  int blocksize = 128 - radius * 2;
//
#pragma omp parallel for schedule(dynamic)
  for (int X = 0; (X < W * early); X += blocksize)
    {

      int w = ((X + blocksize) >= W) ? W : X + blocksize;
      int y, h = H;
      for (y = 0; y < radius * early; y++)
        {
          int s1 = -y;
          int e1 = radius + 1;
          for (int x = X; x < w * early; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_reduce(ref, working, res, amount, gamma, chroma, x, y, s2,
                         e2, s1, e1,radius);
            }
        }

      for (; y < (h - radius - 1) * early; y++)
        {

          // left edge
          int x = X;
          //
          if (X == 0)
            for (; x < radius * early; x++)
              {
                RGB_reduce(ref, working, res, amount, gamma, chroma, x, y,
                           -x, radius + 1, -radius, radius + 1,radius);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; x < end * early; x++)
            {
              RGB_reduce(ref, working, res, amount, gamma, chroma, x, y,
                         -radius, radius + 1, -radius, radius + 1,radius);
            }
          // right part
          if (w == W)
            for (; x < w * early; x++)
              {
                RGB_reduce(ref, working, res, amount, gamma, chroma, x, y,
                           -radius, w - x, -radius, radius + 1,radius);
              }
        }
      for (; y < h * early; y++)
        {

          //cout << "3:line " << y << endl;
          int s1 = (y > radius) ? -radius : -y;
          int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
          for (int x = X; x < w * early; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_reduce(ref, working, res, amount, gamma, chroma, x, y, s2,
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
static void gauss_HDR(HDRImage &ref, HDRImage &res,float gamma)
{
  HDRImage &working = ref;
  int radius=3;
  bool early = true;
  float amount=1.0f;
  float anti_dot=0.0f;

  int W = working.xsize(), H = working.ysize();
  int blocksize = 128 - radius * 2;

//schedule(dynamic)
#pragma omp parallel for
  for (int X = 0; (X < W) ; X += blocksize)
    {

      int w = ((X + blocksize) >= W) ? W : X + blocksize;
      int y, h = H;
      for (y = 0; (y < radius) && early; y++)
        {
          int s1 = -y;
          int e1 = radius + 1;
          for (int x = X; ((x < w) && early); x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_gauss(ref, working, res, amount, gamma, anti_dot, x, y, s2,
                        e2, s1, e1);
            }
        }

      for (; (y < (h - radius - 1)) && early; y++)
        {

          // left edge
          int x = X;
          //
          if (X == 0)
            for (; (x < radius) && early; x++)
              {
                RGB_gauss(ref, working, res, amount, gamma, anti_dot, x, y,
                          -x, radius + 1, -radius, radius + 1);
              }
          // mid part
          int end = (w == W) ? w - radius - 1 : w;
          for (; (x < end) && early; x++)
            {
              RGB_gauss(ref, working, res, amount, gamma, anti_dot, x, y,
                        -radius, radius + 1, -radius, radius + 1);
            }
          // right part
          if (w == W)
            for (; (x < w) && early; x++)
              {
                RGB_gauss(ref, working, res, amount, gamma, anti_dot, x, y,
                          -radius, w - x, -radius, radius + 1);
              }
        }
      for (; (y < h) && early; y++)
        {

          //cout << "3:line " << y << endl;
          int s1 = (y > radius) ? -radius : -y;
          int e1 = ((h - y) > (radius + 1)) ? radius + 1 : (h - y);
          for (int x = X; (x < w) && early; x++)
            {
              int s2 = (x > radius) ? -radius : -x;
              int e2 = ((W - x) > (radius + 1)) ? radius + 1 : (W - x);
              RGB_gauss(ref, working, res, amount, gamma, anti_dot, x, y, s2,
                        e2, s1, e1);
            }
        }
    }
}

void avg(HDRImage & working,HDRImage & tmp,HDRImage & dst)
{
  int W = working.xsize(), H = working.ysize();
  for (int y=0;y<H;y++)
  for (int x=0;x<W;x++)
    {
      dst[y][x].r = 0.5f *(working[y][x].r+tmp[y][x].r);
      dst[y][x].g = 0.5f *(working[y][x].g+tmp[y][x].g);
      dst[y][x].b = 0.5f *(working[y][x].b+tmp[y][x].b);

    }
}

static void RGB_denoise(HDRImage & src, improps & props)
{
  if ((bool) props.pp3["[Directional Pyramid Denoising]"]["Enabled"] != true)
    return;
  float luma = props.pp3["[Directional Pyramid Denoising]"]["Luma"];
  float chroma = props.pp3["[Directional Pyramid Denoising]"]["Chroma"];
  const float gam_in = props.pp3["[Directional Pyramid Denoising]"]["Gamma"];
  int radius = props.pp3["[Directional Pyramid Denoising]"]["Radius"];
  radius = (radius<=0)?8:radius;
  radius=radius/props.scale;
  wdt = 0.01f;//props.pp3["[Directional Pyramid Denoising]"]["Wdt"];
  offset= gam_in*0.02f;//luma;//props.pp3["[Directional Pyramid Denoising]"]["Offset"];
  if (((luma == 0.0) && (chroma == 0.0))||(radius==0))
    return;

  luma = luma * 0.01f;
  //chroma = chroma * 0.01f;
  cout << "doing noise reduction: luma " << luma << " chroma " << chroma << "gamma " << gam_in << endl;
  float lumaw = luma;// * luma;
  float chromaw = chroma* 0.01f;
  float gammaw = gam_in;
  //cout << " denoise using gamma " << gammaw << endl;

// setup size and temporary storage.
  int W = src.xsize(), H = src.ysize();
  cout << " got size " << W << " x " << H << endl;

  HDRImage temp1(W, H); //,temp2;
  HDRImage temp2(W, H);

  //gauss_HDR(src,temp2,0.01f);
#if 1
  if (props.early)
    Bilateral_HDR_Luma( src, src,temp1, radius, gammaw / 10.0f, lumaw, chromaw, props.early);
  //if (props.early) gauss_HDR(temp1,temp2,0.02f);
  //chromaw=chromaw*2.0f;
  //avg(src,temp1,temp2);
  //if (props.early)
  //		Bilateral_HDR_Luma( temp2,temp1,src, 12, gammaw / 10.0f, lumaw, chromaw,props.early);
  if (props.early) src=temp1;
#else
  if (props.early) src=temp2;
#endif

  /*		if (props.early)
  		Bilateral_HDR_Luma(src,temp1, temp2, 5, gammaw / 10.0f, lumaw, 1.0f,
  				props.early);
  	if (props.early) src=temp2;*/
  //Bilateral_HDR_Luma(temp1,src,temp1 ,1, gammaw / 10.0f, lumaw, 0.0f);
  //src=temp1;

#if 0
  Bilateral_HDR_Luma(temp1,src,temp1 ,10, gammaw / 10.0f, lumaw, 1.0f); //lumaw * 0.15f, 0.1f);
  src=temp1;
#else

#endif

}

static int enabled(improps & props)
{
  if ((bool) props.pp3["[Directional Pyramid Denoising]"]["Enabled"] != true)
    return 0;
  float luma = props.pp3["[Directional Pyramid Denoising]"]["Luma"];
  float chroma = props.pp3["[Directional Pyramid Denoising]"]["Chroma"];
  if ((luma == 0.0) && (chroma == 0.0))
    return 0;
  return 1; // yes we have work todo.
}

ADD_FILTER( RGB_denoise, HDRim, 100)
