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
#include "../utils/LUT.h"
#include "fast_demo.h"

static LUTf dirwt;
#define SQR(a) ((a)*(a))
#define abs(a) ((a)<0?-(a):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define DDIFF(a , b) (0.01f/0.01f+(a-b)*(a-b))
#define LDIFF(a , b) (ladjust/(ladjust+(a-b)*(a-b)))
#define SDIFF(a , b) (0.0001f/(0.0001f+(a-b)*(a-b)))
#define CDIFF(a , b) (adjust/(adjust+(a-b)*(a-b)))
#define CLIP( x ) (x)

void fast_demosaic::improve_correlation(HDRImage & pass1, HDRImage& pass2, float gadjust, float ladjust, int green)
{
	// improve green intensity correlations
#pragma omp parallel for
	for ( unsigned int y = 4 ; y < (H - 4) ; y++ )
		for ( unsigned int x = 4 ; x < (W - 4) ; x++ ) {
			float r = pass1[y][x].r;
			float g = pass1[y][x].g;
			float b = pass1[y][x].b;
			float adjust = gadjust;
			if (g < 1.0f)
				adjust = adjust * (1 - g); //
			int col = FC(y, x);
			if ((col != 1) && green) // to be changed green locations
					{

				float ravg = 0.0f, rsum = 0.0f;
				float bavg = 0.0f, bsum = 0.0f;

				// improve correlation for green with red and blue
				for ( int j = y - 4 ; j < y + 5 ; j++ )
					for ( int i = x - 4 ; i < x + 5 ; i++ ) {
						if ((FC(j, i)) == 1)

						{
							float adjust = gadjust * (1 - r), w;
							//if ((FC(j, i)) == 1) //red locations
							//		{
							w = LDIFF(pass1[j][i].r,r);
							ravg += w * pass1[j][i].g; /// (pass1[j][i].r + adjust);
							rsum += w;
							//}
							adjust = gadjust * (1 - b);
							//if ((FC(j, i)) == 1) //blue locations
							//{
							w = LDIFF(pass1[j][i].b,b);
							bavg += w * pass1[j][i].g; /// (pass1[j][i].b + adjust);
							bsum += w;
							//}
						}
					}
				if (FC(y, x) == 0) // red location
					g = (ravg / rsum); //(r + adjust) *
				else g = (bavg / bsum); //(b + adjust) *
			}
			// improve red and blue
			if (green == 0) {
				float ravg = 0.0f, bavg = 0.0f, rsum = 0.0f, bsum = 0.0f;
				// improve correlation for red and blue with green

				for ( int j = y - 2 ; j < y + 3 ; j++ )
					for ( int i = x - 2 ; i < x + 3 ; i++ ) {
						float wg = LDIFF(pass1[j][i].g,g); // / (1.0f + i * i + j * j);
						float wr = wg, wb = wg;
						//if ((FC(j, i)) == 0) wr=wr*8.0f;
						ravg += wr * pass1[j][i].r; /// (pass1[j][i].g + adjust);
						rsum += wr;
						//if ((FC(j, i)) == 2) wb=wb*8.0f; // blue site
						bavg += wb * pass1[j][i].b; // / (pass1[j][i].g + adjust);
						bsum += wb;

					}
				if (FC(y, x) != 0) // changeable red
					r = ravg / rsum; //(g + adjust) * ravg / rsum; // 0.5f * (r + (g + 0.005f) * ravg / rsum);
				if (FC(y, x) != 2) // changeable blue
					b = bavg / bsum; // (g + adjust) * bavg / bsum; //0.5f * (b + (g + 0.005f) * bavg / bsum);

			}
			pass2[y][x].r = r;
			pass2[y][x].g = g;
			pass2[y][x].b = b;

		}
}
void fast_demosaic::color_correct(HDRImage & pass1, HDRImage& pass2,improps &props)
{
	const float expcomp = props.pp3["[Exposure]"]["Compensation"];
	const float post_scale = pow(2.0, expcomp);
	int Hgt=pass2.ysize(),Wdt=pass2.xsize();
	cout << "Doing color correction\n";
#pragma omp for
	for (  int y = 2 ; y < (Hgt - 2) ; y++ )
		for (  int x = 2 ; x < (Wdt - 2) ; x++ ) {
			float r = pass1[y][x].r*post_scale;
			float g = pass1[y][x].g*post_scale;
			float b = pass1[y][x].b*post_scale;

			//float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
			//float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
			//float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
#if 1			// adjust levels
			r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f:r);
			g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f:g);
			b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f:b);
#endif
			float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
			float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
			float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
#if 0 // BW processing for sharpness check
			float c=r;
			pass2[y][x].r = c;
			pass2[y][x].g = c;
			pass2[y][x].b = c;
#else
			pass2[y][x].r = nr;
			pass2[y][x].g = ng;
			pass2[y][x].b = nb;
#endif
		}
}

#define HAVG(y,x) (0.25*(hint[y-1][x-1]+hint[y-1][x]+hint[y][x-1]+hint[y][x]))
#define HINT(y,x) (hint[y-1][x-1]-HAVG(y-1,x-1)+hint[y][x]-HAVG(y+1,x+1) + )

void fast_demosaic::naive(HDRImage & dst, array2D<float> &hint, float adjust, float ladjust)
{
	array2D<float> raw(W, H);
	int rot = (get_rotateDegree() / 90) & 3;
	HDRImage phase2(W, H);

// convert raw to proper colors

	int max[3] = { 0, 0, 0 };
	float maxf[3] = { 0, 0, 0 };
#pragma omp for
	for (  int y = 0 ; y < H ; y++ ) {
		for (  int x = 0 ; x < W ; x++ ) {
			// convert to float, scale and clip
			int c = FC(y, x);
			int v = data[y][x];
			if (v > max[c])
				max[c] = v;
			float r = channel_mul[c] * ((float) v - (float) cblack[c]);
			if (r > maxf[c])
				maxf[c] = r;
			raw[y][x] = (r > 2.0f) ? 2.0f : ((r < 0.0f) ? 0.0f:r);
		}
	}
	for ( int c = 0 ; c < 3 ; c++ )
		cout << "max:" << maxf[c] << endl;
#pragma omp parallel
	{ // naive demosaic
#define stripesize 60
#pragma omp for
		for ( int X = 2 ; X < W - 2 ; X += stripesize )
			for (  int y = 2 ; y < (H - 2) ; y++ ) {
				int end = (X + stripesize) > (W - 2) ? (W - 2) : X + stripesize;
				int mx=W-1;int my=H-1;
				for (  int x = X ; x < end ; x++ ) {
					float r, g, b;

					if (FC(y, x) == 1) // green location
							{
						g = raw[y][x];
						float ul = raw[y - 1][x - 1];
						float ur = raw[y - 1][x + 1];
						float bl = raw[y + 1][x - 1];
						float br = raw[y + 1][x + 1];

						float u1=raw[y - 1][x];
						float u2=raw[y + 1][x];
						float v1=raw[y][x - 1];
						float v2=raw[y][x + 1];

						float up = 0.5 * (ul + ur);
						float dw = 0.5 * (bl + br);
						float le = 0.5 * (ul + bl);
						float ri = 0.5 * (ur + br);

						float u = 0.5f*(u1+u2) + 0.25f*(g-0.5f*(up+dw))*((u1+u2+adjust)/(up+dw+adjust));
						float v = 0.5f*(v1+v2) + 0.25f*(g-0.5f*(le+ri))*((v1+v2+adjust)/(le+ri+adjust));//(g + adjust) * 0.5f * (raw[y][x - 1] / (le + adjust) + raw[y][x + 1] / (ri + adjust));
						//float u = 0.5f * (raw[y - 1][x] + raw[y + 1][x]);
						//float v = 0.5f * (raw[y][x - 1] + raw[y][x + 1]);
						if ((FC(y - 1, x)) == 0) // above = red
								{
							r = u;
							b = v;
						}
						else {
							r = v;
							b = u;
						}

					}
					else {
						float col = raw[y][x];
						//float base = col * 2.0f;
						//float off = col + 2.0f * adjust;
						float n = raw[y - 1][x];
						float w = raw[y][x - 1];
						float e = raw[y][x + 1];
						float s = raw[y + 1][x];

						float cn = (raw[y - 2][x]);// + adjust;
						float cw = (raw[y][x - 2]);// + adjust;
						float ce = (raw[y][x + 2]);// + adjust;
						float cs = (raw[y + 2][x]);// + adjust;

						float nw = raw[y - 1][x - 1];
						float ne = raw[y - 1][x + 1];
						float sw = raw[y + 1][x - 1];
						float se = raw[y + 1][x + 1];

						float cnw = 0.5f * (col + raw[y - 2][x - 2]) + adjust;
						float cne = 0.5f * (col + raw[y - 2][x + 2]) + adjust;
						float csw = 0.5f * (col + raw[y + 2][x - 2]) + adjust;
						float cse = 0.5f * (col + raw[y + 2][x + 2]) + adjust;

						float f;
						double sum=0,av=0;
						// default method.
						/*
						f= CDIFF(cn,cs)*CDIFF(col,(0.5f*(cn+cs)));
						av= f*(n+s)*0.5f;
						sum=f;
						f= CDIFF(cw,ce)*CDIFF(col,(0.5f*(cw+ce)));
						av+= f*(w+e)*0.5f;
						sum+=f;*/
						// refine
						f= LDIFF(col,cn);
						av+= f*n;
						sum+=f;
						f= LDIFF(col,cw);
						av+= f*w;
						sum+=f;
						f= LDIFF(col,ce);
						av+= f*e;
						sum+=f;
						f= LDIFF(col,cs);
						av+= f*s;
						sum+=f;
						av+=5.0f*(0.25f*(n+w+e+s)  + 0.25f*(col-0.25f*(cn+cw+ce+cs)));
						sum+=5.0f;

						g = av/sum;

						float v = 0.25f*(nw + ne + sw + se ) + 0.25f*(g-0.25f*(n+s+e+w));

						if (FC(y, x) == 0) {
							r = col;
							b = v;
						}
						else {
							r = v;
							b = col;
						}
					}




					switch (rot)
					{
						case 0:
								RGB_converted[y][x].r = r > 0.0f ? r : 0.0f;
								RGB_converted[y][x].g = g > 0.0f ? g : 0.0f;
								RGB_converted[y][x].b = b > 0.0f ? b : 0.0f;
								break;
						case 1:
								RGB_converted[x][my - y].r = r > 0.0f ? r : 0.0f;
								RGB_converted[x][my - y].g = g > 0.0f ? g : 0.0f;
								RGB_converted[x][my - y].b = b > 0.0f ? b : 0.0f;
								break;
						case 2:
								RGB_converted[my - y][mx - x].r = r > 0.0f ? r : 0.0f;
								RGB_converted[my - y][mx - x].g = g > 0.0f ? g : 0.0f;
								RGB_converted[my - y][mx - x].b = b > 0.0f ? b : 0.0f;
								break;
						case 3:
								RGB_converted[mx - x][y].r = r > 0.0f ? r : 0.0f;
								RGB_converted[mx - x][y].g = g > 0.0f ? g : 0.0f;
								RGB_converted[mx - x][y].b = b > 0.0f ? b : 0.0f;
								break;
					}
				}
			}
	}
}
/*
void fast_demosaic::corner(HDRImage & dst, array2D<float> &I,improps &props)
{
	array2D<float> raw(W, H);

	float blacklevel = 0.0f; //black;
	float adj_black = black;
	float r_black = cblack[0]; //+black;
	float g_black = cblack[1]; //+black;
	float b_black = cblack[2]; //+black;
	const float expcomp = props.pp3["[Exposure]"]["Compensation"];
	const float post_scale = pow(2.0, expcomp);
	float adj_range = range / post_scale;
	float mr = channel_mul[0] / adj_range; ///range;
	float mg = channel_mul[1] / adj_range; ///range;
	float mb = channel_mul[2] / adj_range; ///range;
// corner demosaic
#pragma omp for
	for ( unsigned int y = 0 ; y < (H - 1) ; y++ )
		for ( unsigned int x = 0 ; x < (W - 1) ; x++ ) {
			float r, g, b;
			int i = x, j = y;
			float c[4];
			switch (FC(y, x))
			{
			case 0: // red
			{
				r = ((float) data[y][x] - r_black);
				g = (0.5f * ((float) data[y][x + 1] + (float) data[y + 1][x]) - g_black);
				b = ((float) data[y + 1][x + 1] - b_black);
			}
				break;
			case 1: // green
			{
				g = (0.5f * ((float) data[y][x] + (float) data[y + 1][x + 1]) - g_black);
				float u = ((float) data[y][x + 1]);
				float v = ((float) data[y + 1][x]);
				if ((FC(y, x + 1)) == 0) // right = red
						{
					r = (u - r_black);
					b = (v - b_black);
				}
				else {
					r = (v - r_black);
					b = (u - b_black);
				}

			}
				break;
			case 2: // blue
			{
				b = ((float) data[y][x] - b_black) * mb;
				g = ((0.5f * ((float) data[y][x + 1] + (float) data[y + 1][x]) - g_black) * mg);
				r = ((float) data[y + 1][x + 1] - r_black) * mr;
			}
				break;
			}
			// clip negative

			r = (r > 0.0f) ? r : 0.0f;
			g = (g > 0.0f) ? g : 0.0f;
			b = (b > 0.0f) ? b : 0.0f;
			I[y][x] = (0.5 * g + 0.2 * r + 0.3 * b);
			float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
			float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
			float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
#if 0
			float col = b;
			dst[y][x].r = col;
			dst[y][x].g = col;
			dst[y][x].b = col;
#else
			dst[y][x].r = nr * mr;
			dst[y][x].g = ng * mg;
			dst[y][x].b = nb * mb;
#endif
		}
}
static void corner_to_I(HDRImage &src, array2D<float> &dst)
{
	int W = src.xsize(), H = src.ysize();
	for ( int y = 0 ; y < H - 1 ; y++ )
		for ( int x = 0 ; x < W - 1 ; x++ )
			dst[y][x] = 0.3f * src[y][x].r + 0.59f * src[y][x].g + 0.11f * src[y][x].b;
}*/
void fast_demosaic::jrp_demo(HDRImage & dst,improps & props)
{
	array2D<float> I(W, H);

	cout << W << "x" << H << "starting conversion\n";
	cout << "jrp demosaic step 1\n";
	// dcraw conversion method
	int sum[8];
	int c;
	memset(sum, 0, sizeof sum);
	for ( int row = 0 ; row < 8 ; row++ )
		for ( int col = 0 ; col < 8 ; col++ ) {
			int val;
			c = FC(row, col);
			if ((val = white[row][col] - cblack[c]) > 0) {
				sum[c] += val;
				sum[c + 4] += 1;
			}
		}
	if (sum[0] && sum[1] && sum[2] && sum[3])
		for ( c = 0; c < 4 ; c++ )
			cam_mul[c] = (float) sum[c + 4] / (float) sum[c];
	float blmax = cblack[0];
	if (cblack[1] > blmax)
		blmax = cblack[1];
	if (cblack[2] > blmax)
		blmax = cblack[2];

	range = ((float) get_white() - blmax - black);
	{
		float min_channel;
		// normalize color channels
		min_channel = cam_mul[0];
		if (min_channel > cam_mul[1])
			min_channel = cam_mul[1];
		if (min_channel > cam_mul[2])
			min_channel = cam_mul[2];
		min_channel *= ((float) get_white() - blmax - black); // full
		for ( int i = 0 ; i < 3 ; i++ ) {

			for ( int j = 0 ; j < 3 ; j++ ) {
				mat[i][j] = rgb_cam[i][j];
				cout << " " << rgb_cam[i][j];
			}
			cout << endl;
		}

		float r = 1;
		float g = 1;
		float b = 1;

		float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
		float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
		float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
		if (ng < nr)
			nr = ng;
		if (nb < nr)
			nr = nb;
		min_channel = min_channel * nr;
		channel_mul[0] = cam_mul[0] / min_channel;
		channel_mul[1] = cam_mul[1] / min_channel;
		channel_mul[2] = cam_mul[2] / min_channel;
	}
	volatile int pct = 0;
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

	if (Tile_flags[0][0]<2)
	{
		//corner(pass1,I);
		Tile_flags[0][0]=2;
		float accur = 0.0004;
		naive(RGB_converted, I, accur, accur*0.25f);
		color_correct(RGB_converted,RGB_converted,props);

	}
	RGB_converted.moveto(-dst.xoff(),-dst.yoff());
	dst <<= RGB_converted;
}
void fast_demosaic::cook_data(improps & props)
{
	cout << W << "x" << H << "starting conversion\n";
	cout << "jrp demosaic step 1\n";
	int sum[8];
	int c;
	memset(sum, 0, sizeof sum);
	for ( int row = 0 ; row < 8 ; row++ )
		for ( int col = 0 ; col < 8 ; col++ ) {
			int val;
			c = FC(row, col);
			if ((val = white[row][col] - cblack[c]) > 0) {
				sum[c] += val;
				sum[c + 4] += 1;
			}
		}
	if (sum[0] && sum[1] && sum[2] && sum[3])
		for ( c = 0; c < 4 ; c++ )
			cam_mul[c] = (float) sum[c + 4] / (float) sum[c];
	float blmax = cblack[0];
	if (cblack[1] > blmax)
		blmax = cblack[1];
	if (cblack[2] > blmax)
		blmax = cblack[2];

	range = ((float) get_white() - blmax - black);
	{
		float min_channel;
		// normalize color channels
		min_channel = cam_mul[0];
		if (min_channel > cam_mul[1])
			min_channel = cam_mul[1];
		if (min_channel > cam_mul[2])
			min_channel = cam_mul[2];
		min_channel *= ((float) get_white() - blmax - black); // full
		cout << " maximum is:" << get_white() << endl;
		for ( int i = 0 ; i < 3 ; i++ ) {
			for ( int j = 0 ; j < 3 ; j++ ) {
				mat[i][j] = rgb_cam[i][j];
				cout << " " << rgb_cam[i][j];
			}
			cout << endl;
		}

		float r = 1;
		float g = 1;
		float b = 1;

		float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
		float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
		float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
		if (ng < nr)
			nr = ng;
		if (nb < nr)
			nr = nb;
		min_channel = min_channel * nr;
		channel_mul[0] = cam_mul[0] / min_channel;
		channel_mul[1] = cam_mul[1] / min_channel;
		channel_mul[2] = cam_mul[2] / min_channel;
	}

	for (int i=0;i<3;i++)
			for (int j=0;j<3;j++)
				props.mat[i][j]=mat[i][j];

	cout << "setup tile management\n";
	int rot = get_rotateDegree() / 90;
	//if (rot&1)
	//	Tile_flags((H + TILE_SIZE - 1) / TILE_SIZE,(W + TILE_SIZE - 1) / TILE_SIZE); // setup correct size
	//else
	Tile_flags((W + TILE_SIZE - 1) / TILE_SIZE,(H + TILE_SIZE - 1) / TILE_SIZE); // setup correct size

	for ( unsigned int ty = 0 ; ty < Tile_flags.height() ; ty++ )
		for ( unsigned int tx = 0 ; tx < Tile_flags.width() ; tx++ )
			Tile_flags[ty][tx] = 0; // set cleared;
	cout << "setup RGB data image\n";
	// check orientation

	//method = VARSIZE_DEMOSAIC;
	method = AMAZE_DEMOSAIC;
	//method = HALFSIZE_DEMOSAIC;
	//method = JRP_DEMOSAIC;

	if ((method != HALFSIZE_DEMOSAIC)&&(method != VARSIZE_DEMOSAIC)){
		if (rot & 1)
			RGB_converted(H, W);
		else RGB_converted(W, H);
	}
	else
	if(method == HALFSIZE_DEMOSAIC) {
		if (rot & 1)
			RGB_converted((H + 1) / 2, (W + 1) / 2);
		else RGB_converted((W + 1) / 2, (H + 1) / 2);
	} else {
		if (rot & 1)
					RGB_converted((H + 1) / 3, (W + 1) / 3);
				else RGB_converted((W + 1) / 3, (H + 1) / 3);
	}
	RGB_converted.moveto(0,0);
	rawData(W, H);
	float max_f=4.0f;
#pragma omp for
	for ( unsigned int y = 0 ; y < H ; y++ ) {
		for ( unsigned int x = 0 ; x < W ; x++ ) {
			// convert to float, scale and clip
			int c = FC(y, x);
			int v = data[y][x];
			float r = channel_mul[c] * ((float) v - (float) cblack[c]);
			rawData[y][x] = (r > max_f) ? max_f : ((r < 0.0f) ? 0.0f:r);
		}
	}
	if ((method==JRP_DEMOSAIC)&&0)
	{
		linear_interpolate();
		color_correct(RGB_converted,RGB_converted,props);
		Tile_flags[0][0]=2;
	}
}

int fast_demosaic::touch_tiles(HDRImage &dest, int &tile_xs, int &tile_xe, int &tile_ys, int &tile_ye)
{
	int rot = (get_rotateDegree() / 90) & 3;

	cout << " check tiles with rotation " << rot << endl;

// clip properly the translation.

	{
		int px = dest.xoff();
		int py = dest.yoff();
		if (px < 0)
			px = 0;
		if (py < 0)
			py = 0;
		cout << "before clip " << px << " " << py << endl;
		cout << " dest X Y: " << RGB_converted.xsize() << " " << RGB_converted.ysize() << endl;
		if ((px + dest.xsize()) > RGB_converted.xsize())
			px = RGB_converted.xsize() - dest.xsize();
		if ((py + dest.ysize()) > RGB_converted.ysize())
			py = RGB_converted.ysize() - dest.ysize();

		cout << "after clip " << px << " " << py << endl;
		dest.moveto(px, py);
	}

	int tsz=TILE_SIZE;
	if (method==HALFSIZE_DEMOSAIC)
		tsz=tsz/2;
	if (method==VARSIZE_DEMOSAIC)
			tsz=tsz/3;
	// something is still not right here.

	int d_x = dest.xoff();
	int d_y = dest.yoff();
	int d_ex = dest.xoff() + dest.xsize() - 1;
	int d_ey = dest.yoff() + dest.ysize() - 1;

	switch (rot)
	{
	case 0:
		tile_xs = d_x / tsz;
		tile_xe = d_ex / tsz;
		tile_ys = d_y / tsz;
		tile_ye = d_ey / tsz;
		break;
	case 1:
		tile_xs= d_y /tsz;
		tile_xe= d_ey/tsz;
		tile_ys= (H-d_ex-1)/tsz;
		tile_ye= (H-d_x-1)/tsz;
		break;
	case 2:
		tile_xs= (W-d_ex-1) /tsz;
		tile_xe= (W-d_x-1)/tsz;
		tile_ys= (H-d_ey-1)/tsz;
		tile_ye= (H-d_y-1)/tsz;
		break;
	case 3:
		tile_xs= (W-d_ey-1) /tsz;
		tile_xe= (W-d_y-1)/tsz;
		tile_ys= (d_x)/tsz;
		tile_ye= (d_ex)/tsz;
		break;
	}

	/*
	tile_xs = dest.xoff() / tsz;
	tile_ys = dest.yoff() / tsz;
	tile_xe = (dest.xoff() + dest.xsize() - 1) / tsz;
	tile_ye = (dest.yoff() + dest.ysize() - 1) / tsz;
	{
		int txs = tile_xs;
		int txe = tile_xe;
		int tys = tile_ys;
		int tye = tile_ye;

		switch (rot)
		{
		case 0:
			break;
		case 1: // 90 degrees rotated means X'=y and Y'=width-x-1;

			// now rotate according to rule

			tile_xs = tile_ys;
			tile_xe = tile_ye;
			tile_ye = Tile_flags.height() - txs - 1;
			tile_ys = Tile_flags.height() - txe - 1;
			break;
		case 2: // 180 degrees rotated means X'=width-x-1 and Y'=height-y-1;

			// now rotate according to rule

			tile_xs = Tile_flags.width() - txe - 1;
			tile_xe = Tile_flags.width() - txs - 1;
			tile_ye = Tile_flags.height() - tys - 1;
			tile_ys = Tile_flags.height() - tye - 1;
			break;
		case 3: // 270 degrees rotated means X'=width-x-1 and Y'=height-y-1;

			// now rotate according to rule

			tile_xs = Tile_flags.width() - tye - 1;
			tile_xe = Tile_flags.width() - tys - 1;
			tile_ye = txe;
			tile_ys = txs;
			break;
		}
	}
*/
	int runtiles = 0;
	cout << "rotation is: "<< rot << endl;
	cout << "tiles from " << tile_xs << "x" << tile_ys << " to " << tile_xe << "x" << tile_ye <<endl;
	if (tile_xs<0) { tile_xs=0; cout << " x too low" << endl;}
	if (tile_ys<0) { tile_ys=0; cout << " y too low" << endl;}
	if (tile_ye>=Tile_flags.height()) { tile_ye=Tile_flags.height()-1;cout << " y too high" << endl;}
	if (tile_xe>=Tile_flags.width()) { tile_ye=Tile_flags.width()-1;;cout << " x too high" << endl;}


	for ( ; tile_ys <= tile_ye ; tile_ys++ )
		for ( int tx = tile_xs ; tx <= tile_xe ; tx++ )
			if (Tile_flags[tile_ys][tx] !=2) {
				Tile_flags[tile_ys][tx] = 1;
				runtiles++;
			}
	return runtiles;
}
void fast_demosaic::half_size_demo(HDRImage & dest,improps & props)
{
	int tile_xs, tile_ys, tile_xe, tile_ye;
	int rot = (get_rotateDegree() / 90) & 3;
	if (touch_tiles(dest, tile_xs, tile_xe, tile_ys, tile_ye) > 0) {
		for ( int X = 0 ; X < W ; X += TILE_SIZE ) {
			int xlim = ((X + TILE_SIZE) < W) ? X + TILE_SIZE:W;
			int mrow = (H/2)-1;
			int mcol = (W/2)-1;

			for ( int Y = 0 ; Y < H ; Y += TILE_SIZE ) {
				if (Tile_flags[Y / TILE_SIZE][X / TILE_SIZE] == 1) { // process request
					int ylim = ((Y + TILE_SIZE) < H) ? Y + TILE_SIZE:H;
					for (int y = Y; y < ylim ; y += 2 )
						for (int x = X; x < xlim ; x += 2 ) {
							float r, g, b;

							if (FC(x, y) == 1) // green here
									{
								g = 0.5f * (rawData[y][x] + rawData[y + 1][x + 1]);
								if (FC((x + 1), y) == 0) //red next
										{
									r = rawData[y][x + 1];
									b = rawData[y + 1][x];
								}
								else {
									b = rawData[y][x + 1];
									r = rawData[y + 1][x];
								}
							}
							else {
								g = 0.5f * (rawData[y][x + 1] + rawData[y + 1][x]);
								if (FC(x, y) == 0) //red next
										{
									r = rawData[y][x];
									b = rawData[y + 1][x + 1];
								}
								else {
									b = rawData[y][x];
									r = rawData[y + 1][x + 1];
								}
							}
							int row = y / 2;
							int col = x / 2;
							switch (rot)
							{
							case 0:
								RGB_converted[row][col].r = r > 0.0f ? r : 0.0f;
								RGB_converted[row][col].g = g > 0.0f ? g : 0.0f;
								RGB_converted[row][col].b = b > 0.0f ? b : 0.0f;
								break;
							case 1:
								RGB_converted[col][mrow - row].r = r > 0.0f ? r : 0.0f;
								RGB_converted[col][mrow - row].g = g > 0.0f ? g : 0.0f;
								RGB_converted[col][mrow - row].b = b > 0.0f ? b : 0.0f;
								break;
							case 2:
								RGB_converted[mrow - row][mcol - col].r = r > 0.0f ? r : 0.0f;
								RGB_converted[mrow - row][mcol - col].g = g > 0.0f ? g : 0.0f;
								RGB_converted[mrow - row][mcol - col].b = b > 0.0f ? b : 0.0f;
								break;

							case 3:
								RGB_converted[mcol - col][row].r = r > 0.0f ? r : 0.0f;
								RGB_converted[mcol - col][row].g = g > 0.0f ? g : 0.0f;
								RGB_converted[mcol - col][row].b = b > 0.0f ? b : 0.0f;
								break;
							}

						}
					// tag this processed tile
				Tile_flags[Y / TILE_SIZE][X / TILE_SIZE] = 2;
				}

			}
		}
	}
	RGB_converted.moveto(-dest.xoff(), -dest.yoff());
	dest <<= RGB_converted;
	color_correct(dest,dest,props);

}
void fast_demosaic::nth_size_demo(HDRImage & dest,int num,improps & props)
{
	int tile_xs, tile_ys, tile_xe, tile_ye;
	int rot = (get_rotateDegree() / 90) & 3;
	if (touch_tiles(dest, tile_xs, tile_xe, tile_ys, tile_ye) > 0) {
		for ( int X = 0 ; X < W ; X += TILE_SIZE ) {
			int xlim = ((X + TILE_SIZE) < W) ? X + TILE_SIZE:W;
			int mrow = (H/num)-1;
			int mcol = (W/num)-1;

			for ( int Y = 0 ; Y < H ; Y += TILE_SIZE ) {
				if (Tile_flags[Y / TILE_SIZE][X / TILE_SIZE] == 1) { // process request
					int ylim = ((Y + TILE_SIZE) < H) ? Y + TILE_SIZE:H;
					for (int y = Y; y < ylim ; y += num )
						for (int x = X; x < xlim ; x += num ) {
							float r=0, g=0, b=0;
							float nr=0,ng=0,nb=0;
							for (int ty=y;ty<y+num;ty++)
								for(int tx=x;tx<x+num;tx++)
								{
									if (FC(tx, ty) == 1) // green here
									{
										g += rawData[ty][tx];
										ng+=1.0;
									} else
									if (FC(tx, ty) == 0) //red next
									{
										r += rawData[ty][tx];
										nr+=1.0;
									} else
									{
										b += rawData[ty][tx];
										nb+=1.0;
									}
								}
							r=r/nr;
							g=g/ng;
							b=b/nb;
							int row = y / num;
							int col = x / num;
							switch (rot)
							{
							case 0:
								RGB_converted[row][col].r = r > 0.0f ? r : 0.0f;
								RGB_converted[row][col].g = g > 0.0f ? g : 0.0f;
								RGB_converted[row][col].b = b > 0.0f ? b : 0.0f;
								break;
							case 1:
								RGB_converted[col][mrow - row].r = r > 0.0f ? r : 0.0f;
								RGB_converted[col][mrow - row].g = g > 0.0f ? g : 0.0f;
								RGB_converted[col][mrow - row].b = b > 0.0f ? b : 0.0f;
								break;
							case 2:
								RGB_converted[mrow - row][mcol - col].r = r > 0.0f ? r : 0.0f;
								RGB_converted[mrow - row][mcol - col].g = g > 0.0f ? g : 0.0f;
								RGB_converted[mrow - row][mcol - col].b = b > 0.0f ? b : 0.0f;
								break;

							case 3:
								RGB_converted[mcol - col][row].r = r > 0.0f ? r : 0.0f;
								RGB_converted[mcol - col][row].g = g > 0.0f ? g : 0.0f;
								RGB_converted[mcol - col][row].b = b > 0.0f ? b : 0.0f;
								break;
							}

						}
					// tag this processed tile
				Tile_flags[Y / TILE_SIZE][X / TILE_SIZE] = 2;
				}

			}
		}
	}
	RGB_converted.moveto(-dest.xoff(), -dest.yoff());
	dest <<= RGB_converted;
	color_correct(dest,dest,props);

}
void fast_demosaic::linear_interpolate()
{
	float * bayer=rawData[0];
    const int bayerStep = rawData.width();
    const int rgbStep = 3 * rawData.width();
    int width = rawData.width();
    int height = rawData.height();
    cout<< "interpolation simple linear \n";
    float *rgb = new float [3*width*height];
    float * refrgb= rgb;

    int blue = ((FC(0,0)==2)||(FC(0,1)==2))?-1:1;
    int start_with_green =(FC(0,0)==1);

//    ClearBorders(rgb, sx, sy, 2);
    rgb += 2 * rgbStep + 6 + 1;
    height -= 4;
    width -= 4;

    /* We begin with a (+1 line,+1 column) offset with respect to bilinear decoding, so start_with_green is the same, but blue is opposite */
    blue = -blue;

    for (; height--; bayer += bayerStep, rgb += rgbStep) {
        float t0, t1;
        const float *bayerEnd = bayer + width;
        const int bayerStep2 = bayerStep * 2;
        const int bayerStep3 = bayerStep * 3;
        const int bayerStep4 = bayerStep * 4;

        if (start_with_green) {
            /* at green pixel */
            rgb[0] = bayer[bayerStep2 + 2];
            t0 = rgb[0] * 5.0f
      + ((bayer[bayerStep + 2] + bayer[bayerStep3 + 2]) *4.0f)
      - bayer[2]
      - bayer[bayerStep + 1]
      - bayer[bayerStep + 3]
      - bayer[bayerStep3 + 1]
      - bayer[bayerStep3 + 3]
      - bayer[bayerStep4 + 2]
      + ((bayer[bayerStep2] + bayer[bayerStep2 + 4] ) *0.5f);
            t1 = rgb[0] * 5.0f +
      ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 3])*4.0f)
      - bayer[bayerStep2]
      - bayer[bayerStep + 1]
      - bayer[bayerStep + 3]
      - bayer[bayerStep3 + 1]
      - bayer[bayerStep3 + 3]
      - bayer[bayerStep2 + 4]
      + ((bayer[2] + bayer[bayerStep4 + 2] ) *0.5f);
            rgb[-blue] = CLIP((t0 ) *0.125f);
            //CLIP(t0, );
            rgb[blue] = CLIP((t1 ) *0.125f);
            //CLIP(t1, );
            bayer++;
            rgb += 3;
        }

        if (blue > 0) {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                /* B at B */
                rgb[1] = bayer[bayerStep2 + 2];
                /* R at B */
                t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
                       bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) *2.0f)
        -
        (((bayer[2] + bayer[bayerStep2] +
           bayer[bayerStep2 + 4] + bayer[bayerStep4 +
                         2]) * 3.0f) *0.5f)
        + rgb[1] * 6.0f;
                /* G at B */
                t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
                       bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2]) *2.0f)
        - (bayer[2] + bayer[bayerStep2] +
           bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
        + (rgb[1] *4.0f);
                rgb[-1]= CLIP(t0 *0.125f);
                rgb[0]= CLIP(t1 *0.125f);
                /* at green pixel */
                rgb[3] = bayer[bayerStep2 + 3];
                t0 = rgb[3] * 5.0f
        + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) *4.0f)
        - bayer[3]
        - bayer[bayerStep + 2]
        - bayer[bayerStep + 4]
        - bayer[bayerStep3 + 2]
        - bayer[bayerStep3 + 4]
        - bayer[bayerStep4 + 3]
        +
        ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] ) *0.5f);
                t1 = rgb[3] * 5.0f +
        ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) *4.0f)
        - bayer[bayerStep2 + 1]
        - bayer[bayerStep + 2]
        - bayer[bayerStep + 4]
        - bayer[bayerStep3 + 2]
        - bayer[bayerStep3 + 4]
        - bayer[bayerStep2 + 5]
        + ((bayer[3] + bayer[bayerStep4 + 3] ) *0.5f);
                rgb[2] = CLIP(t0 *0.125f);
                //CLIP(t0, );
                rgb[4] = CLIP(t1*0.125f);
                //CLIP(t1, rgb[4]);
            }
        } else {
            for (; bayer <= bayerEnd - 2; bayer += 2, rgb += 6) {
                /* R at R */
                rgb[-1] = bayer[bayerStep2 + 2];
                /* B at R */
                t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
                       bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) *2.0f)
        -
        (((bayer[2] + bayer[bayerStep2] +
           bayer[bayerStep2 + 4] + bayer[bayerStep4 +
                         2]) * 3.0f ) *0.5f)
        + rgb[-1] * 6.0f;
                /* G at R */
                t1 = ((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
                       bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2]) *2.0f)
        - (bayer[2] + bayer[bayerStep2] +
           bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
        + (rgb[-1] *4.0f);
                rgb[1]= CLIP(t0 *0.125f);

                rgb[0]= CLIP(t1 *0.125f);

                /* at green pixel */
                rgb[3] = bayer[bayerStep2 + 3];
                t0 = rgb[3] * 5.0f
        + ((bayer[bayerStep + 3] + bayer[bayerStep3 + 3]) *4.0f)
        - bayer[3]
        - bayer[bayerStep + 2]
        - bayer[bayerStep + 4]
        - bayer[bayerStep3 + 2]
        - bayer[bayerStep3 + 4]
        - bayer[bayerStep4 + 3]
        +
        ((bayer[bayerStep2 + 1] + bayer[bayerStep2 + 5] ) *0.5f);
                t1 = rgb[3] * 5.0f +
        ((bayer[bayerStep2 + 2] + bayer[bayerStep2 + 4]) *4.0f)
        - bayer[bayerStep2 + 1]
        - bayer[bayerStep + 2]
        - bayer[bayerStep + 4]
        - bayer[bayerStep3 + 2]
        - bayer[bayerStep3 + 4]
        - bayer[bayerStep2 + 5]
        + ((bayer[3] + bayer[bayerStep4 + 3] ) *0.5f);
                rgb[4]= CLIP(t0 *0.125f);
                rgb[2]= CLIP(t1 *0.125f);
            }
        }

        if (bayer < bayerEnd) {
            /* B at B */
            rgb[blue] = bayer[bayerStep2 + 2];
            /* R at B */
            t0 = ((bayer[bayerStep + 1] + bayer[bayerStep + 3] +
                   bayer[bayerStep3 + 1] + bayer[bayerStep3 + 3]) *2.0f)
      -
      (((bayer[2] + bayer[bayerStep2] +
         bayer[bayerStep2 + 4] + bayer[bayerStep4 +
                       2]) * 3.0f )*0.5f)
      + rgb[blue] * 6.0f;
            /* G at B */
            t1 = (((bayer[bayerStep + 2] + bayer[bayerStep2 + 1] +
                    bayer[bayerStep2 + 3] + bayer[bayerStep3 + 2])) *2.0f)
      - (bayer[2] + bayer[bayerStep2] +
         bayer[bayerStep2 + 4] + bayer[bayerStep4 + 2])
      + (rgb[blue] * 4.0f);
            rgb[-blue]= CLIP(t0 *0.125f);
            rgb[0]= CLIP(t1 *0.125f);
            bayer++;
            rgb += 3;
        }

        bayer -= width;
        rgb -= width * 3;

        blue = -blue;
        start_with_green = !start_with_green;
    }
    rgb=refrgb;
    width = rawData.width();
    height = rawData.height();
    for (int i=0;i<height;i++)
    	for (int j=0;j<width;j++)
    	{
    		RGB_converted[i][j].r=rgb[(i*width+j)*3 + 0];
    		RGB_converted[i][j].g=rgb[(i*width+j)*3 + 1];
    		RGB_converted[i][j].b=rgb[(i*width+j)*3 + 2];

    	}
    delete [] rgb;
}
