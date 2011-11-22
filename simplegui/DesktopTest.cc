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
#include <iostream>
#include <cstdlib>
#include "../rtengine/rtengine.h"
#include "viewport.h"

//using namespace std;
#define X_SIZE 1280
#define Y_SIZE 720
#define START_MOVE 50
void Lab_Denoise(LabImage &a,improps & b);
class desktop: public viewport {

	LabImage baseim;

	Image<rgbHDR> tempim;
	volatile int moved, scale, do_filter;
	volatile int dx, dy, px, py;
	char * raw_to_load;
	Image_Raw * MyRAW;
	int pp3_found;
public:
	//improps props;

	int setup() {
		dx = 0;
		dy = 0;
		px = 0;
		py = 0;
		HDRImage RawTile(width,height);
		scale = 1;
		MyRAW = new Image_Raw(raw_to_load);
		if (MyRAW->pp3_found==0) return 0;
		// this ensure same size as preview.

		// set offset to 0,0
		RawTile.moveto(0, 0);
		// demosaic raw photo
		MyRAW->demosaic(RawTile);
		// output to window
		*this <<= RawTile;

		moved = START_MOVE + 1;
		do_filter = 0;

		// window refreshrate is 50Hz
		usec_delay = 20000;
		return 1;
	}
	~desktop(void)
	{
		if (MyRAW) delete MyRAW;
	}

	// deal with mouse interaction
	void mouse(int x, int y, int butt) {
		static int pressed = 0, oldx, oldy;
		if (butt) {
			if (pressed) {
				dx -= x - oldx;
				dy -= y - oldy;
				moved = START_MOVE;
				py = y - dy;
				px = x - dx;
			}
			oldx = x;
			oldy = y;
			pressed = 1;
			if (butt & 8) {
				moved = START_MOVE;
				do_filter = 0;
			}
			if (butt & 16) {
				moved = START_MOVE;
				do_filter = 1;
			}

		} else
			pressed = 0;
	}

	int render(int input) {
		if (refresh)
		{
			moved=START_MOVE;refresh=0;
		}
		if ((moved == 0)&&(refresh==0))
		{
			usec_delay = 30000;
			return 0;
		}

		usec_delay = 10000;
		if (moved == 1) {
			// update offset
			HDRImage RawTile(width,height);
			RawTile.moveto(dx, dy);

			// convert from raw
			MyRAW->demosaic(RawTile);

			// get current offset
			RawTile.pos(dx, dy); // demosaic has clipped the offsets so we grab them again

			// set offset to 0,0
			RawTile.moveto(0, 0);

			apply_filters(RawTile,  MyRAW->props);

			// output to window
			// does conversion Lab to argb8
			*this <<= RawTile;
			moved=0;
			return 1;
		} else {
			if (moved == START_MOVE)
			{
				HDRImage RawTile(width,height);
				// set offset
				RawTile.moveto(dx, dy);
				// convert raw
				MyRAW->demosaic(RawTile);
				// grab current position
				RawTile.pos(dx, dy); // demosaic has clipped the offsets so we grab them again
				apply_filters(RawTile,  MyRAW->props, 10);
				// set offset to 0,0
				RawTile.moveto(0, 0);

				// show converted image in window.
				*this <<= RawTile;
				moved --;
				return 1;
			}
		}
		if (moved>0) moved--;
		usec_delay = 30000;
		return 0;
	}

	desktop(int argc, char**argv, int nwidth, int nheight) :
			viewport(argv[1], nwidth, nheight) {

		// read pp3 file

		// set name of raw file
		raw_to_load = argv[1];
	}
};
void list_filters(void);
int main(int argc, char ** argv) {
	//list_filters();
	//return 0;
	if (argc < 2) {
		cout << "usage " << argv[0] << " <raw file>\n";
		return 0;
	}

	desktop mydesk(argc, argv, X_SIZE, Y_SIZE);

	if (mydesk.setup())
		mydesk.run();

	return 0;
}

