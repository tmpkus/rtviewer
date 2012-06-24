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

/*
 *  Declaration of flexible 2D arrays
 *
 *  Usage:
 *
 *  	array2D<type> name (X-size,Y-size);
 *		array2D<type> name (X-size,Y-size type ** data);
 *
 *		creates an array which is valid within the normal C/C++ scope "{ ... }"
 *
 *      access to elements is a simple as:
 *
 *      	array2D<float> my_array (10,10); // creates 10x10 array of floats
 *      	value =  my_array[3][5];
 *      	my_array[4][6]=value;
 *
 *      or copy an existing 2D array
 *
 *      	float ** mydata;
 *      	array2D<float> my_array (10,10,mydata);
 *
 *
 *		Useful extra pointers
 *
 *			<type> ** my_array		gives access to the pointer for access with [][]
 *			<type> *  my_array		gives access to the flat stored data.
 *
 *		Advanced usage:
 *			array2D<float> my_array				; // empty container.
 *			my_array(10,10) 					; // resize to 10x10 array
 *			my_array(10,10,ARRAY2D_CLEAR_DATA)  ; // resize to 10x10 and clear data
 *			my_array(10,10,ARRAY2D_CLEAR_DATA|ARRAY2D_LOCK_DATA)  ; same but set a lock on changes
 *
 *			!! locked arrays cannot be resized and cannot be unlocked again !!
 */
#ifndef ARRAY2D_H_
#define ARRAY2D_H_
#include <unistd.h>	// for sleep()
#include <string.h>     // for memset()
#include "logfile.h"    // for log()
#include <iostream>
using namespace std;
// flags for use
#define ARRAY2D_LOCK_DATA	1
#define ARRAY2D_CLEAR_DATA	2
#define ARRAY2D_BYREFERENCE	4
#define ARRAY2D_VERBOSE		8
#define ARRAY3D_CONTIGUOUS	16


template<typename T>
class array2D
{

private:
  unsigned int x, y, owner, flags;
  unsigned int x_owner,y_owner;
  int xoff, yoff,dxoff,dyoff;
  T * data;
  T ** ptr;
  bool lock; // useful lock to ensure data is not changed anymore.
  void ar_realloc(int sw, int sh)
  {
    unsigned int w=sw,h=sh;
    if ((ptr) && ((h > y) || (4 * h < y)))
      {
        delete[] ptr;
        ptr = NULL;
      }
    if ((data) && (((h * w) > (x * y)) || ((h * w) < ((x * y) / 4))))
      {
        delete[] data;
        data = NULL;
      }
    if (ptr == NULL)
      ptr = new T*[h];
    if (data == NULL)
      data = new T[h * w];

    x = w;
    y = h;
    for (int i = 0; i < h; i++)
      ptr[i] = data + w * i;
    owner = 1;
  }
public:

  // use as empty declaration, resize before use!
  // very useful as a member object
  array2D() : x(0), y(0), owner(1),flags(0),xoff(0),yoff(0),dxoff(0),dyoff(0), data(NULL), ptr(NULL), lock(0)
  {
    logmsg("got empty array2D init\n");
  }

  // creator type1
  array2D(int w, int h, unsigned int flgs = 0):xoff(0),yoff(0),dxoff(0),dyoff(0)
  {
    flags = flgs;
    lock = flags & ARRAY2D_LOCK_DATA;
    data = new T[h * w];
    owner = 1;
    x = w;
    y = h;
    ptr = new T*[h];
    for (int i = 0; i < h; i++)
      ptr[i] = data + i * w;
    if (flags & ARRAY2D_CLEAR_DATA)
      memset(data, 0, w * h * sizeof(T));
  }

  // creator type 2
  array2D(int w, int h, T ** source, unsigned int flgs = 0):xoff(0),yoff(0),dxoff(0),dyoff(0)
  {
    flags = flgs;
    //if (lock) { printf("array2D attempt to overwrite data\n");raise(SIGSEGV);}
    lock |= flags & ARRAY2D_LOCK_DATA;
    // when by reference
    // TODO: improve this code with ar_realloc()
    owner = (flags & ARRAY2D_BYREFERENCE) ? 0 : 1;
    if (owner)
      data = new T[h * w];
    else
      data = source[0];
    x = w;
    y = h;
    ptr = new T*[h];
    for (int i = 0; i < h; i++)
      {
        if (owner)
          {
            ptr[i] = data + i * w;
            for (int j = 0; j < w; j++)
              ptr[i][j] = source[i][j];
          }
        else
          ptr[i] = source[i];
      }
  }
  array2D(int w, int h, T * source, unsigned int flgs = 0):xoff(0),yoff(0),dxoff(0),dyoff(0)
  {
    flags = flgs;
    //if (lock) { printf("array2D attempt to overwrite data\n");raise(SIGSEGV);}
    lock |= flags & ARRAY2D_LOCK_DATA;
    // when by reference
    // TODO: improve this code with ar_realloc()
    owner = (flags & ARRAY2D_BYREFERENCE) ? 0 : 1;
    if (owner)
      data = new T[h * w];
    else
      data = NULL;
    x = w;
    y = h;
    ptr = new T*[h];
    for (int i = 0; i < h; i++)
      {
        if (owner)   // copy data?
          {
            ptr[i] = data + i * w;
            for (int j = 0; j < w; j++)
              ptr[i][j] = source[i][j];
          }
        else
          // set by reference? data is shared
          ptr[i] = &source[i * w];
      }
  }
  // destructor
  ~array2D()
  {

    if (flags & ARRAY2D_VERBOSE)
      logmsg(" deleting array2D size %dx%d \n", x, y);

    if ((owner) && (data))
      delete[] data;
    if (ptr)
      delete[] ptr;
  }
  void clear(void)
  {
    /*if (owner && x && y && data)
    {
    	memset(data,0,x*y*sizeof(T));
    }
    if ((owner==0) && (flags & ARRAY2D_BYREFERENCE))
    {*/
    if (ptr && x && y)
      for (int i=0; i<y; i++)
        memset(ptr[i],0,x*sizeof(T));
    //}
  }
  // use with indices
  T * operator[](size_t index)
  {
#ifdef _DEBUG
    if (index<0 || index>=y)
      {
        cout << "Array2d: out of bounds: " << index << " max is " << y << endl;
        return NULL;
      }
#endif
    return ptr[index];//data+index*x;
  }

  // use as pointer to T**
  operator T**()
  {
    return ptr;
  }
  /*
  	// use as pointer to data
  	operator T*() {
  		// only if owner this will return a valid pointer
  		return data;
  	}*/

  // useful within init of parent object
  // or use as resize of 2D array
  void operator()(int w, int h, unsigned int flgs = 0)
  {
    if (flags&ARRAY2D_BYREFERENCE) // we dont own the data!
      {
        if (((unsigned int)w<x_owner)&&((unsigned int)h<y_owner))
          {
            x=w;
            y=h; // setup required cutout
          }
        else
          {
            x=x_owner;
            y=y_owner;
          }
        return;
      }
    flags = flgs;
    if (flags & ARRAY2D_VERBOSE)
      {
        logmsg("got init request %dx%d flags=%d\n", w, h, flags);
        logmsg("previous was data %p ptr %p \n", data, ptr);
      }
    if (lock) // our object was locked so don't allow a change.
      {
        logmsg("got init request but object was locked!\n");
        //raise( SIGSEGV);
      }
    lock = flags & ARRAY2D_LOCK_DATA;

    ar_realloc(w, h);
    if (flags & ARRAY2D_CLEAR_DATA)
      memset(data, 0, w * h * sizeof(T));
  }

  // import from flat data
  void operator()(int w, int h, T* copy, unsigned int flgs = 0)
  {
    flags = flgs;
    if (flags & ARRAY2D_VERBOSE)
      {
        logmsg("got init request %dx%d flags=%d\n", w, h, flags);
        logmsg("previous was data %p ptr %p \n", data, ptr);
      }
    if (lock) // our object was locked so don't allow a change.
      {
        logmsg("got init request but object was locked!\n");
        //raise( SIGSEGV);
      }
    lock = flags & ARRAY2D_LOCK_DATA;

    ar_realloc(w, h);
    memcpy(data, copy, w * h * sizeof(T));
  }
  void operator()(int w, int h, unsigned int * source, unsigned int flgs = 0)
  {
    if ((owner) && (flgs & ARRAY2D_BYREFERENCE))
      {
        delete[] data;
        data = NULL;
      }
    flags = flgs;
    //if (lock) { printf("array2D attempt to overwrite data\n");raise(SIGSEGV);}
    lock |= flags & ARRAY2D_LOCK_DATA;
    // when by reference
    // TODO: improve this code with ar_realloc()
    owner = (flags & ARRAY2D_BYREFERENCE) ? 0 : 1;
    if (owner)
      ar_realloc(w, h);
    else
      {
        if (h > y)
          {
            delete[] ptr;
            ptr = NULL;
          }
        if (ptr == NULL)
          ptr = new T*[h];
        data = (T*) &source[0];
      }
    x = w;
    y = h;

    for (int i = 0; i < h; i++)
      {
        if (owner)   // copy data?
          {
            ptr[i] = data + i * w;
            unsigned int * s = source + i * w;
            for (int j = 0; j < w; j++)
              ptr[i][j] = s[j];
          }
        else
          // set by reference? data is shared
          ptr[i] = (T*) &source[i * w];
      }
  }

  unsigned int width()
  {
    return x;
  }
  unsigned int height()
  {
    return y;
  }
  int xoffset()
  {
    return xoff;
  }
  int yoffset()
  {
    return yoff;
  }

  operator bool()
  {
    return (x > 0 && y > 0);
  }

  template<typename N>
  array2D<T> & operator=(array2D<N> & rhs)
  {
    if ((void*)this != (void*)&rhs)

      {
        unsigned int nx=rhs.width(),ny=rhs.height();
        if (!owner) // we can only copy same size data
          {
            if ((x != nx) || (y != ny))
              {
                logmsg(" assignment error in array2D\n");
                logmsg(" sizes differ and not owner\n");
                //raise( SIGSEGV);
                // prefer to clip
                x = (x > nx) ? nx : x;
                y = (y > ny) ? ny : y;
              }

          }
        else
          {
            ar_realloc(nx, ny);
          }
        // we could have been created from a different
        // array format where each row is created by 'new'
#pragma omp for
        for (int i = 0; i < y; i++)
          {
            N * src=rhs[i];
            T * dst=data+i*x;

            for (int j = 0; j < x; j++)
              dst[j] = src[j];//,x*sizeof(T));
          }
      }
    return *this;
  }

  //  a<<=b will result in placing b with offset in a.
  template<typename N>
  array2D<T> & operator<<=(array2D<N> & rhs)
  {
    if ((void*)this != (void*)&rhs)

      {
        // we can't rely to have access to the private variables
        int nx=rhs.width(),ny=rhs.height();
        int nxoff=rhs.xoffset(),nyoff=rhs.yoffset();
        // empty non ref image will fill to full size.
        if ((this->x == 0) && (this->y == 0) && ((this->flags&ARRAY2D_BYREFERENCE)==0))
          {
            ar_realloc(nx, ny);
            this->xoff = nxoff;
            this->yoff = nyoff;
          }
        // we could have been created from a different
        // array format where each row is created by 'new'

        int yss = 0, yse = ny;
        int yds = 0, yde = y;
        if (yse + nyoff > yde)
          yse = yde - nyoff;
        else
          yde = yse + nyoff; // cut off outside
        if (nyoff > 0)
          yds = nyoff;
        else
          yss = -nyoff; // cut off top
        yss -= yds;
        if (yse > yss)
          {
            int xss = 0, xds = 0;
            int xse = nx, xde = x;
            if (xse + nxoff > xde)
              xse = xde - nxoff;
            else
              xde = xse + nxoff; // cut off outside
            if (nxoff > 0)
              xds = nxoff;
            else
              xss = -nxoff; // cut off top

            if (xde > xds)

#pragma omp parallel for
              for (int i = yds; i < yde; i++)
                {
                  int k = i + yss;
                  N * src=rhs[k];
                  T * dst=data+i*x;
                  for (int j = xds, l = xss; j < xde; j++, l++)
                    dst[j] = src[l];
                }
          }
      }
    return *this;
  }
  void moveto(int nx, int ny)
  {
    xoff = nx;
    yoff = ny;
  }
  ;
  void move(int nx, int ny)
  {
    xoff += nx;
    yoff += ny;
  }
  ;

};
template<typename T>
class array3D
{
private:
  unsigned int num;
  array2D<T> *list;

public:
  array3D(int x, int y, int z, int flags = 0)
  {
    list = new array2D<T> [z];
    if (list==NULL) return;
    if (flags&ARRAY3D_CONTIGUOUS)
      {
        T * data = new T[x*y*z];
        if (data==NULL) return;
        for (int i = 0; i < num; i++)
          {
            list[i](x, y,data+i*x*y, flags | ARRAY2D_VERBOSE| ARRAY2D_BYREFERENCE);
          }
      }
    num=z;
    for (int i = 0; i < num; i++)
      {
        list[i](x, y, flags | ARRAY2D_VERBOSE);
      }
  }
  array3D(int x, int y, int z,T*data, int flags = 0)
  {
    list = new array2D<T> [z];
    if (list==NULL) return;
    if (flags&ARRAY3D_CONTIGUOUS)
      {
        if (data==NULL) return;
        for (int i = 0; i < num; i++)
          {
            list[i](x, y,data+i*x*y, flags | ARRAY2D_VERBOSE| ARRAY2D_BYREFERENCE);
          }
      }
    num=z;
    for (int i = 0; i < num; i++)
      {
        list[i](x, y, flags | ARRAY2D_VERBOSE);
      }
  }
  ~array3D()
  {
    logmsg("trying to delete the list of array2D objects\n");
    delete [] list;
  }

  array2D<T> & operator[](size_t index)
  {
    if (index < 0 || index >= num)
      {
        logmsg("index %d is out of range[0..%d]", index, num - 1);
        //raise( SIGSEGV);
      }
    return list[index];
  }
};
#endif /* array2D_H_ */
