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

#include "ImageRaw.h"
#include "fast_demo.h"

Image_Raw ::Image_Raw(char * new_name)
{
	fast_demosaic *thisraw = new fast_demosaic (new_name);

	ref = (void *)thisraw;
	thisraw->cook_data();
	//ISO=myraw.iso_speed;
	//if(myraw.success) myraw.fast_demo(*this);
	//if(myraw.success) myraw.amaze_demosaic_RT(*this);
	//if(myraw.success) myraw.jrp_demo(*this);
}
Image_Raw ::Image_Raw(char * new_name, improps props)
{
	fast_demosaic *thisraw = new fast_demosaic (new_name,props);
	ref = (void *)thisraw;
	thisraw->cook_data();
	//ISO=myraw.iso_speed;
	//if(myraw.success) myraw.fast_demo(*this);
	//if(myraw.success) myraw.amaze_demosaic_RT(*this);
	//if(myraw.success) myraw.jrp_demo(*this);
}
Image_Raw ::~Image_Raw()
{
	fast_demosaic *thisraw =(fast_demosaic *)ref;
	delete thisraw;
}

void Image_Raw ::demosaic(HDRImage &dest)
{
	fast_demosaic *thisraw =(fast_demosaic *)ref;
	switch (thisraw->demosaic_method())
	{
	case AMAZE_DEMOSAIC:
		thisraw->amaze_demosaic_RT(dest);
		break;
	case FAST_DEMOSAIC:
		thisraw->fast_demo(dest);
		break;
	case JRP_DEMOSAIC:
		thisraw->jrp_demo(dest);
		break;
	case HALFSIZE_DEMOSAIC:
		thisraw->half_size_demo(dest);
		break;
	case VARSIZE_DEMOSAIC:
		thisraw->nth_size_demo(dest,3);
		break;
	}

}
int Image_Raw ::width()
{
	fast_demosaic *thisraw =(fast_demosaic *)ref;
	return thisraw->get_width();
}
int Image_Raw ::height()
{
	fast_demosaic *thisraw =(fast_demosaic *)ref;
	return thisraw->get_height();
}
