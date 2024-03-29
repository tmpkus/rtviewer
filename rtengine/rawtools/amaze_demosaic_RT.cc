////////////////////////////////////////////////////////////////
//
//			AMaZE demosaic algorithm
// (Aliasing Minimization and Zipper Elimination)
//
//	copyright (c) 2008-2010  Emil Martinec <ejmartin@uchicago.edu>
//
// incorporating ideas of Luis Sanz Rodrigues and Paul Lee
//
// code dated: May 27, 2010
//
//	amaze_interpolate_RT.cc is free software: you can redistribute it and/or modify
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

#include "ImageRaw.h"
#include "fast_demo.h"
//#define SQR(x) ((x)*(x))
template<typename T> static inline T SQR(T x)
{
  return x*x;
};
//#define MIN(a,b) ((a) < (b) ? (a) : (b))
template<typename T> static inline T MIN(T a,T b)
{
  return((a) < (b) ? (a) : (b));
};
//#define MAX(a,b) ((a) > (b) ? (a) : (b))
template<typename T> static inline T MAX(T a,T b)
{
  return((a) > (b) ? (a) : (b));
};
//#define LIM(x,min,max) MAX(min,MIN(x,max))
template<typename T> static inline T LIM(T x,T min,T max)
{
  return MAX(min,MIN(x,max));
};
//#define ULIM(x,y,z) ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y))
template <typename T> static inline T ULIM(T x,T y,T z)
{
  return ((y) < (z) ? LIM(x,y,z) : LIM(x,z,y)) ;
}
#define HCLIP(x) x //is this still necessary???

//#define fabs( x ) ((x)>=0.0f?(x):-(x))
template <typename T> static inline T fabs(T x )
{
  return ((x)>=0.0f?(x):-(x));
}
//#define Swap( x , y ) { typeof(x) temp = x;x=y;y=temp;}
template <typename T> static inline void Swap(T &x,T &y)
{
  T temp=x;
  x=y;
  y=temp ;
};
//#define Myfc(y,x) FC(((y)+winy),((x)+winx))
#define Myfc(y,x) FC((y),(x))

void fast_demosaic::amaze_demosaic_RT(HDRImage & dest,improps &props)
{
  //int tile_xs,tile_ys,tile_xe,tile_ye;
  int rot = (get_rotateDegree() / 90) & 3;
  if (touch_tiles(dest) > 0)
    {
      int winx = 0, winy = 0, winw = W, winh = H;
      //cout << " amaze: running " << runtiles << " tiles\n";
      //MIN(clip_pt,x)
      //const float expcomp = props.pp3["[Exposure]"]["Compensation"];
      const float scale_raw_data = 1.0f; //pow(2.0,props.expcomp)/65535.0f;
      //const float post_scale = pow(2.0, expcomp);
      int width = winw, height = winh;

      const float clip_pt = 4.0; // = 1/initialGain;

//#define TS 64	 // Tile size; the image is processed in square tiles to lower memory requirements and facilitate multi-threading
      // local variables

      //offset of R pixel within a Bayer quartet
      int ex, ey;

      //shifts of pointer value to access pixels in vertical and diagonal directions
      static const int v1 = TS, v2 = 2 * TS, v3 = 3 * TS, p1 = -TS + 1, p2 = -2 * TS + 2, p3 = -3 * TS + 3, m1 = TS + 1, m2 = 2 * TS + 2, m3 = 3 * TS + 3;

      //neighborhood of a pixel
      //static const int nbr[5] = { -v2, -2, 2, v2, 0 };

      //tolerance to avoid dividing by zero
      static const float eps = 1e-5, epssq = 1e-10; //tolerance to avoid dividing by zero

      //adaptive ratios threshold
      static const float arthresh = 0.75;
      //nyquist texture test threshold
      static const float nyqthresh = 0.5;
      //diagonal interpolation test threshold
      //static const float pmthresh = 0.25;
      //factors for bounding interpolation in saturated regions
      //static const float lbd = 1.0, ubd = 1.0; //lbd=0.66, ubd=1.5 alternative values;

      //gaussian on 5x5 quincunx, sigma=1.2
      static const float gaussodd[4] = { 0.14659727707323927f, 0.103592713382435f, 0.0732036125103057f, 0.0365543548389495f };
      //gaussian on 5x5, sigma=1.2
      static const float gaussgrad[6] = { 0.07384411893421103f, 0.06207511968171489f, 0.0521818194747806f, 0.03687419286733595f, 0.03099732204057846f, 0.018413194161458882f };
      //gaussian on 3x3, sigma =0.7
      //static const float gauss1[3] = { 0.3376688223162362f, 0.12171198028231786f, 0.04387081413862306f };
      //gaussian on 5x5 alt quincunx, sigma=1.5
      static const float gausseven[2] = { 0.13719494435797422f, 0.05640252782101291f };
      //guassian on quincunx grid
      static const float gquinc[4] = { 0.169917f, 0.108947f, 0.069855f, 0.0287182f };

      //volatile double progress = 0.0;
      //const unsigned short **rawData = (const unsigned short **)data;

      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#pragma omp parallel
      {
        //position of top/left corner of the tile
        //int top, left;
        // beginning of storage block for tile
        float *buffer;
        // rgb values
        float (*rgb)[3];
        // horizontal gradient
        float (*delh);
        // vertical gradient
        float (*delv);
        // square of delh
        float (*delhsq);
        // square of delv
        float (*delvsq);
        // gradient based directional weights for interpolation
        float (*dirwts)[2];
        // vertically interpolated color differences G-R, G-B
        float (*vcd);
        // horizontally interpolated color differences
        float (*hcd);
        // alternative vertical interpolation
        float (*vcdalt);
        // alternative horizontal interpolation
        float (*hcdalt);
        // square of average color difference
        float (*cddiffsq);
        // weight to give horizontal vs vertical interpolation
        float (*hvwt);
        // final interpolated color difference
        float (*Dgrb)[2];
        // gradient in plus (NE/SW) direction
        float (*delp);
        // gradient in minus (NW/SE) direction
        float (*delm);
        // diagonal interpolation of R+B
        float (*rbint);
        // horizontal curvature of interpolated G (used to refine interpolation in Nyquist texture regions)
        float (*Dgrbh2);
        // vertical curvature of interpolated G
        float (*Dgrbv2);
        // difference between up/down interpolations of G
        float (*dgintv);
        // difference between left/right interpolations of G
        float (*dginth);
        // diagonal (plus) color difference R-B or G1-G2
        float (*Dgrbp1);
        // diagonal (minus) color difference R-B or G1-G2
        float (*Dgrbm1);
        // square of diagonal color difference
        float (*Dgrbpsq1);
        // square of diagonal color difference
        float (*Dgrbmsq1);
        // tile raw data
        float (*cfa);
        // relative weight for combining plus and minus diagonal interpolations
        float (*pmwt);
        // interpolated color difference R-B in plus direction
        float (*rbp);
        // interpolated color difference R-B in minus direction
        float (*rbm);

        // nyquist texture flag 1=nyquist, 0=not nyquist
        char (*nyquist);

        // assign working space
        buffer = new float[33 * TS * TS];
        //merror(buffer,"amaze_interpolate()");
        //memset(buffer,0,(34*sizeof(float)+sizeof(int))*TS*TS);
        // rgb array
        rgb	= (float(*)[3]) buffer; //pointers to array
        delh = (float(*)) (buffer + 3 * TS * TS);
        delv = (float(*)) (buffer + 4 * TS * TS);
        delhsq = (float(*)) (buffer + 5 * TS * TS);
        delvsq = (float(*)) (buffer + 6 * TS * TS);
        dirwts = (float(*)[2]) (buffer + 7 * TS * TS);
        vcd = (float(*)) (buffer + 9 * TS * TS);
        hcd = (float(*)) (buffer + 10 * TS * TS);
        vcdalt = (float(*)) (buffer + 11 * TS * TS);
        hcdalt = (float(*)) (buffer + 12 * TS * TS);
        cddiffsq = (float(*)) (buffer + 13 * TS * TS);
        hvwt = (float(*)) (buffer + 14 * TS * TS);
        Dgrb = (float(*)[2]) (buffer + 15 * TS * TS);
        delp = (float(*)) (buffer + 17 * TS * TS);
        delm = (float(*)) (buffer + 18 * TS * TS);
        rbint = (float(*)) (buffer + 19 * TS * TS);
        Dgrbh2 = (float(*)) (buffer + 20 * TS * TS);
        Dgrbv2 = (float(*)) (buffer + 21 * TS * TS);
        dgintv = (float(*)) (buffer + 22 * TS * TS);
        dginth = (float(*)) (buffer + 23 * TS * TS);
        Dgrbp1 = (float(*)) (buffer + 24 * TS * TS);
        Dgrbm1 = (float(*)) (buffer + 25 * TS * TS);
        Dgrbpsq1 = (float(*)) (buffer + 26 * TS * TS);
        Dgrbmsq1 = (float(*)) (buffer + 27 * TS * TS);
        cfa = (float(*)) (buffer + 28 * TS * TS);
        pmwt = (float(*)) (buffer + 29 * TS * TS);
        rbp = (float(*)) (buffer + 30 * TS * TS);
        rbm = (float(*)) (buffer + 31 * TS * TS);

        nyquist = (char(*)) (buffer + 32 * TS * TS);

        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        /*double dt;
         clock_t t1, t2;

         clock_t t1_init,       t2_init       = 0;
         clock_t t1_vcdhcd,      t2_vcdhcd      = 0;
         clock_t t1_cdvar,		t2_cdvar = 0;
         clock_t t1_nyqtest,   t2_nyqtest   = 0;
         clock_t t1_areainterp,  t2_areainterp  = 0;
         clock_t t1_compare,   t2_compare   = 0;
         clock_t t1_diag,   t2_diag   = 0;
         clock_t t1_chroma,    t2_chroma    = 0;*/

        // start
        //if (verbose) fprintf (stderr,_("AMaZE interpolation ...\n"));
        //t1 = clock();
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        /*
         if (plistener) {
         plistener->setProgressStr ("AMaZE Demosaicing...");
         plistener->setProgress (0.0);
         }
         */
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //determine GRBG coset; (ey,ex) is the offset of the R subarray
        if (Myfc(0,0) == 1)   //first pixel is G
          {
            if (Myfc(0,1) == 0)
              {
                ey = 0;
                ex = 1;
              }
            else
              {
                ey = 1;
                ex = 0;
              }
          }
        else   //first pixel is R or B
          {
            if (Myfc(0,0) == 0)
              {
                ey = 0;
                ex = 0;
              }
            else
              {
                ey = 1;
                ex = 1;
              }
          }

        // Main algorithm: Tile loop
        //#pragma omp parallel for shared(rawData,height,width,red,green,blue) private(top,left) schedule(dynamic)
        //code is openmp ready; just have to pull local tile variable declarations inside the tile loop

        // schedule(dynamic) nowait
#pragma omp for
        for (int top = winy - 16; top < winy + height ; top += TILE_SIZE )
          for ( int left = winx - 16; left < winx + width ; left += TILE_SIZE )
            if (Tile_flags[(top + 16) / TILE_SIZE][(left + 16) / TILE_SIZE] == 1) // not yet transformed.
              {
                Tile_flags[(top + 16) / TILE_SIZE][(left + 16) / TILE_SIZE] = 2;
                //location of tile bottom edge
                int bottom = MIN( top+TS,winy+height+16);
                //location of tile right edge
                int right = MIN(left+TS, winx+width+16);
                //tile width  (=TS except for right edge of image)
                int rr1 = bottom - top;
                //tile height (=TS except for bottom edge of image)
                int cc1 = right - left;

                //tile vars
                //counters for pixel location in the image
//						int row, col;
                //min and max row/column in the tile
                int rrmin, rrmax, ccmin, ccmax;
                //counters for pixel location within the tile
//						int rr, cc;
                //color index 0=R, 1=G, 2=B
//						int c;
                //pointer counters within the tile
//						int indx, indx1;
                //direction counter for nbrs[]
//						int dir;
                //dummy indices
//						int i, j;
                // +1 or -1
//						int sgn;

                //color ratios in up/down/left/right directions
//						float cru, crd, crl, crr;
                //adaptive weights for vertical/horizontal/plus/minus directions
//						float vwt, hwt, pwt, mwt;
                //vertical and horizontal G interpolations
//						float Gintv, Ginth;
                //G interpolated in vert/hor directions using adaptive ratios
//						float guar, gdar, glar, grar;
                //G interpolated in vert/hor directions using Hamilton-Adams method
//						float guha, gdha, glha, grha;
                //interpolated G from fusing left/right or up/down
//						float Ginthar, Ginthha, Gintvar, Gintvha;
                //color difference (G-R or G-B) variance in up/down/left/right directions
//						float Dgrbvvaru, Dgrbvvard, Dgrbhvarl, Dgrbhvarr;
                //gradients in various directions
//						float gradp, gradm, gradv, gradh, gradpm, gradhv;
                //color difference variances in vertical and horizontal directions
//						float vcdvar, hcdvar, vcdvar1, hcdvar1, hcdaltvar, vcdaltvar;
                //adaptive interpolation weight using variance of color differences
//						float varwt;
                //adaptive interpolation weight using difference of left-right and up-down G interpolations
//						float diffwt;
                //alternative adaptive weight for combining horizontal/vertical interpolations
//						float hvwtalt;
                //temporary variables for combining interpolation weights at R and B sites
//						float vo, ve;
                //interpolation of G in four directions
//						float gu, gd, gl, gr;
                //variance of G in vertical/horizontal directions
//						float gvarh, gvarv;

                //Nyquist texture test
//						float nyqtest;
                //accumulators for Nyquist texture interpolation
//						float sumh, sumv, sumsqh, sumsqv, areawt;

                //color ratios in diagonal directions
//						float crse, crnw, crne, crsw;
                //color differences in diagonal directions
//						float rbse, rbnw, rbne, rbsw;
                //adaptive weights for combining diagonal interpolations
//						float wtse, wtnw, wtsw, wtne;
                //alternate weight for combining diagonal interpolations
//						float pmwtalt;
                //variance of R-B in plus/minus directions
//						float rbvarp, rbvarm;

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                // rgb from input CFA data
                // rgb values should be floating point number between 0 and 1
                // after white balance multipliers are applied
                // a 16 pixel border is added to each side of the image

                // bookkeeping for borders
                if (top < winy)
                  {
                    rrmin = 16;
                  }
                else
                  {
                    rrmin = 0;
                  }
                if (left < winx)
                  {
                    ccmin = 16;
                  }
                else
                  {
                    ccmin = 0;
                  }
                if (bottom > (winy + height))
                  {
                    rrmax = winy + height - top;
                  }
                else
                  {
                    rrmax = rr1;
                  }
                if (right > (winx + width))
                  {
                    ccmax = winx + width - left;
                  }
                else
                  {
                    ccmax = cc1;
                  }

                for (unsigned int rr = rrmin; rr < rrmax ; rr++ )
                  for ( unsigned int row = rr + top, cc = ccmin; cc < ccmax ; cc++ )
                    {
                      int col = cc + left;
                      unsigned int c = Myfc(rr,cc);
                      unsigned int indx1 = rr * TS + cc;
                      rgb[indx1][c] = ((float) rawData[row][col]) * scale_raw_data;
                      //indx=row*width+col;
                      //rgb[indx1][c] = image[indx][c]*scale_raw_data;//for dcraw implementation

                      cfa[indx1] = rgb[indx1][c];
                    }
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                //fill borders
                if (rrmin > 0)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = ccmin; cc < ccmax ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[rr * TS + cc][c] = rgb[(32 - rr) * TS + cc][c];
                          cfa[rr * TS + cc] = rgb[rr * TS + cc][c];
                        }
                  }
                if (rrmax < rr1)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = ccmin; cc < ccmax ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[(rrmax + rr) * TS + cc][c] = ((float) rawData[(winy + height - rr - 2)][left
                                                            + cc]) * scale_raw_data;
                          //rgb[(rrmax+rr)*TS+cc][c] = (image[(height-rr-2)*width+left+cc][c])*scale_raw_data;//for dcraw implementation
                          cfa[(rrmax + rr) * TS + cc] = rgb[(rrmax + rr) * TS + cc][c];
                        }
                  }
                if (ccmin > 0)
                  {
                    for ( int rr = rrmin; rr < rrmax ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[rr * TS + cc][c] = rgb[rr * TS + 32 - cc][c];
                          cfa[rr * TS + cc] = rgb[rr * TS + cc][c];
                        }
                  }
                if (ccmax < cc1)
                  {
                    for ( int rr = rrmin; rr < rrmax ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[rr * TS + ccmax + cc][c] = ((float) rawData[(top + rr)][(winx + width - cc - 2)]) * scale_raw_data;
                          //rgb[rr*TS+ccmax+cc][c] = (image[(top+rr)*width+(width-cc-2)][c])*scale_raw_data;//for dcraw implementation
                          cfa[rr * TS + ccmax + cc] = rgb[rr * TS + ccmax + cc][c];
                        }
                  }

                //also, fill the image corners
                if (rrmin > 0 && ccmin > 0)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[(rr) * TS + cc][c] = ((float) rawData[winy + 32 - rr][winx + 32 - cc]) * scale_raw_data;
                          //rgb[(rr)*TS+cc][c] = (rgb[(32-rr)*TS+(32-cc)][c]);//for dcraw implementation
                          cfa[(rr) * TS + cc] = rgb[(rr) * TS + cc][c];
                        }
                  }
                if (rrmax < rr1 && ccmax < cc1)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[(rrmax + rr) * TS + ccmax + cc][c] = ((float) rawData[(winy + height - rr - 2)][(winx + width - cc - 2)]) * scale_raw_data;
                          //rgb[(rrmax+rr)*TS+ccmax+cc][c] = (image[(height-rr-2)*width+(width-cc-2)][c])*scale_raw_data;//for dcraw implementation
                          cfa[(rrmax + rr) * TS + ccmax + cc] = rgb[(rrmax + rr) * TS + ccmax + cc][c];
                        }
                  }
                if (rrmin > 0 && ccmax < cc1)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[(rr) * TS + ccmax + cc][c] = ((float) rawData[(winy + 32 - rr)][(winx + width - cc - 2)]) * scale_raw_data;
                          //rgb[(rr)*TS+ccmax+cc][c] = (image[(32-rr)*width+(width-cc-2)][c])*scale_raw_data;//for dcraw implementation
                          cfa[(rr) * TS + ccmax + cc] = rgb[(rr) * TS + ccmax + cc][c];
                        }
                  }
                if (rrmax < rr1 && ccmin > 0)
                  {
                    for ( int rr = 0; rr < 16 ; rr++ )
                      for ( int cc = 0; cc < 16 ; cc++ )
                        {
                          int c = Myfc(rr,cc);
                          rgb[(rrmax + rr) * TS + cc][c] = ((float) rawData[(winy + height - rr - 2)][(winx + 32 - cc)]) * scale_raw_data;
                          //rgb[(rrmax+rr)*TS+cc][c] = (image[(height-rr-2)*width+(32-cc)][c])*scale_raw_data;//for dcraw implementation
                          cfa[(rrmax + rr) * TS + cc] = rgb[(rrmax + rr) * TS + cc][c];
                        }
                  }

                //end of border fill
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                for ( int rr = 1; rr < rr1 - 1 ; rr++ )
                  for ( int cc = 1, indx = (rr) * TS + cc; cc < cc1 - 1 ; cc++, indx++ )
                    {

                      delh[indx] = fabs(cfa[indx+1]-cfa[indx-1]);
                      delv[indx] = fabs(cfa[indx+v1]-cfa[indx-v1]);
                      delhsq[indx] = SQR(delh[indx]);
                      delvsq[indx] = SQR(delv[indx]);
                      delp[indx] = fabs(cfa[indx+p1]-cfa[indx-p1]);
                      delm[indx] = fabs(cfa[indx+m1]-cfa[indx-m1]);

                    }

                for (int  rr = 2; rr < rr1 - 2 ; rr++ )
                  for ( int cc = 2, indx = (rr) * TS + cc; cc < cc1 - 2 ; cc++, indx++ )
                    {

                      dirwts[indx][0] = eps + delv[indx + v1] + delv[indx - v1] + delv[indx]; //+fabs(cfa[indx+v2]-cfa[indx-v2]);
                      //vert directional averaging weights
                      dirwts[indx][1] = eps + delh[indx + 1] + delh[indx - 1] + delh[indx]; //+fabs(cfa[indx+2]-cfa[indx-2]);
                      //horizontal weights

                      if (Myfc(rr,cc) & 1)
                        {
                          //for later use in diagonal interpolation
                          //Dgrbp1[indx]=2*cfa[indx]-(cfa[indx-p1]+cfa[indx+p1]);
                          //Dgrbm1[indx]=2*cfa[indx]-(cfa[indx-m1]+cfa[indx+m1]);
                          Dgrbpsq1[indx] = (SQR(cfa[indx]-cfa[indx-p1]) + SQR(cfa[indx]-cfa[indx+p1]));
                          Dgrbmsq1[indx] = (SQR(cfa[indx]-cfa[indx-m1]) + SQR(cfa[indx]-cfa[indx+m1]));
                        }
                    }

                //t2_init += clock()-t1_init;
                // end of tile initialization
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                //interpolate vertical and horizontal color differences
                //t1_vcdhcd = clock();

                for ( int rr = 4; rr < rr1 - 4 ; rr++ )
                  //for (cc=4+(Myfc(rr,2)&1),indx=rr*TS+cc,c=Myfc(rr,cc); cc<cc1-4; cc+=2,indx+=2) {
                  for ( int cc = 4, indx = rr * TS + cc; cc < cc1 - 4 ; cc++, indx++ )
                    {
                      float cfaindx = cfa[indx];
                      int c = Myfc(rr,cc);
                      //if (c&1) {sgn=-1;} else {sgn=1;}

                      //initialization of nyquist test
                      nyquist[indx] = 0;
                      //preparation for diag interp
                      rbint[indx] = 0;

                      //color ratios in each cardinal direction
                      float cru = cfa[indx - v1] * (dirwts[indx - v2][0] + dirwts[indx][0])
                                  / (dirwts[indx - v2][0] * (eps + cfaindx)
                                     + dirwts[indx][0] * (eps + cfa[indx - v2]));
                      float crd = cfa[indx + v1] * (dirwts[indx + v2][0] + dirwts[indx][0])
                                  / (dirwts[indx + v2][0] * (eps + cfaindx)
                                     + dirwts[indx][0] * (eps + cfa[indx + v2]));
                      float crl = cfa[indx - 1] * (dirwts[indx - 2][1] + dirwts[indx][1])
                                  / (dirwts[indx - 2][1] * (eps + cfaindx)
                                     + dirwts[indx][1] * (eps + cfa[indx - 2]));
                      float crr = cfa[indx + 1] * (dirwts[indx + 2][1] + dirwts[indx][1])
                                  / (dirwts[indx + 2][1] * (eps + cfaindx)
                                     + dirwts[indx][1] * (eps + cfa[indx + 2]));

                      float guha = HCLIP(cfa[indx-v1]) + 0.5f * (cfaindx - cfa[indx - v2]);
                      float gdha = HCLIP(cfa[indx+v1]) + 0.5f * (cfaindx - cfa[indx + v2]);
                      float glha = HCLIP(cfa[indx-1]) + 0.5f * (cfaindx - cfa[indx - 2]);
                      float grha = HCLIP(cfa[indx+1]) + 0.5f * (cfaindx - cfa[indx + 2]);

                      float guar=(fabs(1.0f-cru) < arthresh)?cfaindx * cru:guha;

                      float gdar=(fabs(1.0f-crd) < arthresh)?cfaindx * crd:gdha;
                      float glar=(fabs(1.0f-crl) < arthresh)?cfaindx * crl:glha;
                      float grar=(fabs(1.0f-crr) < arthresh)?cfaindx * crr:grha;


                      float hwt = dirwts[indx - 1][1] / (dirwts[indx - 1][1] + dirwts[indx + 1][1]);
                      float vwt = dirwts[indx - v1][0] / (dirwts[indx + v1][0] + dirwts[indx - v1][0]);

                      //interpolated G via adaptive weights of cardinal evaluations
                      float Gintvar = vwt * gdar + (1.0f - vwt) * guar;
                      float Ginthar = hwt * grar + (1.0f - hwt) * glar;
                      float Gintvha = vwt * gdha + (1.0f - vwt) * guha;
                      float Ginthha = hwt * grha + (1.0f - hwt) * glha;
                      if (c & 1)
                        {
                          //interpolated color differences
                          vcd[indx] = cfaindx - Gintvar;
                          hcd[indx] = cfaindx - Ginthar;
                          vcdalt[indx] = cfaindx - Gintvha;
                          hcdalt[indx] = cfaindx - Ginthha;
                        }
                      else
                        {
                          vcd[indx] = Gintvar - cfaindx;
                          hcd[indx] = Ginthar - cfaindx;
                          vcdalt[indx] = Gintvha - cfaindx;
                          hcdalt[indx] = Ginthha - cfaindx;

                        }
                      if (cfaindx > 0.8f * clip_pt || Gintvha > 0.8f * clip_pt || Ginthha > 0.8f * clip_pt)
                        {
                          //use HA if highlights are (nearly) clipped
                          guar = guha;
                          gdar = gdha;
                          glar = glha;
                          grar = grha;
                          vcd[indx] = vcdalt[indx];
                          hcd[indx] = hcdalt[indx];
                        }

                      //differences of interpolations in opposite directions
                      dgintv[indx] = MIN(SQR(guha-gdha),SQR(guar-gdar));
                      dginth[indx] = MIN(SQR(glha-grha),SQR(glar-grar));

                    }
                //t2_vcdhcd += clock() - t1_vcdhcd;

                //t1_cdvar = clock();
                for ( int rr = 4; rr < rr1 - 4 ; rr++ )
                  //for (cc=4+(Myfc(rr,2)&1),indx=rr*TS+cc,c=Myfc(rr,cc); cc<cc1-4; cc+=2,indx+=2) {
                  for ( int cc = 4, indx = rr * TS + cc; cc < cc1 - 4 ; cc++, indx++ )
                    {
                      int c = Myfc(rr,cc);

                      float hcdvar = 3.0f * (SQR(hcd[indx-2]) + SQR(hcd[indx]) + SQR(hcd[indx+2]))
                                     - SQR(hcd[indx-2]+hcd[indx]+hcd[indx+2]);
                      float hcdaltvar = 3.0f * (SQR(hcdalt[indx-2]) + SQR(hcdalt[indx]) + SQR(hcdalt[indx+2]))
                                        - SQR(hcdalt[indx-2]+hcdalt[indx]+hcdalt[indx+2]);
                      float vcdvar = 3.0f * (SQR(vcd[indx-v2]) + SQR(vcd[indx]) + SQR(vcd[indx+v2]))
                                     - SQR(vcd[indx-v2]+vcd[indx]+vcd[indx+v2]);
                      float vcdaltvar = 3.0f * (SQR(vcdalt[indx-v2]) + SQR(vcdalt[indx]) + SQR(vcdalt[indx+v2]))
                                        - SQR(vcdalt[indx-v2]+vcdalt[indx]+vcdalt[indx+v2]);
                      //choose the smallest variance; this yields a smoother interpolation
                      if (hcdaltvar < hcdvar)
                        hcd[indx] = hcdalt[indx];
                      if (vcdaltvar < vcdvar)
                        vcd[indx] = vcdalt[indx];

                      //bound the interpolation in regions of high saturation
                      if (c & 1)   //G site
                        {
                          float Ginth = cfa[indx] - hcd[indx]; //R or B
                          float Gintv = cfa[indx] - vcd[indx]; //B or R

                          if (hcd[indx] > 0)
                            {
                              if (3.0f * hcd[indx] > (Ginth + cfa[indx]))
                                {
                                  hcd[indx] = -ULIM(Ginth,cfa[indx-1],cfa[indx+1]) + cfa[indx];
                                }
                              else
                                {
                                  float hwt = 1.0f - 3.0f * hcd[indx] / (eps + Ginth + cfa[indx]);
                                  hcd[indx] = hwt * hcd[indx] + (1.0f - hwt) * (-ULIM(Ginth,cfa[indx-1],cfa[indx+1]) + cfa[indx]);
                                }
                            }
                          if (vcd[indx] > 0)
                            {
                              if (3.0f * vcd[indx] > (Gintv + cfa[indx]))
                                {
                                  vcd[indx] = -ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) + cfa[indx];
                                }
                              else
                                {
                                  float vwt = 1.0f - 3.0f * vcd[indx] / (eps + Gintv + cfa[indx]);
                                  vcd[indx] = vwt * vcd[indx] + (1.0f - vwt) * (-ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) + cfa[indx]);
                                }
                            }

                          if (Ginth > clip_pt)
                            hcd[indx] = -ULIM(Ginth,cfa[indx-1],cfa[indx+1]) + cfa[indx]; //for RT implementation
                          if (Gintv > clip_pt)
                            vcd[indx] = -ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) + cfa[indx];
                          //if (Ginth > pre_mul[c]) hcd[indx]=-ULIM(Ginth,cfa[indx-1],cfa[indx+1])+cfa[indx];//for dcraw implementation
                          //if (Gintv > pre_mul[c]) vcd[indx]=-ULIM(Gintv,cfa[indx-v1],cfa[indx+v1])+cfa[indx];

                        }
                      else   //R or B site
                        {

                          float Ginth = hcd[indx] + cfa[indx]; //interpolated G
                          float Gintv = vcd[indx] + cfa[indx];

                          if (hcd[indx] < 0)
                            {
                              if (3.0f * hcd[indx] < -(Ginth + cfa[indx]))
                                {
                                  hcd[indx] = ULIM(Ginth,cfa[indx-1],cfa[indx+1]) - cfa[indx];
                                }
                              else
                                {
                                  float hwt = 1.0f + 3.0f * hcd[indx] / (eps + Ginth + cfa[indx]);
                                  hcd[indx] = hwt * hcd[indx] + (1.0f - hwt) * (ULIM(Ginth,cfa[indx-1],cfa[indx+1]) - cfa[indx]);
                                }
                            }
                          if (vcd[indx] < 0)
                            {
                              if (3.0f * vcd[indx] < -(Gintv + cfa[indx]))
                                {
                                  vcd[indx] = ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) - cfa[indx];
                                }
                              else
                                {
                                  float vwt = 1.0f + 3.0f * vcd[indx] / (eps + Gintv + cfa[indx]);
                                  vcd[indx] = vwt * vcd[indx] + (1.0f - vwt) * (ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) - cfa[indx]);
                                }
                            }

                          if (Ginth > clip_pt)
                            hcd[indx] = ULIM(Ginth,cfa[indx-1],cfa[indx+1]) - cfa[indx]; //for RT implementation
                          if (Gintv > clip_pt)
                            vcd[indx] = ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]) - cfa[indx];
                          //if (Ginth > pre_mul[c]) hcd[indx]=ULIM(Ginth,cfa[indx-1],cfa[indx+1])-cfa[indx];//for dcraw implementation
                          //if (Gintv > pre_mul[c]) vcd[indx]=ULIM(Gintv,cfa[indx-v1],cfa[indx+v1])-cfa[indx];
                        }

                      cddiffsq[indx] = SQR(vcd[indx]-hcd[indx]);
                    }

                for ( int rr = 6; rr < rr1 - 6 ; rr++ )
                  for ( int cc = 6 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 6 ; cc += 2, indx += 2 )
                    {

                      //compute color difference variances in cardinal directions

                      float uave = vcd[indx] + vcd[indx - v1] + vcd[indx - v2] + vcd[indx - v3];
                      float dave = vcd[indx] + vcd[indx + v1] + vcd[indx + v2] + vcd[indx + v3];
                      float lave = (hcd[indx] + hcd[indx - 1] + hcd[indx - 2] + hcd[indx - 3]);
                      float rave = (hcd[indx] + hcd[indx + 1] + hcd[indx + 2] + hcd[indx + 3]);

                      float Dgrbvvaru = SQR(vcd[indx]-uave) + SQR(vcd[indx-v1]-uave) + SQR(vcd[indx-v2]-uave) + SQR(vcd[indx-v3]-uave);
                      float Dgrbvvard = SQR(vcd[indx]-dave) + SQR(vcd[indx+v1]-dave) + SQR(vcd[indx+v2]-dave) + SQR(vcd[indx+v3]-dave);
                      float Dgrbhvarl = SQR(hcd[indx]-lave) + SQR(hcd[indx-1]-lave) + SQR(hcd[indx-2]-lave) + SQR(hcd[indx-3]-lave);
                      float Dgrbhvarr = SQR(hcd[indx]-rave) + SQR(hcd[indx+1]-rave) + SQR(hcd[indx+2]-rave) + SQR(hcd[indx+3]-rave);

                      float hwt = dirwts[indx - 1][1] / (dirwts[indx - 1][1] + dirwts[indx + 1][1]);
                      float vwt = dirwts[indx - v1][0] / (dirwts[indx + v1][0] + dirwts[indx - v1][0]);

                      float vcdvar = epssq + vwt * Dgrbvvard + (1.0f - vwt) * Dgrbvvaru;
                      float hcdvar = epssq + hwt * Dgrbhvarr + (1.0f - hwt) * Dgrbhvarl;

                      //compute fluctuations in up/down and left/right interpolations of colors
                      Dgrbvvaru = (dgintv[indx]) + (dgintv[indx - v1]) + (dgintv[indx - v2]);
                      Dgrbvvard = (dgintv[indx]) + (dgintv[indx + v1]) + (dgintv[indx + v2]);
                      Dgrbhvarl = (dginth[indx]) + (dginth[indx - 1]) + (dginth[indx - 2]);
                      Dgrbhvarr = (dginth[indx]) + (dginth[indx + 1]) + (dginth[indx + 2]);

                      float vcdvar1 = epssq + vwt * Dgrbvvard + (1.0f - vwt) * Dgrbvvaru;
                      float hcdvar1 = epssq + hwt * Dgrbhvarr + (1.0f - hwt) * Dgrbhvarl;

                      //determine adaptive weights for G interpolation
                      float varwt = hcdvar / (vcdvar + hcdvar);
                      float diffwt = hcdvar1 / (vcdvar1 + hcdvar1);

                      //if both agree on interpolation direction, choose the one with strongest directional discrimination;
                      //otherwise, choose the u/d and l/r difference fluctuation weights
                      if ((0.5f - varwt) * (0.5f - diffwt) > 0 && fabs(0.5f-diffwt) < fabs(0.5f-varwt))
                        {
                          hvwt[indx] = varwt;
                        }
                      else
                        {
                          hvwt[indx] = diffwt;
                        }

                      //hvwt[indx]=varwt;
                    }
                //t2_cdvar += clock() - t1_cdvar;

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // Nyquist test
                //t1_nyqtest = clock();

                for ( int rr = 6; rr < rr1 - 6 ; rr++ )
                  for ( int cc = 6 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 6 ; cc += 2, indx += 2 )
                    {

                      //nyquist texture test: ask if difference of vcd compared to hcd is larger or smaller than RGGB gradients
                      float nyqtest = (gaussodd[0] * cddiffsq[indx]
                                       + gaussodd[1]* (cddiffsq[indx - m1] + cddiffsq[indx + p1] + cddiffsq[indx - p1] + cddiffsq[indx + m1])
                                       + gaussodd[2]* (cddiffsq[indx - v2] + cddiffsq[indx - 2] + cddiffsq[indx + 2] + cddiffsq[indx + v2])
                                       + gaussodd[3]* (cddiffsq[indx - m2] + cddiffsq[indx + p2] + cddiffsq[indx - p2] + cddiffsq[indx + m2]));

                      nyqtest -= nyqthresh
                                 * (gaussgrad[0] * (delhsq[indx] + delvsq[indx])
                                    + gaussgrad[1]
                                    * (delhsq[indx - v1] + delvsq[indx - v1] + delhsq[indx + 1]
                                       + delvsq[indx + 1] + delhsq[indx - 1] + delvsq[indx - 1]
                                       + delhsq[indx + v1] + delvsq[indx + v1])
                                    + gaussgrad[2]
                                    * (delhsq[indx - m1] + delvsq[indx - m1] + delhsq[indx + p1]
                                       + delvsq[indx + p1] + delhsq[indx - p1]
                                       + delvsq[indx - p1] + delhsq[indx + m1]
                                       + delvsq[indx + m1])
                                    + gaussgrad[3]
                                    * (delhsq[indx - v2] + delvsq[indx - v2] + delhsq[indx - 2]
                                       + delvsq[indx - 2] + delhsq[indx + 2] + delvsq[indx + 2]
                                       + delhsq[indx + v2] + delvsq[indx + v2])
                                    + gaussgrad[4]
                                    * (delhsq[indx - 2 * TS - 1] + delvsq[indx - 2 * TS - 1]
                                       + delhsq[indx - 2 * TS + 1] + delvsq[indx - 2 * TS + 1]
                                       + delhsq[indx - TS - 2] + delvsq[indx - TS - 2]
                                       + delhsq[indx - TS + 2] + delvsq[indx - TS + 2]
                                       + delhsq[indx + TS - 2] + delvsq[indx + TS - 2]
                                       + delhsq[indx + TS + 2] + delvsq[indx - TS + 2]
                                       + delhsq[indx + 2 * TS - 1] + delvsq[indx + 2 * TS - 1]
                                       + delhsq[indx + 2 * TS + 1] + delvsq[indx + 2 * TS + 1])
                                    + gaussgrad[5]
                                    * (delhsq[indx - m2] + delvsq[indx - m2] + delhsq[indx + p2]
                                       + delvsq[indx + p2] + delhsq[indx - p2]
                                       + delvsq[indx - p2] + delhsq[indx + m2]
                                       + delvsq[indx + m2]));

                      if (nyqtest > 0)
                        {
                          nyquist[indx] = 1;
                        } //nyquist=1 for nyquist region
                    }

                for ( int rr = 8; rr < rr1 - 8 ; rr++ )
                  for ( int cc = 8 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 8 ; cc += 2, indx += 2 )
                    {

                      int areawt = (nyquist[indx - v2] + nyquist[indx - m1] + nyquist[indx + p1]
                                    + nyquist[indx - 2] + nyquist[indx] + nyquist[indx + 2] + nyquist[indx - p1]
                                    + nyquist[indx + m1] + nyquist[indx + v2]);
                      //if most of your neighbors are named Nyquist, it's likely that you're one too
                      if (areawt > 4)
                        nyquist[indx] = 1;
                      //or not
                      if (areawt < 4)
                        nyquist[indx] = 0;
                    }

                //t2_nyqtest += clock() - t1_nyqtest;
                // end of Nyquist test
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // in areas of Nyquist texture, do area interpolation
                //t1_areainterp = clock();
                for ( int rr = 8; rr < rr1 - 8 ; rr++ )
                  for ( int cc = 8 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 8 ; cc += 2, indx += 2 )
                    {

                      if (nyquist[indx])
                        {
                          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                          // area interpolation
                          float sumh,sumv,sumsqh,sumsqv,areawt;
                          sumh = sumv = sumsqh = sumsqv = areawt = 0;
                          for ( int i = -6; i < 7 ; i += 2 )
                            for ( int j = -6; j < 7 ; j += 2 )
                              {
                                int indx1 = (rr + i) * TS + cc + j;
                                if (nyquist[indx1])
                                  {
                                    sumh += cfa[indx1] - 0.5f * (cfa[indx1 - 1] + cfa[indx1 + 1]);
                                    sumv += cfa[indx1] - 0.5f * (cfa[indx1 - v1] + cfa[indx1 + v1]);
                                    sumsqh += 0.5f * (SQR(cfa[indx1]-cfa[indx1-1]) + SQR(cfa[indx1]-cfa[indx1+1]));
                                    sumsqv += 0.5f * (SQR(cfa[indx1]-cfa[indx1-v1]) + SQR(cfa[indx1]-cfa[indx1+v1]));
                                    areawt += 1.0f;
                                  }
                              }

                          //horizontal and vertical color differences, and adaptive weight
                          float hcdvar = epssq + fabs(areawt*sumsqh-sumh*sumh);
                          float vcdvar = epssq + fabs(areawt*sumsqv-sumv*sumv);
                          hvwt[indx] = hcdvar / (vcdvar + hcdvar);

                          // end of area interpolation
                          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                        }
                    }
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                //t2_areainterp += clock() - t1_areainterp;

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                //populate G at R/B sites
                for ( int rr = 8; rr < rr1 - 8 ; rr++ )
                  for ( int cc = 8 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 8 ; cc += 2, indx += 2 )
                    {

                      //first ask if one gets more directional discrimination from nearby B/R sites
                      float hvwtalt = 0.25f * (hvwt[indx - m1] + hvwt[indx + p1] + hvwt[indx - p1] + hvwt[indx + m1]);
                      float vo = fabs(0.5f-hvwt[indx]);
                      float ve = fabs(0.5f-hvwtalt);
                      if (vo < ve)
                        {
                          hvwt[indx] = hvwtalt;
                        } //a better result was obtained from the neighbors

                      Dgrb[indx][0] = (hcd[indx] * (1.0f - hvwt[indx]) + vcd[indx] * hvwt[indx]); //evaluate color differences
                      //if (hvwt[indx]<0.5) Dgrb[indx][0]=hcd[indx];
                      //if (hvwt[indx]>0.5) Dgrb[indx][0]=vcd[indx];
                      rgb[indx][1] = cfa[indx] + Dgrb[indx][0]; //evaluate G (finally!)

                      //local curvature in G (preparation for nyquist refinement step)
                      if (nyquist[indx])
                        {
                          Dgrbh2[indx] = SQR(rgb[indx][1] - 0.5f*(rgb[indx-1][1]+rgb[indx+1][1]));
                          Dgrbv2[indx] = SQR(rgb[indx][1] - 0.5f*(rgb[indx-v1][1]+rgb[indx+v1][1]));
                        }
                      else
                        {
                          Dgrbh2[indx] = Dgrbv2[indx] = 0;
                        }
                    }

                //end of standard interpolation
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // refine Nyquist areas using G curvatures

                for ( int rr = 8; rr < rr1 - 8 ; rr++ )
                  for ( int cc = 8 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 8 ; cc += 2, indx += 2 )
                    {

                      if (nyquist[indx])
                        {
                          //local averages (over Nyquist pixels only) of G curvature squared
                          float gvarh = epssq + (gquinc[0] * Dgrbh2[indx] + gquinc[1]
                                           * (Dgrbh2[indx - m1] + Dgrbh2[indx + p1] + Dgrbh2[indx - p1] + Dgrbh2[indx + m1])
                                           + gquinc[2] * (Dgrbh2[indx - v2] + Dgrbh2[indx - 2] + Dgrbh2[indx + 2] + Dgrbh2[indx + v2])
                                           + gquinc[3] * (Dgrbh2[indx - m2] + Dgrbh2[indx + p2] + Dgrbh2[indx - p2] + Dgrbh2[indx + m2]));
                          float gvarv = epssq + (gquinc[0] * Dgrbv2[indx]
                                           + gquinc[1] * (Dgrbv2[indx - m1] + Dgrbv2[indx + p1] + Dgrbv2[indx - p1] + Dgrbv2[indx + m1])
                                           + gquinc[2] * (Dgrbv2[indx - v2] + Dgrbv2[indx - 2] + Dgrbv2[indx + 2] + Dgrbv2[indx + v2])
                                           + gquinc[3] * (Dgrbv2[indx - m2] + Dgrbv2[indx + p2] + Dgrbv2[indx - p2] + Dgrbv2[indx + m2]));
                          //use the results as weights for refined G interpolation
                          Dgrb[indx][0] = (hcd[indx] * gvarv + vcd[indx] * gvarh) / (gvarv + gvarh);
                          rgb[indx][1] = cfa[indx] + Dgrb[indx][0];
                        }
                    }

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                //t1_diag = clock();
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // diagonal interpolation correction

                for ( int rr = 8; rr < rr1 - 8 ; rr++ )
                  for ( int cc = 8 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 8 ; cc += 2, indx += 2 )
                    {

                      float rbvarp = epssq + (gausseven[0] * (Dgrbpsq1[indx - v1] + Dgrbpsq1[indx - 1] + Dgrbpsq1[indx + 1] + Dgrbpsq1[indx + v1])
                                     + gausseven[1] * (Dgrbpsq1[indx - v2 - 1] + Dgrbpsq1[indx - v2 + 1] + Dgrbpsq1[indx - 2 - v1] + Dgrbpsq1[indx + 2 - v1]
                                     + Dgrbpsq1[indx - 2 + v1] + Dgrbpsq1[indx + 2 + v1] + Dgrbpsq1[indx + v2 - 1] + Dgrbpsq1[indx + v2 + 1]));
                      float rbvarm = epssq + (gausseven[0] * (Dgrbmsq1[indx - v1] + Dgrbmsq1[indx - 1] + Dgrbmsq1[indx + 1] + Dgrbmsq1[indx + v1])
                                     + gausseven[1] * (Dgrbmsq1[indx - v2 - 1] + Dgrbmsq1[indx - v2 + 1] + Dgrbmsq1[indx - 2 - v1] + Dgrbmsq1[indx + 2 - v1]
                                     + Dgrbmsq1[indx - 2 + v1] + Dgrbmsq1[indx + 2 + v1] + Dgrbmsq1[indx + v2 - 1] + Dgrbmsq1[indx + v2 + 1]));

                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                      //diagonal color ratios
                      float crse = 2.0f * (cfa[indx + m1]) / (eps + cfa[indx] + (cfa[indx + m2]));
                      float crnw = 2.0f * (cfa[indx - m1]) / (eps + cfa[indx] + (cfa[indx - m2]));
                      float crne = 2.0f * (cfa[indx + p1]) / (eps + cfa[indx] + (cfa[indx + p2]));
                      float crsw = 2.0f * (cfa[indx - p1]) / (eps + cfa[indx] + (cfa[indx - p2]));

                      //assign B/R at R/B sites
                      float rbse = (fabs(1.0f-crse) < arthresh)?cfa[indx] * crse : (cfa[indx + m1]) + 0.5f * (cfa[indx] - cfa[indx + m2]);
                      float rbnw = (fabs(1.0f-crnw) < arthresh)?cfa[indx] * crnw : (cfa[indx - m1]) + 0.5f * (cfa[indx] - cfa[indx - m2]);
                      float rbne = (fabs(1.0f-crne) < arthresh)?cfa[indx] * crne : (cfa[indx + p1]) + 0.5f * (cfa[indx] - cfa[indx + p2]);
                      float rbsw = (fabs(1.0f-crsw) < arthresh)?cfa[indx] * crsw : (cfa[indx - p1]) + 0.5f * (cfa[indx] - cfa[indx - p2]);


                      float wtse = eps + delm[indx] + delm[indx + m1] + delm[indx + m2]; //same as for wtu,wtd,wtl,wtr
                      float wtnw = eps + delm[indx] + delm[indx - m1] + delm[indx - m2];
                      float wtne = eps + delp[indx] + delp[indx + p1] + delp[indx + p2];
                      float wtsw = eps + delp[indx] + delp[indx - p1] + delp[indx - p2];

                      rbm[indx] = (wtse * rbnw + wtnw * rbse) / (wtse + wtnw);
                      rbp[indx] = (wtne * rbsw + wtsw * rbne) / (wtne + wtsw);

                      pmwt[indx] = rbvarm / (rbvarp + rbvarm);

                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                      //bound the interpolation in regions of high saturation
                      if (rbp[indx] < cfa[indx])
                        {
                          if (2.0f * rbp[indx] < cfa[indx])
                            {
                              rbp[indx] = ULIM(rbp[indx] ,cfa[indx-p1],cfa[indx+p1]);
                            }
                          else
                            {
                              float pwt = 2.0f * (cfa[indx] - rbp[indx]) / (eps + rbp[indx] + cfa[indx]);
                              rbp[indx] = pwt * rbp[indx] + (1.0f - pwt) * ULIM(rbp[indx],cfa[indx-p1],cfa[indx+p1]);
                            }
                        }
                      if (rbm[indx] < cfa[indx])
                        {
                          if (2.0 * rbm[indx] < cfa[indx])
                            {
                              rbm[indx] = ULIM(rbm[indx] ,cfa[indx-m1],cfa[indx+m1]);
                            }
                          else
                            {
                              float mwt = 2.0f * (cfa[indx] - rbm[indx]) / (eps + rbm[indx] + cfa[indx]);
                              rbm[indx] = mwt * rbm[indx] + (1.0f - mwt) * ULIM(rbm[indx],cfa[indx-m1],cfa[indx+m1]);
                            }
                        }

                      if (rbp[indx] > clip_pt)
                        rbp[indx] = ULIM(rbp[indx],cfa[indx-p1],cfa[indx+p1]); //for RT implementation
                      if (rbm[indx] > clip_pt)
                        rbm[indx] = ULIM(rbm[indx],cfa[indx-m1],cfa[indx+m1]);
                      //c=2-Myfc(rr,cc);//for dcraw implementation
                      //if (rbp[indx] > pre_mul[c]) rbp[indx]=ULIM(rbp[indx],cfa[indx-p1],cfa[indx+p1]);
                      //if (rbm[indx] > pre_mul[c]) rbm[indx]=ULIM(rbm[indx],cfa[indx-m1],cfa[indx+m1]);
                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                      //rbint[indx] = 0.5*(cfa[indx] + (rbp*rbvarm+rbm*rbvarp)/(rbvarp+rbvarm));//this is R+B, interpolated
                    }

                for ( int rr = 10; rr < rr1 - 10 ; rr++ )
                  for ( int cc = 10 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 10 ;
                        cc += 2, indx += 2 )
                    {

                      //first ask if one gets more directional discrimination from nearby B/R sites
                      float pmwtalt = 0.25f * (pmwt[indx - m1] + pmwt[indx + p1] + pmwt[indx - p1] + pmwt[indx + m1]);
                      float vo = fabs(0.5f-pmwt[indx]);
                      float ve = fabs(0.5f-pmwtalt);
                      if (vo < ve)
                        {
                          pmwt[indx] = pmwtalt;
                        } //a better result was obtained from the neighbors
                      rbint[indx] = 0.5f * (cfa[indx] + rbm[indx] * (1.0f - pmwt[indx]) + rbp[indx] * pmwt[indx]); //this is R+B, interpolated
                    }

                for ( int rr = 12; rr < rr1 - 12 ; rr++ )
                  for ( int cc = 12 + (Myfc(rr,2) & 1), indx = rr * TS + cc; cc < cc1 - 12 ; cc += 2, indx += 2 )
                    {
                      if (fabs(0.5f-pmwt[indx]) < fabs(0.5f-hvwt[indx]))
                        continue;

                      //now interpolate G vertically/horizontally using R+B values
                      //unfortunately, since G interpolation cannot be done diagonally this may lead to color shifts
                      //color ratios for G interpolation

                      float cru = cfa[indx - v1] * 2.0f / (eps + rbint[indx] + rbint[indx - v2]);
                      float crd = cfa[indx + v1] * 2.0f / (eps + rbint[indx] + rbint[indx + v2]);
                      float crl = cfa[indx - 1] * 2.0f / (eps + rbint[indx] + rbint[indx - 2]);
                      float crr = cfa[indx + 1] * 2.0f / (eps + rbint[indx] + rbint[indx + 2]);

                      //interpolated G via adaptive ratios or Hamilton-Adams in each cardinal direction
                      float gu = (fabs(1.0f-cru) < arthresh) ? rbint[indx] * cru : cfa[indx - v1] + 0.5f * (rbint[indx] - rbint[indx - v2]);
                      float gd = (fabs(1.0f-crd) < arthresh) ? rbint[indx] * crd : cfa[indx + v1] + 0.5f * (rbint[indx] - rbint[indx + v2]);
                      float gl = (fabs(1.0f-crl) < arthresh) ? rbint[indx] * crl : cfa[indx - 1] + 0.5f * (rbint[indx] - rbint[indx - 2]);
                      float gr = (fabs(1.0f-crr) < arthresh) ? rbint[indx] * crr : cfa[indx + 1] + 0.5f * (rbint[indx] - rbint[indx + 2]);

                      //gu=rbint[indx]*cru;
                      //gd=rbint[indx]*crd;
                      //gl=rbint[indx]*crl;
                      //gr=rbint[indx]*crr;

                      //interpolated G via adaptive weights of cardinal evaluations
                      float Gintv = (dirwts[indx - v1][0] * gd + dirwts[indx + v1][0] * gu)
                                    / (dirwts[indx + v1][0] + dirwts[indx - v1][0]);
                      float Ginth = (dirwts[indx - 1][1] * gr + dirwts[indx + 1][1] * gl)
                                    / (dirwts[indx - 1][1] + dirwts[indx + 1][1]);

                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                      //bound the interpolation in regions of high saturation
                      if (Gintv < rbint[indx])
                        {
                          if (2.0f * Gintv < rbint[indx])
                            {
                              Gintv = ULIM(Gintv ,cfa[indx-v1],cfa[indx+v1]);
                            }
                          else
                            {
                              float vwt = 2.0f * (rbint[indx] - Gintv) / (eps + Gintv + rbint[indx]);
                              Gintv = vwt * Gintv + (1.0f - vwt) * ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]);
                            }
                        }
                      if (Ginth < rbint[indx])
                        {
                          if (2.0f * Ginth < rbint[indx])
                            {
                              Ginth = ULIM(Ginth ,cfa[indx-1],cfa[indx+1]);
                            }
                          else
                            {
                              float hwt = 2.0f * (rbint[indx] - Ginth) / (eps + Ginth + rbint[indx]);
                              Ginth = hwt * Ginth + (1.0f - hwt) * ULIM(Ginth,cfa[indx-1],cfa[indx+1]);
                            }
                        }

                      if (Ginth > clip_pt)
                        Ginth = ULIM(Ginth,cfa[indx-1],cfa[indx+1]); //for RT implementation
                      if (Gintv > clip_pt)
                        Gintv = ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]);
                      //c=Myfc(rr,cc);//for dcraw implementation
                      //if (Ginth > pre_mul[c]) Ginth=ULIM(Ginth,cfa[indx-1],cfa[indx+1]);
                      //if (Gintv > pre_mul[c]) Gintv=ULIM(Gintv,cfa[indx-v1],cfa[indx+v1]);
                      // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                      rgb[indx][1] = Ginth * (1.0f - hvwt[indx]) + Gintv * hvwt[indx];
                      //rgb[indx][1] = 0.5*(rgb[indx][1]+0.25*(rgb[indx-v1][1]+rgb[indx+v1][1]+rgb[indx-1][1]+rgb[indx+1][1]));
                      Dgrb[indx][0] = rgb[indx][1] - cfa[indx];

                      //rgb[indx][2-Myfc(rr,cc)]=2*rbint[indx]-cfa[indx];
                    }
                //end of diagonal interpolation correction
                //t2_diag += clock() - t1_diag;
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                //t1_chroma = clock();
                //fancy chrominance interpolation
                //(ey,ex) is location of R site
                for ( int rr = 13 - ey; rr < rr1 - 12 ; rr += 2 )
                  for ( int cc = 13 - ex, indx = rr * TS + cc; cc < cc1 - 12 ; cc += 2, indx += 2 )   //B coset
                    {
                      Dgrb[indx][1] = Dgrb[indx][0]; //split out G-B from G-R
                      Dgrb[indx][0] = 0;
                    }
                for ( int rr = 12; rr < rr1 - 12 ; rr++ )
                  for ( int cc = 12 + (Myfc(rr,2) & 1), indx = rr * TS + cc, c = 1 - Myfc(rr,cc) / 2; cc < cc1 - 12 ; cc += 2, indx += 2 )
                    {
                      float wtnw = 1.0f
                                   / (eps + fabs(Dgrb[indx-m1][c]-Dgrb[indx+m1][c])
                                      + fabs(Dgrb[indx-m1][c]-Dgrb[indx-m3][c])
                                      + fabs(Dgrb[indx+m1][c]-Dgrb[indx-m3][c]));
                      float wtne = 1.0f
                                   / (eps + fabs(Dgrb[indx+p1][c]-Dgrb[indx-p1][c])
                                      + fabs(Dgrb[indx+p1][c]-Dgrb[indx+p3][c])
                                      + fabs(Dgrb[indx-p1][c]-Dgrb[indx+p3][c]));
                      float wtsw = 1.0f
                                   / (eps + fabs(Dgrb[indx-p1][c]-Dgrb[indx+p1][c])
                                      + fabs(Dgrb[indx-p1][c]-Dgrb[indx+m3][c])
                                      + fabs(Dgrb[indx+p1][c]-Dgrb[indx-p3][c]));
                      float wtse = 1.0f
                                   / (eps + fabs(Dgrb[indx+m1][c]-Dgrb[indx-m1][c])
                                      + fabs(Dgrb[indx+m1][c]-Dgrb[indx-p3][c])
                                      + fabs(Dgrb[indx-m1][c]-Dgrb[indx+m3][c]));

                      //Dgrb[indx][c]=(wtnw*Dgrb[indx-m1][c]+wtne*Dgrb[indx+p1][c]+wtsw*Dgrb[indx-p1][c]+wtse*Dgrb[indx+m1][c])/(wtnw+wtne+wtsw+wtse);

                      Dgrb[indx][c] = (wtnw
                                       * (1.325f * Dgrb[indx - m1][c] - 0.175f * Dgrb[indx - m3][c]
                                          - 0.075f * Dgrb[indx - m1 - 2][c] - 0.075f * Dgrb[indx - m1 - v2][c])
                                       + wtne
                                       * (1.325f * Dgrb[indx + p1][c] - 0.175f * Dgrb[indx + p3][c]
                                          - 0.075f * Dgrb[indx + p1 + 2][c]
                                          - 0.075f * Dgrb[indx + p1 + v2][c])
                                       + wtsw
                                       * (1.325f * Dgrb[indx - p1][c] - 0.175f * Dgrb[indx - p3][c]
                                          - 0.075f * Dgrb[indx - p1 - 2][c]
                                          - 0.075f * Dgrb[indx - p1 - v2][c])
                                       + wtse
                                       * (1.325f * Dgrb[indx + m1][c] - 0.175f * Dgrb[indx + m3][c]
                                          - 0.075f * Dgrb[indx + m1 + 2][c]
                                          - 0.075f * Dgrb[indx + m1 + v2][c]))
                                      / (wtnw + wtne + wtsw + wtse);
                    }
                for ( int rr = 12; rr < rr1 - 12 ; rr++ )
                  for ( int cc = 12 + (Myfc(rr,1) & 1), indx = rr * TS + cc;//, c = Myfc(rr,cc+1) / 2
                        cc < cc1 - 12 ; cc += 2, indx += 2 )
                    for ( int c = 0; c < 2 ; c++ )
                      {

                        Dgrb[indx][c] = ((hvwt[indx - v1]) * Dgrb[indx - v1][c]
                                         + (1.0f - hvwt[indx + 1]) * Dgrb[indx + 1][c]
                                         + (1.0f - hvwt[indx - 1]) * Dgrb[indx - 1][c]
                                         + (hvwt[indx + v1]) * Dgrb[indx + v1][c])
                                        / ((hvwt[indx - v1]) + (1.0f - hvwt[indx + 1]) + (1.0f - hvwt[indx - 1])
                                           + (hvwt[indx + v1]));

                      }
                for ( int rr = 12; rr < rr1 - 12 ; rr++ )
                  for ( int cc = 12, indx = rr * TS + cc; cc < cc1 - 12 ; cc++, indx++ )
                    {
                      rgb[indx][0] = (rgb[indx][1] - Dgrb[indx][0]);
                      rgb[indx][2] = (rgb[indx][1] - Dgrb[indx][1]);
                    }
                //t2_chroma += clock() - t1_chroma;

                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
                // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

                // copy smoothed results back to image matrix
                int mrow = H - 1;
                int mcol = W - 1;
                for ( int rr = 16; rr < rr1 - 16 ; rr++ )
                  for ( unsigned int row = rr + top, cc = 16; cc < cc1 - 16 ; cc++ )
                    {
                      unsigned int col = cc + left;

                      unsigned int indx = rr * TS + cc;
                      float r = rgb[indx][0];// * post_scale; //- boffset;
                      float g = rgb[indx][1];// * post_scale; // - boffset;
                      float b = rgb[indx][2];// * post_scale; // - boffset;
                      /*
                      								r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f:r);
                      								g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f:g);
                      								b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f:b);

                      								float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b;
                      								float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b;
                      								float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b;
                      								*/
                      switch (rot)
                        {
                        case 0:
                          RGB_converted[row][col].r = r;// nr > 0.0f ? (nr>1.0f?1.0f:nr) : 0.0f;
                          RGB_converted[row][col].g = g;//ng > 0.0f ? (ng>1.0f?1.0f:ng) : 0.0f;
                          RGB_converted[row][col].b = b;//nb > 0.0f ? (nb>1.0f?1.0f:nb) : 0.0f;
                          break;
                        case 1:
                          RGB_converted[col][mrow - row].r = r;// nr > 0.0f ? (nr>1.0f?1.0f:nr) : 0.0f;
                          RGB_converted[col][mrow - row].g = g;//ng > 0.0f ? (ng>1.0f?1.0f:ng) : 0.0f;
                          RGB_converted[col][mrow - row].b = b;//nb > 0.0f ? (nb>1.0f?1.0f:nb) : 0.0f;
                          break;
                        case 2:
                          RGB_converted[mrow - row][mcol - col].r = r;// nr > 0.0f ? (nr>1.0f?1.0f:nr) : 0.0f;
                          RGB_converted[mrow - row][mcol - col].g = g;//ng > 0.0f ? (ng>1.0f?1.0f:ng) : 0.0f;
                          RGB_converted[mrow - row][mcol - col].b = b;//nb > 0.0f ? (nb>1.0f?1.0f:nb) : 0.0f;
                          break;

                        case 3:
                          RGB_converted[mcol - col][row].r = r; // nr > 0.0f ? (nr>1.0f?1.0f:nr) : 0.0f;
                          RGB_converted[mcol - col][row].g = g; //ng > 0.0f ? (ng>1.0f?1.0f:ng) : 0.0f;
                          RGB_converted[mcol - col][row].b = b; //nb > 0.0f ? (nb>1.0f?1.0f:nb) : 0.0f;
                          break;
                        }

                      //for dcraw implementation
                      //for (c=0; c<3; c++){
                      //	image[indx][c] = CLIP((int)(65535.0f*rgb[rr*TS+cc][c] + 0.5f));
                      //}

                    }
                //end of main loop

                // clean up
                //free(buffer);
                /*
                 progress+=((double)(TS-32)*(TS-32))/(double)(height*width);
                 if (progress>1.0)
                 {
                 progress=1.0;
                 }
                 if(plistener) plistener->setProgress(progress);*/
                //Tile_flags[(top+16)/TILE_SIZE][(left+16)/TILE_SIZE]=2; // tell this tile is transformed.
              }

        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

        // clean up
        delete[] buffer;
      }
    }
  RGB_converted.moveto(-dest.xoff(), -dest.yoff());
  dest <<= RGB_converted;
//#pragma omp parallel
//	color_correct(dest,dest,props);
  // done

#undef TS
#undef fabs

}
