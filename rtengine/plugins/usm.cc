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

// image formats etc.
#include <iostream>
using namespace std;
#include "../imageformats/image.h"
#include "filtermodule.h"

#define MINMAX3(a,b,c,min,max) \
{ \
if ((a)<(b)) { \
  if ((b)<(c)) { \
    (min) = (a); \
    (max) = (c); \
  } \
  else { \
    (max) = (b); \
    if ((a)<(c)) \
      (min) = (a); \
    else \
      (min) = (c); \
  } \
} else { \
  if ((b)>(c)) { \
    (min) = (c); \
    (max) = (a); \
  } \
  else { \
    (min) = (b); \
    if ((a)>(c)) \
      (max) = (a); \
    else \
      (max) = (c); \
  } \
} \
}

#define MIN3(a,b,c,min) \
{ \
if ((a)<(b)) { \
  if ((a)<(c))  \
    (min) = (a); \
  else  \
    (min) = (c); \
} else { \
  if ((b)>(c))  \
    (min) = (c); \
  else \
    (min) = (b); \
} \
} 

#define MAX3(a,b,c,min) \
{ \
if ((a)>(b)) { \
  if ((a)>(c))  \
    (max) = (a); \
  else  \
    (max) = (c); \
} else { \
  if ((b)<(c))  \
    (max) = (c); \
  else \
    (max) = (b); \
} \
} 

// classical filtering if the support window is small:

template <class T> void gaussHorizontal3 (T & src, T & dst, const float c0, const float c1) {

	float * buffer=NULL;
	int H=dst.ysize(),W=dst.xsize();
	if (&dst==&src) buffer=new float [W];
#ifdef _OPENMP
#pragma omp for
#endif
    for (int i=0; i<H; i++) {
    	if (buffer)
    	{
			for (int j=1; j<W-1; j++)
				buffer[j] = (c1 * (src[i][j-1].L + src[i][j+1].L) + c0 * src[i][j].L);
			for (int j=1; j<dst.xsize()-1; j++)
				dst[i][j].L = buffer[j];
			}
        else    
			for (int j=1; j<dst.xsize()-1; j++)
				dst[i][j].L = (c1 * (src[i][j-1].L + src[i][j+1].L) + c0 * src[i][j].L);
        dst[i][0].L = src[i][0].L;
        dst[i][W-1].L = src[i][W-1].L;
    }
    if (buffer) delete [] buffer;
}

template <class T> void gaussVertical3 (T & src, T & dst, const float c0, const float c1) {
    
	float * buffer=NULL;
	int H=dst.ysize(),W=dst.xsize();
	if (&dst==&src) buffer=new float [H];
#ifdef _OPENMP
#pragma omp for
#endif
    for (int i=0; i<W; i++) {
    	if (buffer)
    	{
			for (int j = 1; j<H-1; j++)
				buffer[j] = (c1 * (src[j-1][i].L + src[j+1][i].L) + c0 * src[j][i].L);
			for (int j = 1; j<H-1; j++)
				dst[j][i].L=buffer[j];
		}
		else
			for (int j = 1; j<H-1; j++)
				dst[j][i].L = (c1 * (src[j-1][i].L + src[j+1][i].L) + c0 * src[j][i].L);
			
        dst[0][i].L = src[0][i].L;
        dst[H-1][i].L = src[H-1][i].L;
    }
}

// fast gaussian approximation if the support window is large

template <class T> void gaussHorizontal (T & src, T & dst, float sigma) {

    if (sigma<0.25) {
        // dont perform filtering
        if (&src!=&dst)
			dst<<=src;
        return;
    }

    if (sigma<0.6) {
        // compute 3x3 kernel
        float c1 = exp (-1.0 / (2.0 * sigma * sigma));
        float csum = 2.0 * c1 + 1.0;
        c1 /= csum;
        float c0 = 1.0 / csum;
        gaussHorizontal3(src, dst, c0, c1);
        return;
    }

    // coefficient calculation
    float q = 0.98711 * sigma - 0.96330;
    if (sigma<2.5)
        q = 3.97156 - 4.14554 * sqrt (1.0 - 0.26891 * sigma);
    float b0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
    float b1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
    float b2 = -1.4281*q*q - 1.26661*q*q*q;
    float b3 = 0.422205*q*q*q;
    float B = 1.0 - (b1+b2+b3) / b0;

    b1 /= b0;
    b2 /= b0;
    b3 /= b0;

    // From: Bill Triggs, Michael Sdika: Boundary Conditions for Young-van Vliet Recursive Filtering
    float M[3][3];
    M[0][0] = -b3*b1+1.0-b3*b3-b2;
    M[0][1] = (b3+b1)*(b2+b3*b1);
    M[0][2] = b3*(b1+b3*b2);
    M[1][0] = b1+b3*b2;
    M[1][1] = -(b2-1.0)*(b2+b3*b1);
    M[1][2] = -(b3*b1+b3*b3+b2-1.0)*b3;
    M[2][0] = b3*b1+b2+b1*b1-b2*b2;
    M[2][1] = b1*b2+b3*b2*b2-b1*b3*b3-b3*b3*b3-b3*b2+b3;
    M[2][2] = b3*(b1+b3*b2);
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            M[i][j] /= (1.0+b1-b2+b3)*(1.0+b2+(b1-b3)*b3);
            
    float * buffer;
    int H=dst.ysize(),W=dst.xsize();
	buffer = new float [W];

	#pragma omp for
    for (int i=0; i<H; i++) {
        
        buffer[0] = B * src[i][0].L + b1*src[i][0].L + b2*src[i][0].L + b3*src[i][0].L;
        buffer[1] = B * src[i][1].L + b1*buffer[0]  + b2*src[i][0].L + b3*src[i][0].L;
        buffer[2] = B * src[i][2].L + b1*buffer[1]  + b2*buffer[0]  + b3*src[i][0].L;

        for (int j=3; j<W; j++)
            buffer[j] = B * src[i][j].L + b1*buffer[j-1] + b2*buffer[j-2] + b3*buffer[j-3];

        float bufferWm1 = src[i][W-1].L + M[0][0]*(buffer[W-1] - src[i][W-1].L) + M[0][1]*(buffer[W-2] - src[i][W-1].L) + M[0][2]*(buffer[W-3] - src[i][W-1].L);
        float bufferW   = src[i][W-1].L + M[1][0]*(buffer[W-1] - src[i][W-1].L) + M[1][1]*(buffer[W-2] - src[i][W-1].L) + M[1][2]*(buffer[W-3] - src[i][W-1].L);
        float bufferWp1 = src[i][W-1].L + M[2][0]*(buffer[W-1] - src[i][W-1].L) + M[2][1]*(buffer[W-2] - src[i][W-1].L) + M[2][2]*(buffer[W-3] - src[i][W-1].L);

        buffer[W-1] = bufferWm1;
        buffer[W-2] = B * buffer[W-2] + b1*buffer[W-1] + b2*bufferW + b3*bufferWp1;
        buffer[W-3] = B * buffer[W-3] + b1*buffer[W-2] + b2*buffer[W-1] + b3*bufferW;

        for (int j=W-4; j>=0; j--)
            buffer[j] = B * buffer[j] + b1*buffer[j+1] + b2*buffer[j+2] + b3*buffer[j+3];
        for (int j=0; j<W; j++)
            dst[i][j].L = buffer[j];
    }
    delete [] buffer;
}

template <class T> void gaussVertical (T & src, T & dst, float sigma) {

    if (sigma<0.25) {
        // dont perform filtering
        if (&src!=&dst)
			dst<<=src;
        return;
    }

    if (sigma<0.6) {
        // compute 3x3 kernel
        float c1 = exp (-1.0 / (2.0 * sigma * sigma));
        float csum = 2.0 * c1 + 1.0;
        c1 /= csum;
        float c0 = 1.0 / csum;
        gaussVertical3 (src, dst, c0, c1);
        return;
    }

    // coefficient calculation
    float q = 0.98711 * sigma - 0.96330;
    if (sigma<2.5)
        q = 3.97156 - 4.14554 * sqrt (1.0 - 0.26891 * sigma);
    float b0 = 1.57825 + 2.44413*q + 1.4281*q*q + 0.422205*q*q*q;
    float b1 = 2.44413*q + 2.85619*q*q + 1.26661*q*q*q;
    float b2 = -1.4281*q*q - 1.26661*q*q*q;
    float b3 = 0.422205*q*q*q;
    float B = 1.0 - (b1+b2+b3) / b0;

    b1 /= b0;
    b2 /= b0;
    b3 /= b0;

    // From: Bill Triggs, Michael Sdika: Boundary Conditions for Young-van Vliet Recursive Filtering
    float M[3][3];
    M[0][0] = -b3*b1+1.0-b3*b3-b2;
    M[0][1] = (b3+b1)*(b2+b3*b1);
    M[0][2] = b3*(b1+b3*b2);
    M[1][0] = b1+b3*b2;
    M[1][1] = -(b2-1.0)*(b2+b3*b1);
    M[1][2] = -(b3*b1+b3*b3+b2-1.0)*b3;
    M[2][0] = b3*b1+b2+b1*b1-b2*b2;
    M[2][1] = b1*b2+b3*b2*b2-b1*b3*b3-b3*b3*b3-b3*b2+b3;
    M[2][2] = b3*(b1+b3*b2);
    for (int i=0; i<3; i++)
        for (int j=0; j<3; j++)
            M[i][j] /= (1.0+b1-b2+b3)*(1.0+b2+(b1-b3)*b3);
            
    int H=dst.ysize(),W=dst.xsize();
    float* temp2 = new float [H];
#ifdef _OPENMP
#pragma omp for
#endif
    for (int i=0; i<W; i++) {
        
    	temp2[0] = B * src[0][i].L + b1*src[0][i].L + b2*src[0][i].L + b3*src[0][i].L;
        temp2[1] = B * src[1][i].L + b1*temp2[0]  + b2*src[0][i].L + b3*src[0][i].L;
        temp2[2] = B * src[2][i].L + b1*temp2[1]  + b2*temp2[0]  + b3*src[0][i].L;

        for (int j=3; j<H; j++)
            temp2[j] = B * src[j][i].L + b1*temp2[j-1] + b2*temp2[j-2] + b3*temp2[j-3];

        float temp2Hm1 = src[H-1][i].L + M[0][0]*(temp2[H-1] - src[H-1][i].L) + M[0][1]*(temp2[H-2] - src[H-1][i].L) + M[0][2]*(temp2[H-3] - src[H-1][i].L);
        float temp2H   = src[H-1][i].L + M[1][0]*(temp2[H-1] - src[H-1][i].L) + M[1][1]*(temp2[H-2] - src[H-1][i].L) + M[1][2]*(temp2[H-3] - src[H-1][i].L);
        float temp2Hp1 = src[H-1][i].L + M[2][0]*(temp2[H-1] - src[H-1][i].L) + M[2][1]*(temp2[H-2] - src[H-1][i].L) + M[2][2]*(temp2[H-3] - src[H-1][i].L);

        temp2[H-1] = temp2Hm1;
        temp2[H-2] = B * temp2[H-2] + b1*temp2[H-1] + b2*temp2H + b3*temp2Hp1;
        temp2[H-3] = B * temp2[H-3] + b1*temp2[H-2] + b2*temp2[H-1] + b3*temp2H;
        
        for (int j=H-4; j>=0; j--)
            temp2[j] = B * temp2[j] + b1*temp2[j+1] + b2*temp2[j+2] + b3*temp2[j+3];
        
        for (int j=0; j<H; j++)
            dst[j][i].L = temp2[j];
    }
    delete [] temp2;
}   
/*
void sharpen (LBrBbImage & dst,improps &props)
{
	if ((bool) props.pp3["[Sharpening]"]["Enabled"] != true) return;
	float radius = props.pp3["[Sharpening]"]["Radius"];
	float amount = props.pp3["[Sharpening]"]["Amount"];
	float thresh = props.pp3["[Sharpening]"]["Threshold"];
	if (amount==0.0) return;
	amount=amount* 0.01f;
	LBrBbImage temp;
	temp <<=dst;
	int H=dst.ysize(),W=dst.xsize();
#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        gaussHorizontal(dst, temp, radius);
        gaussVertical(temp, temp, radius);
		#pragma omp for
    	for (int i=0; i<H; i++)
            for (int j=0; j<W; j++) {
                float diff = dst[i][j].L - temp[i][j].L;
                if (diff>thresh||diff<-thresh) {
                    dst[i][j].L= dst[i][j].L + amount * diff;
                    
                }
            }
	}
}*/
void sharpen (LabImage & dst,improps &props)
{
	if ((bool) props.pp3["[Sharpening]"]["Enabled"] != true) return;
	float radius = props.pp3["[Sharpening]"]["Radius"];
	float amount = props.pp3["[Sharpening]"]["Amount"];
	float thresh = props.pp3["[Sharpening]"]["Threshold"];
	if (amount==0.0) return;
	cout << "sharpen filter: radius" << radius << " amount "<< amount << " threshold "<< thresh << endl;
	amount=amount* 0.01f;
	thresh=thresh*0.01f;
	LabImage temp;
	temp <<=dst;
	int H=dst.ysize(),W=dst.xsize();
#ifdef _OPENMP
#pragma omp parallel
#endif
    {
        gaussHorizontal(dst, temp, radius);
        gaussVertical(temp, temp, radius);
		#pragma omp for
    	for (int i=0; i<H; i++)
            for (int j=0; j<W; j++) {
                float diff = dst[i][j].L - temp[i][j].L;
                if (diff>thresh||diff<-thresh) {
                    dst[i][j].L= dst[i][j].L + amount * diff;
                }
            }
	}
}
/*
void sharpen (HDRImage & dst,improps &props) {
	LBrBbImage temp;
	temp <<=dst;
	sharpen (temp, props);
	dst <<= temp;
}
*/
ADD_FILTER( sharpen, Labim , 100)
