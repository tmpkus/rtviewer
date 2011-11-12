/*
 * X11 shm.h
 *
 *  Created on: Jun 11, 2011
 *      Author: janrinze
 */

#ifndef X11SHM_H_
#define X11SHM_H_

#include <string>
#include "graphic.h"

class viewport: public Image<argb8>
{

public:
	virtual void mouse(int x,int y,int butt){};
	virtual void key(int,int){};
	virtual void mainloop(int delta_time) {};
	virtual void render(){};
	viewport(char * ntitle, int width, int height);
	~viewport(void);
	int update (void);
	int process_events (void);
	void run(void);
	unsigned int usec_delay;
private:
	int width,height,bpp;
	std::string title;
	void * ref;
};
#define __CENTER_WINDOW__
#endif /* X11SHM_H_ */
