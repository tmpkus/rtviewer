////////////////////////////////////////////////////////////////
//
//		Fast demosaicing algorythm
//
//		copyright (c) 2008-2010  Emil Martinec <ejmartin@uchicago.edu>
//
//
// code dated: August 26, 2010
//
//	fast_demo.cc is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#include "../utils/LUT.h"
#include "fast_demo.h"

static LUTf dirwt;
#define SQR(a) ((a)*(a))
#define abs(a) ((a)<0?-(a):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))

//static void __attribute__((constructor)) setup_dirwt()
//{
//	dirwt(0x10000);
//	//set up directional weight function
//	for (int i=0; i<0x10000; i++)
//		dirwt[i] = 1.0f/SQR(1.0f+(float)i);
//}

/*
 static void __attribute__((destructor)) cleanup_dirwt()
 {
 delete [] dirwt;
 }
 */
void fast_demosaic::fast_demo(HDRImage & dest)
{
	dirwt(0x30000);
	//set up directional weight function
	for (int i = 0; i < 0x10000; i++)
		dirwt[i] = 1.0f / SQR(1.0f+(float)i/3.0f);
	//int winx=0, winy=0;
	//int winw=W, winh=H;
	/*
	 if (plistener) {
	 plistener->setProgressStr ("Fast demosaicing...");
	 plistener->setProgress (0.0);
	 }*/
	//float progress = 0.0;
	dest(W, H);

	//float my_gamma = 1.0f / 2.2f;
	cout << W << "x" << H << "starting conversion\n";
	cout << "step 1\n";
#define bord 4
	float min[H], max[H], r_black = black, g_black = black, b_black = black;
	int clip_pt = 4 * 65535;//* initialGain;
	const unsigned short **rawData = (const unsigned short **)data;
	float mat[3][3];
	{
		float channel_mul[3];

		// normalize color channels
		float min_channel = cam_mul[0];
		if (min_channel > cam_mul[1])
			min_channel = cam_mul[1];
		if (min_channel > cam_mul[2])
			min_channel = cam_mul[2];
		min_channel *= (get_white() - black); // full
		channel_mul[0] = cam_mul[0] / min_channel;
		channel_mul[1] = cam_mul[1] / min_channel;
		channel_mul[2] = cam_mul[2] / min_channel;

		for (int i = 0; i < 3; i++)
		{

			for (int j = 0; j < 3; j++)
			{
				mat[i][j] = rgb_cam[i][j] * channel_mul[j];
				cout << " " << rgb_cam[i][j];
			}
			cout << endl;
		}
	}
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#pragma omp parallel
	{
		// copy-in original 16bit raw data
		/*#pragma omp for
		 for (int row = 0; row < H; row++)
		 for (int col = 0; col < W; col++)
		 rawData[row][col] = data[row][col];*/

#pragma omp for
		//first, interpolate borders using bilinear
		for (int i = 0; i < H; i++)
		{
			for (int j = 0; j < bord; j++)
			{//first few columns
				unsigned int sum[6];
				for (int c = 0; c < 6; c++)
					sum[c] = 0;
				for (int i1 = i - 1; i1 < i + 2; i1++)
					for (int j1 = j - 1; j1 < j + 2; j1++)
					{
						if ((i1 > -1) && (i1 < H) && (j1 > -1))
						{
							int c = FC(i1, j1);
							sum[c] += rawData[i1][j1];
							sum[c + 3] ++;
						}
					}
				int c = FC(i, j);
				if (c == 1)
				{
					dest[i][j].r = (float)sum[0] / (float)sum[3];
					dest[i][j].g = (float)rawData[i][j];
					dest[i][j].b = (float)sum[2] / (float)sum[5];
				}
				else
				{
					dest[i][j].g = (float)sum[1] / (float)sum[4];
					if (c == 0)
					{
						dest[i][j].r = (float)rawData[i][j];
						dest[i][j].b = (float)sum[2] / (float)sum[5];
					}
					else
					{
						dest[i][j].r = (float)sum[0] / (float)sum[3];
						dest[i][j].b = (float)rawData[i][j];
					}
				}
			}//j

			for (int j = W - bord; j < W; j++)
			{//last few columns
				unsigned int sum[6];
				for (int c = 0; c < 6; c++)
					sum[c] = 0;
				for (int i1 = i - 1; i1 < i + 2; i1++)
					for (int j1 = j - 1; j1 < j + 2; j1++)
					{
						if ((i1 > -1) && (i1 < H) && (j1 < W))
						{
							int c = FC(i1, j1);
							sum[c] += rawData[i1][j1];
							sum[c + 3] ++;
						}
					}
				int c = FC(i, j);
				if (c == 1)
				{
					dest[i][j].r = (float)sum[0] / (float)sum[3];
					dest[i][j].g = (float)rawData[i][j];
					dest[i][j].b = (float)sum[2] / (float)sum[5];
				}
				else
				{
					dest[i][j].g = (float)sum[1] / (float)sum[4];
					if (c == 0)
					{
						dest[i][j].r = (float)rawData[i][j];
						dest[i][j].b = (float)sum[2] / (float)sum[5];
					}
					else
					{
						dest[i][j].r = (float)sum[0] / (float)sum[3];
						dest[i][j].b = (float)rawData[i][j];
					}
				}

			}//j
		}//i
		cout << "step 2\n";
		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#pragma omp for
		for (int j = bord; j < W - bord; j++)
		{
			for (int i = 0; i < bord; i++)
			{//first few rows
				unsigned int sum[6];
				for (int c = 0; c < 6; c++)
					sum[c] = 0;
				for (int i1 = i - 1; i1 < i + 2; i1++)
					for (int j1 = j - 1; j1 < j + 2; j1++)
					{
						if ((j1 > -1) && (j1 < W) && (i1 > -1))
						{
							int c = FC(i1, j1);
							sum[c] += rawData[i1][j1];
							sum[c + 3] ++;
						}
					}
				int c = FC(i, j);
				if (c == 1)
				{
					dest[i][j].r = (float)sum[0] / (float)sum[3];
					dest[i][j].g = (float)rawData[i][j];
					dest[i][j].b = (float)sum[2] / (float)sum[5];
				}
				else
				{
					dest[i][j].g = (float)sum[1] / (float)sum[4];
					if (c == 0)
					{
						dest[i][j].r = (float)rawData[i][j];
						dest[i][j].b = (float)sum[2] / (float)sum[5];
					}
					else
					{
						dest[i][j].r = (float)sum[0] / (float)sum[3];
						dest[i][j].b = (float)rawData[i][j];
					}
				}

			}//i

			for (int i = H - bord; i < H; i++)
			{//last few rows
				unsigned int sum[6];
				for (int c = 0; c < 6; c++)
					sum[c] = 0;
				for (int i1 = i - 1; i1 < i + 2; i1++)
					for (int j1 = j - 1; j1 < j + 2; j1++)
					{
						if ((j1 > -1) && (j1 < W) && (i1 < H))
						{
							int c = FC(i1, j1);
							sum[c] += rawData[i1][j1];
							sum[c + 3] ++;
						}
					}
				int c = FC(i, j);
				if (c == 1)
				{
					dest[i][j].r = (float)sum[0] / (float)sum[3];
					dest[i][j].g = (float)rawData[i][j];
					dest[i][j].b = (float)sum[2] / (float)sum[5];
				}
				else
				{
					dest[i][j].g = (float)sum[1] / (float)sum[4];
					if (c == 0)
					{
						dest[i][j].r = (float)rawData[i][j];
						dest[i][j].b = (float)sum[2] / (float)sum[5];
					}
					else
					{
						dest[i][j].r = (float)sum[0] / (float)sum[3];
						dest[i][j].b = (float)rawData[i][j];
					}
				}

			}//i
		}//j

		//if(plistener) plistener->setProgress(0.05);

		//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

		cout << "step 3\n";

#pragma omp for 
		// interpolate G using gradient weights
		for (int i = bord; i < H - bord; i++)
		{

			for (int j = bord; j < W - bord; j++)
			{

				if (FC(i, j) == 1)
				{
					dest[i][j].g = (float)rawData[i][j];
					//dest[i][j].r = dest[i][j].g;
					//dest[i][j].b = dest[i][j].g;

				}
				else
				{
					float wtu, wtd, wtl, wtr;
					//compute directional weights using image gradients
					wtu = dirwt[(abs(rawData[i+1][j]-rawData[i-1][j])
							+abs(rawData[i][j]-rawData[i-2][j])
							+abs(rawData[i-1][j]-rawData[i-3][j]))];
					wtd = dirwt[(abs(rawData[i-1][j]-rawData[i+1][j])
							+abs(rawData[i][j]-rawData[i+2][j])
							+abs(rawData[i+1][j]-rawData[i+3][j]))];
					wtl = dirwt[(abs(rawData[i][j+1]-rawData[i][j-1])
							+abs(rawData[i][j]-rawData[i][j-2])
							+abs(rawData[i][j-1]-rawData[i][j-3]))];
					wtr = dirwt[(abs(rawData[i][j-1]-rawData[i][j+1])
							+abs(rawData[i][j]-rawData[i][j+2])
							+abs(rawData[i][j+1]-rawData[i][j+3]))];

					//store in rgb array the interpolated G value at R/B grid points using directional weighted average
					float sum = wtu + wtd + wtl + wtr;
					dest[i][j].g = ((wtu * (float) rawData[i - 1][j] + wtd
							* (float) rawData[i + 1][j] + wtl
							* (float) rawData[i][j - 1] + wtr
							* (float) rawData[i][j + 1]) / sum);
					//dest[i][j].r = dest[i][j].g;
					//dest[i][j].b = dest[i][j].g;

				}
			}
			//progress+=(double)0.33/(H);
			//if(plistener) plistener->setProgress(progress);
		}
		//if(plistener) plistener->setProgress(0.4);

		cout << "step 4\n";
#pragma omp for 		
		for (int i = bord; i < H - bord; i++)
		{
			for (int j = bord + (FC(i, 2) & 1); j < W - bord; j += 2)
			{

				int c = FC(i, j);
				//interpolate B/R colors at R/B sites

				if (c == 0)
				{//R site
					dest[i][j].r = (float)rawData[i][j];
					dest[i][j].b
							= ((dest[i][j].g
									- 0.25f
											* ((dest[i - 1][j - 1].g + dest[i
													- 1][j + 1].g
													+ dest[i + 1][j + 1].g
													+ dest[i + 1][j - 1].g)
													- MIN(clip_pt,rawData[i-1][j-1]+rawData[i-1][j+1]+rawData[i+1][j+1]+rawData[i+1][j-1]))));
				}
				else
				{//B site
					dest[i][j].r
							= ((dest[i][j].g
									- 0.25f
											* ((dest[i - 1][j - 1].g + dest[i
													- 1][j + 1].g
													+ dest[i + 1][j + 1].g
													+ dest[i + 1][j - 1].g)
													- MIN(clip_pt,rawData[i-1][j-1]+rawData[i-1][j+1]+rawData[i+1][j+1]+rawData[i+1][j-1]))));
					dest[i][j].b = rawData[i][j];
				}
			}
			//progress+=(double)0.33/(H);
			//if(plistener) plistener->setProgress(progress);
		}
		//if(plistener) plistener->setProgress(0.7);

#pragma omp barrier
		cout << "step 5\n";
#pragma omp for 		

		// interpolate R/B using color differences
		for (int i = bord; i < H - bord; i++)
		{
			for (int j = bord + 1 - (FC(i, 2) & 1); j < W - bord; j += 2)
			{

				//interpolate R and B colors at G sites
				dest[i][j].r = ((dest[i][j].g - 0.25f * ((dest[i - 1][j].g
						- dest[i - 1][j].r) + (dest[i + 1][j].g
						- dest[i + 1][j].r) + (dest[i][j - 1].g
						- dest[i][j - 1].r) + (dest[i][j + 1].g
						- dest[i][j + 1].r))));
				dest[i][j].b = ((dest[i][j].g - 0.25f * ((dest[i - 1][j].g
						- dest[i - 1][j].b) + (dest[i + 1][j].g
						- dest[i + 1][j].b) + (dest[i][j - 1].g
						- dest[i][j - 1].b) + (dest[i][j + 1].g
						- dest[i][j + 1].b))));
			}
			//progress+=(double)0.33/(H);
			//if(plistener) plistener->setProgress(progress);
		}
		//if(plistener) plistener->setProgress(0.99);

#pragma omp for

		for (int i = 0; i < H; i++)
		{
			min[i] = 1000000.0f;
			max[i] = -1.0f;
			for (int j = 0; j < W; j++)
			{
				float r = dest[i][j].r - r_black;
				float g = dest[i][j].g - g_black;
				float b = dest[i][j].b - b_black;
				float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
				float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
				float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;

				/* clip pink
				 if (g > 1.0f && r > 1.0f)
				 r = g = b = 1.0f;*/
				if (ng > max[i])
					max[i] = ng;
				if (ng < min[i])
					min[i] = ng;
				dest[i][j].r = nr;
				dest[i][j].g = ng;
				dest[i][j].b = nb;
			}
		}
	}
	float Mg, mg;
	Mg = max[0];
	mg = min[0];
	for (int i = 1; i < H; i++)
	{
		if (max[i] > Mg)
			Mg = max[i];
		if (min[i] < mg)
			mg = min[i];
	}
	cout << " min/max" << mg << " / " << Mg << endl;

	cout << "ready conversion\n";
#undef bord
}
