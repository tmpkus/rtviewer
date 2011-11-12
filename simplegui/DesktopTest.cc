//============================================================================
// Name        : DesktopTest.cpp
// Author      : 
// Version     :
// Copyright   : Jan Rinze Peterzon
// Description : demo for using DWM++ library.
//============================================================================

#include <iostream>
#include <cstdlib>
#include "viewport.h"

//using namespace std;
#define X_SIZE 1280
#define Y_SIZE 720
#define START_MOVE 50

class desktop: public viewport {
	HDRImage RawTile;
	LabImage baseim;
	LabImage workim;
	Image<rgbHDR> tempim;
	volatile int moved, scale, do_filter;
	volatile int dx, dy, px, py;
	char * raw_to_load;
	Image_Raw * MyRAW;
	int pp3_found;
public:
	improps props;

	int setup() {
		dx = 0;
		dy = 0;
		px = 0;
		py = 0;
		scale = 1;
		if (pp3_found==0) return 0;
		MyRAW = new Image_Raw(raw_to_load, props);
		// this ensure same size as preview.
		RawTile <<= *this;
		// set offset to 0,0
		RawTile.moveto(0, 0);
		// demosaic raw photo
		MyRAW->demosaic(RawTile);
		// output to window
		*this <<= RawTile;

		moved = START_MOVE + 1;
		do_filter = 0;
		std::cout << "size x=" << workim.xsize() << " y=" << workim.ysize()
				<< "\n";
		// window refreshrate is 50Hz
		usec_delay = 20000;
		return 1;
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
				moved = 2;
				do_filter = 1;
			}

		} else
			pressed = 0;
	}

	int render(int input) {
		if (moved == 0)
		{
			usec_delay = 30000;
			return 0;
		}
		usec_delay = 10000;

		//
		if (do_filter && moved == 1) {
			// update offset
			RawTile.moveto(dx, dy);

			// convert from raw
			MyRAW->demosaic(RawTile);

			// get current offset
			RawTile.pos(dx, dy); // demosaic has clipped the offsets so we grab them again

			// set offset to 0,0
			RawTile.moveto(0, 0);

			// convert to Lab
			workim <<= RawTile;
			// set offset to 0,0
			workim.moveto(0, 0);

			// apply sharpening if necessary
			if ((bool) props.pp3["[Sharpening]"]["Enabled"] == true) {
				cout << "doing sharpening\n";
				if (props.sh_amount > 0.0f)
					sharpen(workim, props.sh_radius, props.sh_amount * 0.01f,
							0.0f);
			}

			// apply denoise if necessary
			if ((bool) props.pp3["[Directional Pyramid Denoising]"]["Enabled"]
					== true) {
				cout << "doing noise reduction\n";
				if ((props.noise_lamount > 0.0f)
						|| (props.noise_camount > 0.0f))
					workim.Lab_denoise(props.noise_lamount * 0.01f,
							props.noise_camount * 0.01f, props.noise_gamma);
			}

			// output to window
			// does conversion Lab to argb8
			*this <<= workim;
		} else {
			if (moved == START_MOVE)
			{
				// set offset
				RawTile.moveto(dx, dy);
				// convert raw
				MyRAW->demosaic(RawTile);
				// grab current position
				RawTile.pos(dx, dy); // demosaic has clipped the offsets so we grab them again

				// set offset to 0,0
				RawTile.moveto(0, 0);

				// show converted image in window.
				*this <<= RawTile;
			}
		}
		moved--;
		return 1;
	}

	desktop(int argc, char**argv, int width, int height) :
			viewport(argv[1], width, height) {

		// read pp3 file
		pp3_found = props.read(argc, argv);
		// set name of raw file
		raw_to_load = argv[1];
	}
};

int main(int argc, char ** argv) {
	if (argc < 2) {
		cout << "usage " << argv[0] << " <raw file>\n";
		return 0;
	}

	desktop mydesk(argc, argv, X_SIZE, Y_SIZE);

	if (mydesk.setup())
		mydesk.run();

	return 0;
}

