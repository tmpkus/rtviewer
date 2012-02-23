/*
 * Lab_adjustments.cc
 *
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
#include "../utils/LUT.h"
#include "../imageformats/image.h"
#include "../processing/improps.h"
#include <iostream>
#include "filtermodule.h"
using namespace std;

#ifdef _OPENMP
#include <omp.h>
#endif

float lcurve(float i,float * params)
{
	float brightness = params[0];
	float Contrast = params[1];
	float Saturation = params[2];

	i=

	return i*brightness

}


void Lab_curves (LabImage & lab, improps & props)
{
	//if ((bool) props.pp3["[Luminance Curve]"]["Enabled"] != true) return;
	float brightness = props.pp3["[Luminance Curve]"]["Brightness"] * 0.01f;
	float Contrast = props.pp3["[Luminance Curve]"]["Contrast"] * 0.01f;
	float Saturation = props.pp3["[Luminance Curve]"]["Saturation"] * 0.01f;

	float parms[3] = { brightness,Contrast,Saturation};
	// devide into 4096 steps with full scale is 100.0f
	LUTf Lcurve(4096, lcurve,100.0f);
	LUTf Ccurve(8192, ccurve,100.0f);

	if (brightness>0.0f)
	{
		//void ImProcFunctions::luminanceCurve (LabImage* lold, LabImage* lnew, LUTf & curve) {

		    int W = lold->W;
		    int H = lold->H;
		    for (int i=0; i<H; i++)
		        for (int j=0; j<W; j++) {
					float Lin=lold->L[i][j];
						lnew->L[i][j] = Lcurve[Lin];
					float ain=lold->a[i][j];
					float bin=lold->b[i][j];
					lnew->a[i][j] = Ccurve[ain+32768.0f]-32768.0f;
					lnew->b[i][j] = Ccurve[bin+32768.0f]-32768.0f;
				}
		}
	}

}

