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
#include "processfilters.h"
#include <iostream>
using namespace std;

void apply_filters(HDRImage & im, improps & props, int max)
{
  image_type cur = HDRim;
  LabImage L(im.xsize(),im.ysize());
  L.moveto(0,0);

  /* -- HSV is broken , needs fixing --
  HSVImage H;//(im.xsize(),im.ysize());
  H.moveto(0,0);
  */

  //list_filters();
  module * list = get_filters();

  while (list && list->rank<=max && props.early)
    {

      // convert current type to desired type
      // if necessary

      // while converting we don't use '<<=' but '='
      // which ensures we can resize or rotate the original too.

      // HSV support is broken so we comment it

      cout << "adapt imagetype" << endl;
      if (cur != list->type)
        {
          switch (cur)
            {
            case HDRim:
              cout << "adapt imagetype from HDR" << endl;
              switch (list->type)
                {
                case Labim:
                  L = im;
                  cur = Labim;
                  break;
                  /* -- HSV is broken , needs fixing --
                  case HSVim:
                  	//H <<= im;
                  	cur = HSVim;
                  	break;*/
                case HDRim:
                default: // nothing to do
                  break;
                }
            case Labim:
              switch (list->type)
                {
                case Labim:
                  break;
                  /* -- HSV is broken , needs fixing --
                  case HSVim:
                  	H = L;
                  	cur = HSVim;
                  	break; */
                case HDRim:
                  im = L;
                  cur = HDRim;
                  break;
                default: // nothing to do
                  break;
                }
              /* -- HSV is broken , needs fixing --
              case HSVim:
              	switch (list->type) {
              	case Labim:
              		//L <<= H;
              		cur = Labim;
              		break;
              	case HSVim:
              		break;
              	case HDRim:
              		cur = HDRim;
              		//im <<= H;
              		break;
              	default: // nothing to do
              		break;
              	}
              	*/
            }
        }

      // call function with current type.
      cout << "applying filter: " << list->name << endl;
      switch (list->type)
        {
        case Labim:
          list->fLabim(L, props);
          break;
          /* -- HSV is broken , needs fixing --
          case HSVim:
          	break;*/
        case HDRim:
          list->fHDRim(im, props);
          break;
        default: // nothing to do
          break;
        }
      list = list->next;
    }
  cout << "convert back imagetype" << endl;

  // convert back to HDRImage.
  // since that is where we started.
  switch (cur)
    {
    case HDRim:
      break;
    case Labim:
      im = L;
      break;
      /* -- HSV is broken , needs fixing --
      case HSVim:
      	im <<= H;
      	break;*/
    default: // nothing to do
      break;
    }
}
