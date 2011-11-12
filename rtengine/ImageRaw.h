/*
 * RawImage.h
 *
 *  Created on: Jun 14, 2011
 *      Author: janrinze
 */

#ifndef RAWIMAGE_H_
#define RAWIMAGE_H_
#include <string>
#include "image.h"

class Image_Raw : public Image<rgbHDR>
{
private:
	float ISO;
	void * ref;
public:
	Image_Raw(char * new_name);
	Image_Raw(char * new_name,improps properties);
	float get_ISO(void) {return ISO;};
	~Image_Raw();
	void demosaic(HDRImage &dest);
	int width();
	int height();
};

#endif /* RAWIMAGE_H_ */
