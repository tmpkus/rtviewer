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
  pp3_found = props.read(new_name);
  if (pp3_found)
    {
      ref = new fast_demosaic (new_name);
      ref->cook_data(props);
    }
}

Image_Raw ::~Image_Raw()
{
  delete ref;
}

void Image_Raw ::demosaic(HDRImage &dest,int scale)
{
  if (scale!=ref->resize)
    ref->cook_data(props,scale);
  if (scale==1)
    switch (ref->demosaic_method())
      {
      case AMAZE_DEMOSAIC:
        ref->amaze_demosaic_RT(dest,props);
        break;
      case FAST_DEMOSAIC:
        ref->fast_demo(dest,props);
        break;
      case VARSIZE_DEMOSAIC:
        ref->nth_size_demo(dest,3,props);
        break;
      }
  else
    ref->nth_size_demo(dest,scale,props);

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
