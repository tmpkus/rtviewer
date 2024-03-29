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

#ifndef VIEWPORT_H_
#define VIEWPORT_H_
#include "../rtengine/rtengine.h"
class viewport: public Image<argb8>
{

public:
  virtual void mouse(int x,int y,int butt) {};
  virtual void key(int,int) {};
  virtual int mainloop(int delta_time)
  {
    return 1;
  };
  virtual int render(int)
  {
    return 1;
  };
  //virtual int setup(void *data){return 1;};
  viewport(char * ntitle, int width, int height);
  ~viewport(void);
  int update (void);
  int process_events (void);
  void run(void);
  unsigned int usec_delay;
  volatile int refresh,width,height,stop;

private:
  int bpp;
  std::string title;
  void * ref;
};
#define __CENTER_WINDOW__
#endif /* VIEWPORT_H_ */
