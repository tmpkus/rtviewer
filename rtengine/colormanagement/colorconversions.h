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

#ifndef COLORCONVERSIONS_H_
#define COLORCONVERSIONS_H_


#define epsilon 0.00885645f
#define kappa 903.2963f
#define di_kappa (kappa/116.0f)
#define di_add	(16.0f/116.0f)
//inverse of kappa
#define kappainv 0.00110706f
// kappa*epsilon
#define kapeps 8
#define Lab2xyz(F) (( ( temp=F*F*F) > epsilon) ? temp : (116.0f*F-16.0f)*kappainv)

#define D50x 0.96422f
#define D50z 0.82521f

#define CHAR_TO_FLOAT 0.003921568627f

static float toxyz[3][3] = { { 0.4124564, 0.3575761, 0.1804375 }, { 0.2126729, 0.7151522, 0.0721750 }, { 0.0193339, 0.1191920, 0.9503041 }};
static float torgb[3][3] = { { 3.2404542, -1.5371385, -0.4985314 }, {-0.9692660, 1.8760108, 0.0415560 }, { 0.0556434, -0.2040259, 1.0572252 }};

static const float C_Gamma = 1.0f / 2.2f;
static const float C_IGamma = 2.2f;

float igamma_fn(float index)
{
  return pow(index, C_IGamma);
}
float gamma_fn(float index)
{
  return pow(index, C_Gamma);
}
float cache_fn(float index)
{
  return (index < epsilon ? di_kappa * index + di_add : (exp(log(index) / 3.0f)));
}

static LUTf cachef(65536, cache_fn, 1.0f);
static LUTf IGamma(256, igamma_fn, 1.0f);
static LUTf Gamma(65536, gamma_fn, 1.0f);
static LUTf HIGamma(65536, igamma_fn, 1.0f);

Lab & Lab::operator =(class argb8 & rhs)
{
  float r = IGamma[rhs.r]; // inverse gamma
  float g = IGamma[rhs.g]; // inverse gamma
  float b = IGamma[rhs.b]; // inverse gamma

  // conversion is done to abide to a specific color space.
  float x = (toxyz[0][0] * r + toxyz[0][1] * g + toxyz[0][2] * b)/D50x;
  float y = (toxyz[1][0] * r + toxyz[1][1] * g + toxyz[1][2] * b);
  float z = (toxyz[2][0] * r + toxyz[2][1] * g + toxyz[2][2] * b)/D50z;

  x = cachef[x];
  y = cachef[y];
  z = cachef[z];

  // Lab is D50 colorspace.
  this->L = (116.0f * y - 16.0f); //5242.88=16.0*327.68;
  this->a = (500.0f * (x - y));
  this->b = (200.0f * (y - z));

  return *this;
}
Lab & Lab::operator =(class rgbHDR & rhs)
{

  //
  float r = rhs.r;
  float g = rhs.g;
  float b = rhs.b;
  // Linear rgb to XYZ
  float x = (toxyz[0][0] * r + toxyz[0][1] * g + toxyz[0][2] * b)/D50x;
  float y = (toxyz[1][0] * r + toxyz[1][1] * g + toxyz[1][2] * b);
  float z = (toxyz[2][0] * r + toxyz[2][1] * g + toxyz[2][2] * b)/D50z;

  x = (x<1.0f) ? cachef[x] : (exp(log(x)/3.0f ));
  y = (y<1.0f) ? cachef[y] : (exp(log(y)/3.0f ));
  z = (z<1.0f) ? cachef[z] : (exp(log(z)/3.0f ));

  this->L = (116.0f * y - 16.0f); //5242.88=16.0*327.68;
  this->a = (500.0f * (x - y));
  this->b = (200.0f * (y - z));

  return *this;
}

Lab & Lab::operator=(unsigned int & rhs)
{
  rgbHDR t;
  t = rhs;
  *this = t;
  return *this;
}
float Lab2xyz_fn(float fx)
{
  float temp;
  return 255.0 * Lab2xyz(fx);
}
;
#define argbCLIP( a ) (a>255?255:((a<0)?0:(a)))
#define argbINDX( a ) (int)(a>1.0f?4095:((a<0.0f)?0:(a*4095.0f)))

unsigned int uc_gamma_fn(float i)
{
  if (i<=0.0031308f){
      return (unsigned int)(255.0f * (12.92f*i));
  } else
    return(unsigned int)(((1.0f+0.055f)*exp(log(i)*C_Gamma)-0.055f)*255.0f);
  // float temp = 255.0f * pow(i, C_Gamma);//exp(log(i)*C_Gamma);
  // return (unsigned int) argbCLIP(temp);
}

static LUT<unsigned int> argb8_gamma(16384, uc_gamma_fn, 1.0f);
static LUTf Lab2XYZ(16384, Lab2xyz_fn, 1.0f);
float HLab2xyz_fn(float fx)
{
  float temp;
  return Lab2xyz(fx);
}

static LUTf HLab2XYZ(32768, HLab2xyz_fn, 6.0f);
argb8 & argb8::operator=(Lab & rhs)
{
  float temp;
  float fy = 0.00862069f * rhs.L + 0.137931034f; // (L+16)/116
  float fx = 0.002f * rhs.a + fy;
  float fz = fy - 0.005f * rhs.b;
  fx = HLab2XYZ[fx]* D50x;
  fy = HLab2XYZ[fy];
  fz = HLab2XYZ[fz]* D50z;
  this->a = 255;

  // XYZ to linear RGB
  float r = (torgb[0][0] * fx + torgb[0][1] * fy + torgb[0][2] * fz);
  float g = (torgb[1][0] * fx + torgb[1][1] * fy + torgb[1][2] * fz);
  float b = (torgb[2][0] * fx + torgb[2][1] * fy + torgb[2][2] * fz);

  // linear RGB to gamma corrected argb
  this->r = argb8_gamma[r];
  this->g = argb8_gamma[g];
  this->b = argb8_gamma[b];
  return *this;
}
#define ARGB8CLIP( x ) ((int)(x)>255?255:(int)(x));
argb8 & argb8::operator=(rgbHDR & rhs)
{
  this->r = argb8_gamma[rhs.r];
  this->g = argb8_gamma[rhs.g];
  this->b = argb8_gamma[rhs.b];
  this->a = 255;
  return *this;
}

rgbHDR & rgbHDR::operator=(Lab & rhs)
{

  // Lab to XYZ and restore D65
  float fy = 0.00862069f * rhs.L + 0.137931034f; // (L+16)/116
  float fx = 0.002f * rhs.a + fy;
  float fz = fy - 0.005f * rhs.b;
  fx=HLab2XYZ[fx]*D50x;
  fy=HLab2XYZ[fy];
  fz=HLab2XYZ[fz]*D50z;

  // XYZ to linear RGB

  float r = (torgb[0][0] * fx + torgb[0][1] * fy + torgb[0][2] * fz);
  float g = (torgb[1][0] * fx + torgb[1][1] * fy + torgb[1][2] * fz);
  float b = (torgb[2][0] * fx + torgb[2][1] * fy + torgb[2][2] * fz);

  this->r = r;// HIGamma[r];
  this->g = g;// HIGamma[g];
  this->b = b;// HIGamma[b];

  return *this;
}
static inline void to_YCbCr(const rgbHDR & in,LBrBb & out)
{
  float L =( 0.2990000f * in.r*in.r + 0.587000f * in.g*in.g + 0.114000f * in.b*in.b);
  float b =(-0.1687360f * in.r*in.r - 0.331264f * in.g*in.g + 0.5f      * in.b*in.b);
  float a =( 0.5f       * in.r*in.r - 0.418688f * in.g*in.g - 0.081312f * in.b*in.b);
  if (L>0.0f) b=b/L;
  if (L>0.0f) a=a/L;
  out.L=L;
  out.a=a;
  out.b=b;

}
static inline void from_YCbCr(const LBrBb & in,rgbHDR & out)
{
  float r = sqrt(in.L*(1.000f  + 0.000001f * in.b + 1.402000f * in.a));
  float g = sqrt(in.L*(1.000f  - 0.344136f * in.b - 0.714136f * in.a));
  float b = sqrt(in.L*(1.000f  + 1.772000f * in.b + 0.000000f * in.a));
  out.r = r;
  out.g = g;
  out.b = b;

}

LBrBb & LBrBb::operator =(class argb8 & rhs)
{
  rgbHDR tmp;
  tmp.r = IGamma[rhs.r]; // inverse gamma
  tmp.g = IGamma[rhs.g]; // inverse gamma
  tmp.b = IGamma[rhs.b]; // inverse gamma

  to_YCbCr(tmp,*this);

  return *this;
}
LBrBb & LBrBb::operator =(class rgbHDR & rhs)
{

  to_YCbCr(rhs,*this);

  return *this;
}

LBrBb & LBrBb::operator=(unsigned int & rhs)
{
  rgbHDR t;
  t = rhs;
  *this = t;
  return *this;
}
rgbHDR & rgbHDR::operator=(LBrBb & rhs)
{

  from_YCbCr(rhs,*this);

  return *this;
}
argb8 & argb8::operator=(LBrBb & rhs)
{
  rgbHDR t;
  from_YCbCr(rhs,t);
  *this = t;
  return *this;
}
rgbHDR & rgbHDR::operator=(argb8 & rhs)
{
  //this->a=IGamma[rhs.a]; // inverse gamma
  this->r = IGamma[rhs.r]; // inverse gamma
  this->g = IGamma[rhs.g]; // inverse gamma
  this->b = IGamma[rhs.b]; // inverse gamma
  return *this;
}

rgbHI & rgbHI::operator=(argb8 & rhs)
{
  this->a = (65535.0f * IGamma[rhs.a]); // inverse gamma
  this->r = (65535.0f * IGamma[rhs.r]); // inverse gamma
  this->g = (65535.0f * IGamma[rhs.g]); // inverse gamma
  this->b = (65535.0f * IGamma[rhs.b]); // inverse gamma
  return *this;
}

#endif
