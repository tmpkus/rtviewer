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

#ifndef RAWIMAGE_H_
#define RAWIMAGE_H_
#include <string>
#include "../imageformats/image.h"

class Image_Raw : public Image<rgbHDR>
{
private:
  float ISO;
  class fast_demosaic * ref;
public:
  int pp3_found;
  improps props;
  Image_Raw(char * new_name);
  Image_Raw(char * new_name,improps properties);
  float get_ISO(void)
  {
    return ISO;
  };
  ~Image_Raw();
  void demosaic(HDRImage &dest,int scale=1);
  int width();
  int height();
};

#endif /* RAWIMAGE_H_ */
