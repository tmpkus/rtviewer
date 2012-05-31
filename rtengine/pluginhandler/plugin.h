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

#ifndef FILTERMODULE_H_
#define FILTERMODULE_H_
#include "../processing/improps.h"
#include "../imageformats/image.h"

/*
#define ARGBImage Image<argb8>
#define LabImage Image<Lab>
#define HSVImage Image<hsv>
#define HIImage Image<rgbHI>
#define HDRImage Image<rgbHDR>
#define LBrBbImage Image<LBrBb>
*/
typedef enum image_type_t
{
	Labim,
	HSVim,
	HDRim,
	LBrBbim,
	ARGBim,
}image_type;
 typedef struct module_t {
	 char * name;
	 image_type type;
	 int rank;
	 void(* fLabim) (LabImage &a,improps &b);
	 void(* fHSVim) (HSVImage &a,improps &b);
	 void(* fHDRim) (HDRImage &a,improps &b);
	 void(* fLBrBbim) (LBrBbImage &a,improps &b);
	 void(* fARGBim) (ARGBImage &a,improps &b);

	 struct module_t * next;
 } module;


 void list_filters(void);
 void addmodule(module & moduleinfo);
 module * get_filters();

#define ADD_FILTER( fn, inputtype , myrank) \
static module moduleinfo; \
static int setmoduleinfo(void) \
{ \
	moduleinfo.name=#fn ; \
	moduleinfo.type = inputtype ; \
	moduleinfo.rank= myrank; \
	moduleinfo.f##inputtype = fn; \
	addmodule(moduleinfo); \
   return 1; \
}; \
static int filterinit = setmoduleinfo();
#endif
