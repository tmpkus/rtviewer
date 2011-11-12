/*
 *  This file is part of RTviewer.
 *
 *  Copyright (c) 2011 Jan Rinze Peterzon (janrinze@gmail.com)
 *
 */
/*
 * DWMviewport.h
 *
 *  Created on: Jun 11, 2011
 *      Author: janrinze
 */

#ifndef VIEWPORT_H_
#define VIEWPORT_H_

#include "../rtengine/image.h"
#include "../rtengine/ImageRaw.h"
#include "../rtengine/improps.h"
#include "../rtengine/usm.h"

class viewport: public Image<argb8>
{

public:
	virtual void mouse(int x,int y,int butt){};
	virtual void key(int,int){};
	virtual int mainloop(int delta_time) {return 1;};
	virtual int render(int){return 1;};
	virtual int setup(void *data){return 1;};
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
#endif /* VIEWPORT_H_ */
