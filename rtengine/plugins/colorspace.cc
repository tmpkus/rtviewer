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
#include "../pluginhandler/plugin.h"
#include <iostream>
#include <cmath>

inline float Contrast(float i) {
    if (i < 0.5f) return i * i * 2.0f;
    i = (1 - i);
    return 1.0f - i * i * 2.0f;
}

inline float invcontrast(float i)
{
	if ( i < 0.5f) return (i>0.0)? sqrt(i)*0.707106781 :0.0;
	if (i<1.0f) return 1.0f - sqrt(1.0f -i)*0.707106781;
	return 1.0f;
}

void color_correct(HDRImage & dest,improps &props)
{
	float expcomp = props.pp3["[Exposure]"]["Compensation"];
	float bright = props.pp3["[Exposure]"]["Brightness"];
	float contrast = props.pp3["[Exposure]"]["Contrast"];
	float black = props.pp3["[Exposure]"]["Black"];

	black = 0.25f * black /65536.0f;
	bright = bright*0.001f;
	int inv=(contrast<0);
	contrast=fabs(contrast*0.01f);
	const float not_contrast = 1.0f - contrast;

	const float post_scale = pow(2.0f, expcomp)/(1.0f-black);
	float mat[3][3];
	for (int i=0;i<3;i++)
		for (int j=0;j<3;j++)
			mat[i][j]=props.mat[i][j];

	int Hgt=dest.ysize(),Wdt=dest.xsize();
	cout << "Doing color correction " << expcomp <<endl;
#pragma omp parallel for
	for (  int y = 0 ; y < Hgt ; y++ )
		for (  int x = 0 ; x < Wdt ; x++ ) {
			float r = (dest[y][x].r-black)*post_scale ;
			float g = (dest[y][x].g-black)*post_scale ;
			float b = (dest[y][x].b-black)*post_scale ;

			// adjust levels




			r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f:r);
			g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f:g);
			b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f:b);

			float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b + bright;
			float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b + bright;
			float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b + bright;

			// apply contrast
			if ((contrast >0.0f) && (inv==0))
			{
				nr= not_contrast*nr + contrast * Contrast(nr) ;
				ng= not_contrast*ng + contrast * Contrast(ng) ;
				nb= not_contrast*nb + contrast * Contrast(nb) ;
			}
			if ((contrast >0.0f) && inv)
			{
				nr= not_contrast*nr + contrast * invcontrast(nr) ;
				ng= not_contrast*ng + contrast * invcontrast(ng) ;
				nb= not_contrast*nb + contrast * invcontrast(nb) ;
			}


			// apply brightness


			dest[y][x].r = nr;//+bright;
			dest[y][x].g = ng;//+bright;
			dest[y][x].b = nb;//+bright;
		}
}

ADD_FILTER( color_correct, HDRim , 1)
