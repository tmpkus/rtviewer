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


#define SQR(a) ((a)*(a))
#define abs(a) ((a)<0?-(a):(a))
#define MIN(a,b) ((a)<(b)?(a):(b))
#define DDIFF(a , b) (0.01f/0.01f+(a-b)*(a-b))
#define LDIFF(a , b) (ladjust/(ladjust+(a-b)*(a-b)))
#define SDIFF(a , b) (0.0001f/(0.0001f+(a-b)*(a-b)))
#define CDIFF(a , b) (adjust/(adjust+(a-b)*(a-b)))
#define CLIP( x ) (x)


#define HAVG(y,x) (0.25*(hint[y-1][x-1]+hint[y-1][x]+hint[y][x-1]+hint[y][x]))
#define HINT(y,x) (hint[y-1][x-1]-HAVG(y-1,x-1)+hint[y][x]-HAVG(y+1,x+1) + )


void fast_demosaic::cook_data(improps & props,int scale)
{
  cout << W << "x" << H << "starting conversion\n";
  cout << "jrp demosaic step 1\n";
  int sum[8];
  int c;
  resize=scale;
  recalc =1;
  memset(sum, 0, sizeof sum);
  for ( int row = 0 ; row < 8 ; row++ )
    for ( int col = 0 ; col < 8 ; col++ )
      {
        int val;
        c = FC(row, col);
        if ((val = white[row][col] - cblack[c]) > 0)
          {
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
    for ( int i = 0 ; i < 3 ; i++ )
      {
        for ( int j = 0 ; j < 3 ; j++ )
          {
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

  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      props.mat[i][j]=mat[i][j];

  cout << "setup tile management\n";
  int rot = get_rotateDegree() / 90;

  Tile_flags((W + 32 + TILE_SIZE - 1) / (TILE_SIZE),(H + 32 + TILE_SIZE - 1) / (TILE_SIZE)); // setup correct size

  for ( unsigned int ty = 0 ; ty < Tile_flags.height() ; ty++ )
    for ( unsigned int tx = 0 ; tx < Tile_flags.width() ; tx++ )
      Tile_flags[ty][tx] = 0; // set cleared;
  cout << "setup RGB data image\n";
  // check orientation

  if (scale ==1) method = AMAZE_DEMOSAIC;
  else method = VARSIZE_DEMOSAIC;

  if (rot & 1)
    RGB_converted(H, W);
  else RGB_converted(W, H);

  RGB_converted.moveto(0,0);
  RGB_converted.clear();
  float max_f=4.0f;
  if (first_pass)
    {
      rawData(W, H);
#pragma omp for
      for ( unsigned int y = 0 ; y < H ; y++ )
        {
          for ( unsigned int x = 0 ; x < W ; x++ )
            {
              // convert to float, scale and clip
              int c = FC(y, x);
              int v = data[y][x];
              float r = channel_mul[c] * ((float) v - (float) cblack[c]);
              rawData[y][x] = (r > max_f) ? max_f : ((r < 0.0f) ? 0.0f:r);
            }
        }
    }
  first_pass=0;

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


  int runtiles = 0;
  //cout << "rotation is: "<< rot << endl;

  if (tile_xs<0)
    {
      tile_xs=0;
      cout << " x too low" << endl;
    }
  if (tile_ys<0)
    {
      tile_ys=0;
      cout << " y too low" << endl;
    }
  if (tile_ye>=Tile_flags.height())
    {
      tile_ye=Tile_flags.height()-1;
      cout << " y too high" << endl;
    }
  if (tile_xe>=Tile_flags.width())
    {
      tile_xe=Tile_flags.width()-1;;
      cout << " x too high" << endl;
    }
  //cout << "tiles from " << tile_xs << "x" << tile_ys << " to " << tile_xe << "x" << tile_ye <<endl;


  for ( ; tile_ys <= tile_ye ; tile_ys++ )
    for ( int tx = tile_xs ; tx <= tile_xe ; tx++ )
      if (Tile_flags[tile_ys][tx] !=2)
        {
          Tile_flags[tile_ys][tx] = 1;
          runtiles++;
        }
  //cout << " done runtiles" << endl;
  return runtiles;
}

// tiles don't match resize algo
// so we don't use them..
// the algo is very quick anyway

void fast_demosaic::nth_size_demo(HDRImage & dest,int num,improps & props)
{
  int tile_xs, tile_ys, tile_xe, tile_ye;
  int rot = (get_rotateDegree() / 90) & 3;
  if (recalc)
    {
      recalc=0;
#pragma omp parallel for
      for ( int Y = 0 ; Y < (H-num) ; Y += num )
        {

          int mrow = (H/num)-1;
          int mcol = (W/num)-1;
          for ( int X = 0 ; X < (W-num) ; X += num )
            {
              float r=0,b=0,g=0,nr=0,ng=0,nb=0;
              for (int ty=Y; ty<Y+num; ty++)
                for(int tx=X; tx<X+num; tx++)
                  {
                    int color=FC(tx, ty);
                    if (color == 1) // green here
                      {
                        g += rawData[ty][tx];
                        ng+=1.0;
                      }
                    else if (color == 0)  //red next
                      {
                        r += rawData[ty][tx];
                        nr+=1.0;
                      }
                    else
                      {
                        b += rawData[ty][tx];
                        nb+=1.0;
                      }
                  }
              r=r/nr;
              g=g/ng;
              b=b/nb;
              int row = Y / num;
              int col = X / num;
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

        }
    }
  cout << "done demosaicing" << endl;
  RGB_converted.moveto(-dest.xoff(), -dest.yoff());
  dest <<= RGB_converted;
  cout << "done demosaicing" << endl;
}

