/*
 * image.cc
 *
 *  Created on: Jun 13, 2011
 *      Author: janrinze
 */
#include <typeinfo>
#include "utils/logfile.h"
#include "image.h"
#include "colorconversions.h"

template<typename color> void Image<color>::set_pixel(int w, int h,
		unsigned int c) // create new
{
	if (((unsigned int) w < data.width()) && ((unsigned int) h < data.height()))
		data[h][w] = c;
}
template<typename color> void Image<color>::set_pixel(int w, int h, color c) // create new
{
	if (((unsigned int) w < data.width()) && ((unsigned int) h < data.height()))
		data[h][w] = c;
}
template<typename color> Image<color>::Image(int w, int h) // create new
{
	data(w, h);
}
template<typename color> Image<color>::Image(int w, int h, unsigned int * orig,
		unsigned int flgs) // create new by reference or copying
{
	if (flgs)
		data(w, h, orig, ARRAY2D_BYREFERENCE);
	else
		data(w, h, orig);
}

template<typename color> void Image<color>::operator()(int w, int h,
		color * orig, unsigned int flgs) // change by copy or reference
{
	if (flgs)
		data(w, h, orig, ARRAY2D_BYREFERENCE);
	else
		data(w, h, orig);
}
template<typename color> void Image<color>::set_ref(int w, int h, color * orig,
		unsigned int flgs) // change by copy or reference
{
	if (flgs)
		data(w, h, orig, ARRAY2D_BYREFERENCE);
	else
		data(w, h, orig);
}
template<typename color> void Image<color>::operator()(int w, int h,
		unsigned int * orig, unsigned int flgs) // change by copy or reference
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
template<typename color> void Image<color>::set_ref(int w, int h,
		unsigned int * orig, unsigned int flgs) // change by copy or reference
{
	if (flgs)
		data(w, h, orig, ARRAY2D_BYREFERENCE);
	else
		data(w, h, orig);
}

template<typename color> Image<color> & Image<color>::operator=(
		Image<color> & rhs) {
	if (this != &rhs) {
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
int Image<color>::load(std::string name) {
	logerr("image::load(std::string name) not implemented yet");
	return 0;
}
template<typename color>
int Image<color>::save(std::string name) {
	logerr("image::save(std::string name) not implemented yet");
	return 0;
}
void Lab_Denoise(LabImage &src,const float luma, const float chroma, float gam_in);

template <typename color> void Image<color>::Lab_denoise(const float luma, const float chroma, float gam_in)
{
			LabImage src(this->xsize(),this->ysize());
			int x=data.xoffset(),y=data.yoffset();
			data.moveto(0,0);
			{
				HDRImage temp;
				temp<<=*this;
				src<<=temp;
			}
			src.Lab_denoise(luma,chroma,gam_in);
			src.moveto(0,0);
			{
				HDRImage temp;
				temp<<=src;
				*this<<=temp;
			}
			data.moveto(x,y);
}
template <> void Image<Lab>::Lab_denoise(const float luma, const float chroma, float gam_in)
{
	Lab_Denoise(*this,luma,chroma,gam_in);
}
template <> void Image<rgbHDR>::Lab_denoise(const float luma, const float chroma, float gam_in)
{
	LabImage src;
	int x=data.xoffset(),y=data.yoffset();
	data.moveto(0,0);
	src<<=*this;
	src.Lab_denoise(luma,chroma,gam_in);
	src.moveto(0,0);
	*this<<=src;
	data.moveto(x,y);
}
// Instantiate myclass for the supported template type parameters

template class Image<argb8> ;
template class Image<Lab> ;
//template class Image<hsv> ;
//template class Image<rgbHI> ;
template class Image<rgbHDR> ;
template class Image<LBrBb> ;
