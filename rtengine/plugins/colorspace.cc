/*
 * Lab_adjustments.cc
 *
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
#include "../pluginhandler/plugin.h"
#include <iostream>
#include <cmath>
#define  Pr  0.299f
#define  Pg  0.587f
#define  Pb  0.114f

inline float Contrast(float i)
{
  if (i < 0.5f) return i * i * 2.0f;
  i = (1.0f - i);
  return 1.0f - i * i * 2.0f;
}

static inline float hlcurve (const float exp_scale, const float comp, float val)
        {
                if (comp>0.0f) {
                        //float val = level+(hlrange-65536.0);
                        float Y = val;//hlrange;
                        float R = 1.0f/(val*comp);
                        return log(1.0f+Y*comp)*R;
                } else {
                        return 1.0f;
                }
        }
inline void highlightcomp(const float post_scale, const float comp,float & r,float & g, float&b)
{
  float factor = sqrt(r*r*Pr+g*g*Pg+b*b*Pb)*post_scale;
      factor = hlcurve(post_scale, comp, factor)*post_scale;

        r=r*factor;//+bright;
        g=g*factor;//+bright;
        b=b*factor;//+bright;
}
inline float invcontrast(float i)
{
  if ( i < 0.5f) return (i>0.0f)? sqrt(i)*0.707106781f :0.0f;
  if (i<1.0f) return 1.0f - sqrt(1.0f -i)*0.707106781f;
  return 1.0f;
}

inline void saturation(float &R, float&G, float&B, float change)
{

  float  P=sqrt(R*R*Pr+G*G*Pg+B*B*Pb) ;
  //float  P=sqrt(R*R+G*G+B*B) ;
  R=P+(R-P)*change;
  G=P+(G-P)*change;
  B=P+(B-P)*change;
}

inline void ClipFull(float &r,float &g,float &b)
{
  r = (r > 1.0f) ? 1.0f : ((r < 0.0f) ? 0.0f:r);
  g = (g > 1.0f) ? 1.0f : ((g < 0.0f) ? 0.0f:g);
  b = (b > 1.0f) ? 1.0f : ((b < 0.0f) ? 0.0f:b);
}

inline void Contrast2(float & r,float &g , float & b,float contrast,int inv,float not_contrast)
{
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0f)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0f)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
}
inline void Contrast(float & r,float &g , float & b,float contrast,int inv,float not_contrast)
{
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            r= not_contrast*r + contrast * Contrast(r) ;
            g= not_contrast*g + contrast * Contrast(g) ;
            b= not_contrast*b + contrast * Contrast(b) ;
            r= not_contrast*r + contrast * Contrast(r) ;
            g= not_contrast*g + contrast * Contrast(g) ;
            b= not_contrast*b + contrast * Contrast(b) ;
          }
        if ((contrast >0.0f) && inv)
          {
            r= not_contrast*r + contrast * invcontrast(r) ;
            g= not_contrast*g + contrast * invcontrast(g) ;
            b= not_contrast*b + contrast * invcontrast(b) ;
            r= not_contrast*r + contrast * invcontrast(r) ;
            g= not_contrast*g + contrast * invcontrast(g) ;
            b= not_contrast*b + contrast * invcontrast(b) ;
          }
}
void color_correct(HDRImage & dest,improps &props)
{
  float expcomp = props.pp3["Exposure"]["Compensation"];
  float bright = props.pp3["Exposure"]["Brightness"];
  float contrast = props.pp3["Exposure"]["Contrast"];
  float black = props.pp3["Exposure"]["Black"];
  float comp = props.pp3["Exposure"]["HighlightCompr"];
  float sat = props.pp3["Exposure"]["Saturation"];
  sat =1.0f + sat*0.01f;

  comp = comp*0.01f;
  black =  (black+props.black) /65536.0f;

  bright = bright*0.001f;
  int inv=(contrast<0);
  contrast=fabs(contrast*0.01f);
  const float not_contrast = 1.0f - contrast;
  float end_scale=0.0f;
  if (expcomp<0.0)
      {
        // color artefacts will happen due to clipped colors.
        // without proper highlight conversion

        end_scale=pow(2.0f, expcomp);
        expcomp=0.0f;
      }
  const float post_scale = pow(2.0f, expcomp);

  float mat[3][3];
  for (int i=0; i<3; i++)
    for (int j=0; j<3; j++)
      mat[i][j]=props.mat[i][j];

  int Hgt=dest.ysize(),Wdt=dest.xsize();
  cout << "Doing color correction " << expcomp <<endl;
  float tblack = 0.0f;
#pragma omp parallel for
  for (  int y = 0 ; y < Hgt ; y++ )
    for (  int x = 0 ; x < Wdt ; x++ )
      {
        // adjust levels
        float r = (dest[y][x].r-tblack);//-black+bright)/(1.0f-black);//*post_scale ;
        float g = (dest[y][x].g-tblack);//-black+bright)/(1.0f-black);//*post_scale ;
        float b = (dest[y][x].b-tblack);//-black+bright)/(1.0f-black);//*post_scale ;

        highlightcomp(post_scale, comp,r,g,b);

#if 0

        // clip bottom
        r=(r<0.0f)?0.0f:r;
        g=(g<0.0f)?0.0f:g;
        b=(b<0.0f)?0.0f:b;

        // highlight correction, treating all colors the same
        // providing color correct conversion.
        float factor = sqrt(r*r+g*g+b*b);
        factor = hlcurve(post_scale, comp, factor);
        r=r*factor;
        g=g*factor;
        b=b*factor;
#endif
      // clipping to range [0,1]
        ClipFull(r,g,b);

#if 0
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        if ((contrast >0.0f) && (inv==0))
          {
            r= not_contrast*r + contrast * Contrast(r) ;
            g= not_contrast*g + contrast * Contrast(g) ;
            b= not_contrast*b + contrast * Contrast(b) ;
          }
        if ((contrast >0.0f) && inv)
          {
            r= not_contrast*r + contrast * invcontrast(r) ;
            g= not_contrast*g + contrast * invcontrast(g) ;
            b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        if ((contrast >0.0f) && (inv==0))
          {
            r= not_contrast*r + contrast * Contrast(r) ;
            g= not_contrast*g + contrast * Contrast(g) ;
            b= not_contrast*b + contrast * Contrast(b) ;
          }
        if ((contrast >0.0f) && inv)
          {
            r= not_contrast*r + contrast * invcontrast(r) ;
            g= not_contrast*g + contrast * invcontrast(g) ;
            b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
        float nr = mat[0][0] * r + mat[0][1] * g + mat[0][2] * b -black;
        float ng = mat[1][0] * r + mat[1][1] * g + mat[1][2] * b -black;
        float nb = mat[2][0] * r + mat[2][1] * g + mat[2][2] * b -black;
        // highlight correction, treating all colors the same
        // providing color correct conversion.

        // clip bottom
        nr=(nr<0.0f)?0.0f:nr;
        ng=(ng<0.0f)?0.0f:ng;
        nb=(nb<0.0f)?0.0f:nb;


#if 0
        // highlight
        //float factor = sqrt(nr*nr+ng*ng+nb*nb)*post_scale;
        float factor = sqrt(nr*nr*Pr+ng*ng*Pg+nb*nb*Pb)*post_scale;
        factor = hlcurve(post_scale, comp, factor)*post_scale;
        nr=nr*factor;//+bright;
        ng=ng*factor;//+bright;
        nb=nb*factor;//+bright;
#endif
        //
        if (end_scale!=0.0)
          {
            nr=nr*end_scale;
            ng=ng*end_scale;
            nb=nb*end_scale;
          }
        saturation(nr,ng,nb,sat);

        ClipFull(nr,ng,nb);
        Contrast(nr,ng,nb,contrast,inv,not_contrast);
#if 0
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = nr*Pr+ng*Pg+nb*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = r*Pr+g*Pg+b*Pb;//sqrt(r*r*Pr+g*g*Pg+b*b*Pb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            r= not_contrast*r + contrast * l*r ;
            g= not_contrast*g + contrast * l*g ;
            b= not_contrast*b + contrast * l*b ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        if ((contrast >0.0f) && (inv==0))
          {
            nr= not_contrast*nr + contrast * Contrast(nr) ;
            ng= not_contrast*ng + contrast * Contrast(ng) ;
            nb= not_contrast*nb + contrast * Contrast(nb) ;
          }
        if ((contrast >0.0f) && inv)
          {
            nr= not_contrast*nr + contrast * invcontrast(nr) ;
            ng= not_contrast*ng + contrast * invcontrast(ng) ;
            nb= not_contrast*nb + contrast * invcontrast(nb) ;
          }
#endif

#if 0
        // apply contrast
        if ((contrast >0.0f) && (inv==0))
          {
            nr= not_contrast*nr + contrast * Contrast(nr) ;
            ng= not_contrast*ng + contrast * Contrast(ng) ;
            nb= not_contrast*nb + contrast * Contrast(nb) ;
          }
        if ((contrast >0.0f) && inv)
          {
            nr= not_contrast*nr + contrast * invcontrast(nr) ;
            ng= not_contrast*ng + contrast * invcontrast(ng) ;
            nb= not_contrast*nb + contrast * invcontrast(nb) ;
          }
#endif
#if 0
        // clipping to range [0,1]
                nr = (nr > 1.0f) ? 1.0f : ((nr < 0.0f) ? 0.0f:nr);
                ng = (ng > 1.0f) ? 1.0f : ((ng < 0.0f) ? 0.0f:ng);
                nb = (nb > 1.0f) ? 1.0f : ((nb < 0.0f) ? 0.0f:nb);
#endif
#if 0
        // apply contrast
        // range will remain [0,1]
        if ((contrast >0.0f) && (inv==0))
          {
            float l = sqrt(nr*nr*Pr+ng*ng*Pg+nb*nb*Pb);//sqrt(nr*nr+ng*ng+nb*nb);
            l=(l>0.0)? Contrast(l)/l:1.0f;
            nr= not_contrast*nr + contrast * l*nr ;
            ng= not_contrast*ng + contrast * l*ng ;
            nb= not_contrast*nb + contrast * l*nb ;
          }
        if ((contrast >0.0f) && inv)
          {
            float l = sqrt(nr*nr*Pr+ng*ng*Pg+nb*nb*Pb);//sqrt(nr*nr+ng*ng+nb*nb);
            l=(l>0.0)? invcontrast(l)/l:1.0f;
            nr= not_contrast*nr + contrast * l*nr ;
            ng= not_contrast*ng + contrast * l*ng ;
            nb= not_contrast*nb + contrast * l*nb ;

            //r= not_contrast*r + contrast * invcontrast(r) ;
            //g= not_contrast*g + contrast * invcontrast(g) ;
            //b= not_contrast*b + contrast * invcontrast(b) ;
          }
#endif
#if 0
        // apply contrast
        if ((contrast >0.0f) && (inv==0))
          {
            nr= not_contrast*nr + contrast * Contrast(nr) ;
            ng= not_contrast*ng + contrast * Contrast(ng) ;
            nb= not_contrast*nb + contrast * Contrast(nb) ;
          }
        if ((contrast >0.0f) && inv)
          {
            nr= not_contrast*nr + contrast * invcontrast(nr) ;
            ng= not_contrast*ng + contrast * invcontrast(ng) ;
            nb= not_contrast*nb + contrast * invcontrast(nb) ;
          }
#endif
#if 0
        nr=nr*hlcurve(post_scale, comp, nr);
        ng=ng*hlcurve(post_scale, comp, ng);
        nb=nb*hlcurve(post_scale, comp, nb);
#endif

        dest[y][x].r = nr;
        dest[y][x].g = ng;
        dest[y][x].b = nb;
      }
}
static int enabled(improps & props)
{
  return 1; // always enabled due to color matrix conversion
}
ADD_FILTER( color_correct, HDRim , 2)
