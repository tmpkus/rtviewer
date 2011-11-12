/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2004-2010 Gabor Horvath <hgabor@rawtherapee.com>
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
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
 *  along with RawTherapee.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _RAWIMAGESOURCE_
#define _RAWIMAGESOURCE_

//#include <imagesource.h>
#ifdef USE_LCMS2
#include <lcms2.h>
#endif

#include "../utils/array2D.h"
#include "rawimage.h"
#define HR_SCALE 2


/*
template<class T> void freeArray(T** a, int H) {
	//for (int i=0; i<H; i++)
	delete[] a[0];
	delete[] a;
}

template<class T> T** allocArray(int W, int H) {

	T** t = new T*[H];
	t[0] = new T[H * W];
	for (int i = 1; i < H; i++)
		t[i] = t[i - 1] + W;
	return t;
}

template<class T> void freeArray2(T** a, int H) {
	//for (int i=0; i<H; i++)
	delete[] a[0];
}
*/
class RawImageSource: public RawImage {

protected:
	//Glib::Mutex isrcMutex;

	int W, H;
	//ColorTemp wb;
	//ProgressListener* plistener;
	float scale_mul[4]; // multiplier for each color
	int cblack[4]; // black offsets
	float camwb_red;
	float camwb_green;
	float camwb_blue;
	float rgb_cam[3][3];
	float cam_rgb[3][3];
	float xyz_cam[3][3];
	float cam_xyz[3][3];
	bool fuji;
	bool d1x;
	int border;
	array2D<char> hpmap;
	array2D<float> hrmap[3]; // for color propagation
	array2D<char> needhr; // for color propagation
	int max[3];
	float initialGain; // initial gain calculated after scale_colors
	float defGain;
	//int blcode[16][16][32];  // Looks like it's an unused variable...
	bool full;
	/*
	cmsHPROFILE camProfile;
	cmsHPROFILE embProfile;
	*/
	RawImage* ri; // Copy of raw pixels

	// to accelerate CIELAB conversion:
	float lc00, lc01, lc02, lc10, lc11, lc12, lc20, lc21, lc22;
	float* cache;
	int threshold;

	array2D<float> rawData;
	//float ** rawData; // holds pixel values, data[i][j] corresponds to the ith row and jth column

	// the interpolated green plane:
	//float** green;
	array2D<float> green;
	// the interpolated red plane:
	//float** red;
	array2D<float> red;
	// the interpolated blue plane:
	//float** blue;
	array2D<float> blue;
/* these don't belong here
	void hphd_vertical(array2D<float> &hpmap, int col_from, int col_to);
	void hphd_horizontal(array2D<float> &hpmap, int row_from, int row_to);
	void hphd_green();
	void correction_YIQ_LQ_(Imagefloat* im, int row_from, int row_to);
	void hlRecovery(std::string method, float* red, float* green, float* blue,
			int i, int sx1, int width, int skip,float headroom);
	int defTransform(int tran);
	void
			rotateLine(float* line, float** channel, int tran, int i, int w,
					int h);
	void transformRect(PreviewProps pp, int tran, int &sx1, int &sy1,
			int &width, int &height, int &fw);
	void transformPosition(int x, int y, int tran, int& tx, int& ty);

	void updateHLRecoveryMap(std::string method, float rm, float gm,
			float bm);
	void updateHLRecoveryMap_ColorPropagation();
	void HLRecovery_ColorPropagation(float* red, float* green, float* blue,
			int i, int sx1, int width, int skip);
			*/
	//unsigned FC(int row, int col) {
	//	return ri->FC(row, col);
	//}
	/* Color space is universal so not here!!
	void colorSpaceConversion(Imagefloat* im, ColorManagementParams cmp,
			cmsHPROFILE embedded, cmsHPROFILE camprofile, float cam[3][3],
			float colorscale);*/
public:
	RawImageSource(char * new_filename);
	~RawImageSource();

	int load(string fname, bool batch = false);

	/*
	void preprocess(const RAWParams &raw);
	void demosaic(const RAWParams &raw);
	void copyOriginalPixels(const RAWParams &raw, RawImage *ri,
			RawImage *riDark, RawImage *riFlatFile);
	void cfaboxblur(RawImage *riFlatFile, float* cfablur, int boxH, int boxW);
	*/
	void scaleColors(int winx, int winy, int winw, int winh);
	/*
	void getImage(ColorTemp ctemp, int tran, Imagefloat* image,
			PreviewProps pp, HRecParams hrp, ColorManagementParams cmp,
			RAWParams raw);


	ColorTemp getWB() {
		return wb;
	}
	ColorTemp getAutoWB();
	ColorTemp getSpotWB(std::vector<Coord2D> red, std::vector<Coord2D> green,
			std::vector<Coord2D>& blue, int tran);
	*/
	float getDefGain() {
		return defGain;
	}
	float getGamma() {
		return 2.2;
	};
/*
	void getFullSize(int& w, int& h, int tr = TR_NONE);
	void getSize(int tran, PreviewProps pp, int& w, int& h);

	ImageData* getImageData() {
		return idata;
	}*/
	/*
	void setProgressListener(ProgressListener* pl) {
		plistener = pl;
	}
	int getAEHistogram(LUTu & histogram, int& histcompr);

	static void colorSpaceConversion16(Image16* im, ColorManagementParams cmp,
			cmsHPROFILE embedded, cmsHPROFILE camprofile, float cam[3][3],
			float& defgain);
			*/
	static void inverse33(float(*coeff)[3], float(*icoeff)[3]);
/*
	static void HLRecovery_Luminance(float* rin, float* gin, float* bin,
			float* rout, float* gout, float* bout, int width, float maxval);
	static void HLRecovery_CIELab(float* rin, float* gin, float* bin,
			float* rout, float* gout, float* bout, int width, float maxval,
			float cam[3][3], float icam[3][3]);
	*/
protected:
	typedef unsigned short ushort;
	/*
	void correction_YIQ_LQ(Imagefloat* i, int times);
	inline void convert_row_to_YIQ(float* r, float* g, float* b, float* Y,
			float* I, float* Q, int W);
	inline void convert_row_to_RGB(float* r, float* g, float* b, float* Y,
			float* I, float* Q, int W);

	inline void convert_to_cielab_row(float* ar, float* ag, float* ab,
			float* oL, float* oa, float* ob);
	inline void interpolate_row_g(float* agh, float* agv, int i);
	inline void interpolate_row_rb(float* ar, float* ab, float* pg, float* cg,
			float* ng, int i);
	inline void interpolate_row_rb_mul_pp(float* ar, float* ab, float* pg,
			float* cg, float* ng, int i, float r_mul, float g_mul,
			float b_mul, int x1, int width, int skip);

	int LinEqSolve(int nDim, float* pfMatr, float* pfVect, float* pfSolution);//Emil's CA auto correction
	void CA_correct_RT(float cared, float cablue);
	void ddct8x8s(int isgn, float (&a)[8][8]);

//	int cfaCleanFromMap(PixelsMap &bitmapBads);
//	int findHotDeadPixel(PixelsMap &bpMap, float thresh);

	void cfa_linedn(float linenoiselevel);//Emil's line denoise

	void green_equilibrate(float greenthresh);//Emil's green equilibration

	void nodemosaic();
	void eahd_demosaic();
	void hphd_demosaic();
	void vng4_demosaic();
	void ppg_demosaic();
	void amaze_demosaic_RT(int winx, int winy, int winw, int winh);//Emil's code for AMaZE
	void fast_demo(int winx, int winy, int winw, int winh);//Emil's code for fast demosaicing
	void dcb_demosaic(int iterations, int dcb_enhance);
	void ahd_demosaic(int winx, int winy, int winw, int winh);
	void border_interpolate(int border, float(*image)[4], int start = 0,
			int end = 0);
	void dcb_initTileLimits(int &colMin, int &rowMin, int &colMax, int &rowMax,
			int x0, int y0, int border);
	void fill_raw(ushort(*cache)[4], int x0, int y0, float** rawData);
	void fill_border(ushort(*cache)[4], int border, int x0, int y0);
	void copy_to_buffer(ushort(*image2)[3], ushort(*image)[4]);
	void dcb_hid(ushort(*image)[4], ushort(*bufferH)[3], ushort(*bufferV)[3],
			int x0, int y0);
	void dcb_color(ushort(*image)[4], int x0, int y0);
	void dcb_hid2(ushort(*image)[4], int x0, int y0);
	void dcb_map(ushort(*image)[4], int x0, int y0);
	void dcb_correction(ushort(*image)[4], int x0, int y0);
	void dcb_pp(ushort(*image)[4], int x0, int y0);
	void dcb_correction2(ushort(*image)[4], int x0, int y0);
	void restore_from_buffer(ushort(*image)[4], ushort(*image2)[3]);
	void dcb_refinement(ushort(*image)[4], int x0, int y0);
	void dcb_color_full(ushort(*image)[4], int x0, int y0, float(*chroma)[2]);

	void transLine(float* red, float* green, float* blue, int i,
			Imagefloat* image, int tran, int imw, int imh, int fw);
	void hflip(Imagefloat* im);
	void vflip(Imagefloat* im);
	*/
	void preprocess(void);
	friend class fast_demosaic;
};

#endif
