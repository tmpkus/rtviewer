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
#include <typeinfo>
#include "../utils/logfile.h"
#include "image.h"
#include "../colormanagement/colorconversions.h"

template<typename color> void Image<color>::set_pixel(int w, int h, unsigned int c)
{
  if (((unsigned int) w < data.width()) && ((unsigned int) h < data.height()))
    data[h][w] = c;
}
template<typename color> void Image<color>::set_pixel(int w, int h, color c)
{
  if (((unsigned int) w < data.width()) && ((unsigned int) h < data.height()))
    data[h][w] = c;
}
template<typename color> Image<color>::Image(int w, int h)
{
  data(w, h);
}
template<typename color> Image<color>::Image(int w, int h, unsigned int * orig, unsigned int flgs) // create new by reference or copying
{
  if (flgs)
    data(w, h, orig, ARRAY2D_BYREFERENCE);
  else
    data(w, h, orig);
}

template<typename color> void Image<color>::operator()(int w, int h, color * orig, unsigned int flgs) // change by copy or reference
{
  if (flgs)
    data(w, h, orig, ARRAY2D_BYREFERENCE);
  else
    data(w, h, orig);
}
template<typename color> void Image<color>::set_ref(int w, int h, color * orig, unsigned int flgs) // change by copy or reference
{
  if (flgs)
    data(w, h, orig, ARRAY2D_BYREFERENCE);
  else
    data(w, h, orig);
}
template<typename color> void Image<color>::operator()(int w, int h, unsigned int * orig, unsigned int flgs) // change by copy or reference
{
  if (flgs)
    data(w, h, orig, ARRAY2D_BYREFERENCE);
  else
    data(w, h, orig);
}
template<typename color> void Image<color>::operator()(int w, int h) // change by copy or reference
{
  data(w, h);
}
template<typename color> void Image<color>::set_ref(int w, int h, unsigned int * orig, unsigned int flgs) // change by copy or reference
{
  if (flgs)
    data(w, h, orig, ARRAY2D_BYREFERENCE);
  else
    data(w, h, orig);
}

template<typename color> Image<color> & Image<color>::operator=( Image<color> & rhs)
{
  if (this != &rhs)
    {
      data = rhs.data;
    }
  return *this;
}
/*
 template <typename color,typename othercolor>
 image<color> &  image<color>::operator=(image<othercolor> & rhs) {
 data = rhs.data; // this should auto-magically do color conversion
 return *this;
 }

 template <typename color> template <typename othercolor>
 image<color> & image<color>::operator<<=(image<othercolor> & rhs) {
 data <<= rhs.data; // this should auto-magically do color conversion
 return *this;
 }
*/

template<typename color>
int Image<color>::load(std::string name)
{
  logerr("image::load(std::string name) not implemented yet");
  return 0;
}
template<typename color>
int Image<color>::save(std::string name)
{
  logerr("image::save(std::string name) not implemented yet");
  return 0;
}

// Instantiate the classes for the supported template type parameters

template class Image<argb8> ;
template class Image<Lab> ;
//template class Image<hsv> ;
//template class Image<rgbHI> ;
template class Image<rgbHDR> ;
template class Image<LBrBb> ;
