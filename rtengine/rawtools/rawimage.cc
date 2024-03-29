/*
 *  This file is part of RawTherapee.
 *
 *  Created on: 20/nov/2010
 */

#include "rawimage.h"

#ifdef WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

RawImage::RawImage(  char * new_name )
  :allocation(NULL)
  ,data(NULL)
  ,profile_data(NULL)
  ,filename(new_name)
  ,prefilters(0)
{
}

RawImage::~RawImage()
{
  printf(" RawImage delete called\n");
  if(ifp)
    fclose(ifp);
  if( image )
    delete [] image;
  if(allocation)
    {
      delete [] allocation;
      allocation=NULL;
    }
  if(data)
    {
      delete [] data;
      data=NULL;
    }
  if(profile_data)
    {
      delete [] profile_data;
      profile_data=NULL;
    }
}

/* Similar to dcraw scale_colors for coeff. calculation, but without actual pixels scaling.
 * need pixels in data[][] available
 */
int RawImage::get_colorsCoeff( float *pre_mul_, float *scale_mul_, int *cblack_  )
{
  unsigned  row, col, ur, uc, i, x, y, c, sum[8];
  unsigned  W = this->get_width();
  unsigned  H = this->get_height();
  int val, dark, sat;
  float dsum[8], dmin, dmax;

  for (int c = 0; c < 4; c++)
    {
      cblack_[c] = this->get_cblack(c) + this->get_black();
      pre_mul_[c] = this->get_pre_mul(c);
    }
//TODO: reduction by using dsum0 .. dsum7 will improve parallelism
  if ( this->get_cam_mul(0) == -1 )
    {
      memset(dsum, 0, sizeof dsum);
      for (row = 0; row < H; row += 8)
        for (col = 0; col < W ; col += 8)
          {
            memset(sum, 0, sizeof sum);
            for (y = row; y < row + 8 && y < H; y++)
              for (x = col; x < col + 8 && x < W; x++)
                for (int c = 0; c < 3; c++)
                  {
                    if (this->isBayer())
                      {
                        c = FC(y, x);
                        val = data[y][x];
                      }
                    else
                      val = data[y][3*x+c];
                    if (val > this->get_white() - 25)
                      goto skip_block;
                    if ((val -= cblack_[c]) < 0)
                      val = 0;
                    sum[c] += val;
                    sum[c + 4]++;
                    if ( this->isBayer())
                      break;
                  }
            for (c = 0; c < 8; c++)
              dsum[c] += sum[c];
skip_block:
            ;
          }
      for (int c = 0; c < 4; c++)
        if (dsum[c])
          pre_mul_[c] = dsum[c + 4] / dsum[c];
    }
  else
    {
      memset(sum, 0, sizeof sum);
      for (row = 0; row < 8; row++)
        for (col = 0; col < 8; col++)
          {
            int c = FC(row, col);
            if ((val = white[row][col] - cblack_[c]) > 0)
              sum[c] += val;
            sum[c + 4]++;
          }
      if (sum[0] && sum[1] && sum[2] && sum[3])
        for (int c = 0; c < 4; c++)
          pre_mul_[c] = (float) sum[c + 4] / sum[c];
      else if (this->get_cam_mul(0) && this->get_cam_mul(2))
        {
          pre_mul_[0] = this->get_cam_mul(0);
          pre_mul_[1] = this->get_cam_mul(1);
          pre_mul_[2] = this->get_cam_mul(2);
          pre_mul_[3] = this->get_cam_mul(3);
        }
      else
        fprintf(stderr, "Cannot use camera white balance.\n");
    }
  if (pre_mul_[3] == 0)
    pre_mul_[3] = this->get_colors() < 4 ? pre_mul_[1] : 1;
  dark = this->get_black();
  sat = this->get_white();
  sat -= this->get_black();
  for (dmin = dmax = pre_mul_[0], c = 1; c < 4; c++)
    {
      if (dmin > pre_mul_[c])
        dmin = pre_mul_[c];
      if (dmax < pre_mul_[c])
        dmax = pre_mul_[c];
    }

  for (c = 0; c < 4; c++)
    scale_mul_[c] = (pre_mul_[c] /= dmax) * 65535.0 / sat;
  if (verbose)
    {
      fprintf(stderr,"Scaling with darkness %d, saturation %d, and\nmultipliers", dark, sat);
      for (c = 0; c < 4; c++)
        fprintf(stderr, " %f", pre_mul[c]);
      fputc('\n', stderr);
    }
}

int RawImage::loadRaw (bool loadData, bool closeFile)
{
  ifname = filename.c_str(); //safe_locale_from_utf8(filename).c_str();
  verbose =0;// settings.verbose;
  oprof = NULL;
  cout << " RawImage::loadRaw " << filename << endl;
  ifp = gfopen (ifname);
  if (!ifp)
    return 3;
  cout << " RawImage::loadRaw success opening " << filename << endl;
  thumb_length = 0;
  thumb_offset = 0;
  thumb_load_raw = 0;
  use_camera_wb = 0;
  highlight = 1;
  half_size = 0;

  //***************** Read ALL raw file info
  identify ();
  cout << " RawImage::loadRaw done identify() on " << filename << endl;
  if (!is_raw)
    {
      fclose(ifp);
      ifp=NULL;
      return 2;
    }

  if (flip==5)
    this->rotate_deg = 270;
  else if (flip==3)
    this->rotate_deg = 180;
  else if (flip==6)
    this->rotate_deg = 90;
  else
    this->rotate_deg = 0;

  if( loadData )
    {

      use_camera_wb = 1;
      shrink = 0;
      //if (verbose) printf ("Loading %s %s image from %s...\n", make, model, filename.c_str());
      cout << " RawImage::loadRaw loading file " << filename << endl;
      iheight = height;
      iwidth  = width;
      // dcraw needs this global variable to hold pixel data
      image =  newimage(height,width,meta_length);
      cout << " RawImage::loadRaw found simensions " << width << " x " << height<< "@" << image << endl;

      meta_data = (char *) (image + height*width);
      if(!image)
        return 200;

      if (setjmp (failure))
        {
          if (image)
            delete [] image;
          fclose (ifp);
          return 100;
        }

      // Load raw pixels data
      fseek (ifp, data_offset, SEEK_SET);
      cout << " RawImage::loadRaw run dcraw " << endl;
      (this->*load_raw)();
      cout << " RawImage::loadRaw done run dcraw " << endl;
      // Load embedded profile
      if (profile_length)
        {
          profile_data = new char[profile_length];
          fseek ( ifp, profile_offset, SEEK_SET);
          fread ( profile_data, 1, profile_length, ifp);
        }

      // Setting the black and cblack
      int i = cblack[3];
      for (int c=0; c <3; c++)
        if (i > cblack[c])
          i = cblack[c];
      for (int c=0; c < 4; c++)
        cblack[c] -= i;
      black += i;
      cout << " RawImage::loadRaw finished loading" << endl;
    }
  if( closeFile )
    {
      fclose(ifp);
      ifp=NULL;
    }
  return 0;
}

unsigned short** RawImage::compress_image()
{
  if( !image )
    return NULL;
  if (filters)
    {
      if (!allocation)
        {
          allocation = new unsigned short[height * width];
          if (data) delete [] data;
          data = new unsigned short*[height];
          for (int i = 0; i < height; i++)
            data[i] = allocation + i * width;
        }
    }
  else
    {
      if (!allocation)
        {
          allocation = new unsigned short[3 * height * width];
          if (data) delete [] data;
          data = new unsigned short*[height];
          for (int i = 0; i < height; i++)
            data[i] = allocation + 3 * i * width;
        }
    }

  // copy pixel raw data: the compressed format earns space
  if (filters != 0)
    {
      for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
          this->data[row][col] = image[row * width + col][FC(row, col)];
    }
  else
    {
      for (int row = 0; row < height; row++)
        for (int col = 0; col < width; col++)
          {
            this->data[row][3 * col + 0] = image[row * width + col][0];
            this->data[row][3 * col + 1] = image[row * width + col][1];
            this->data[row][3 * col + 2] = image[row * width + col][2];
          }
    }
  delete [] image; // we don't need this anymore
  image=NULL;
  return data;
}

bool
RawImage::is_supportedThumb() const
{
  return ( (thumb_width * thumb_height) > 0 &&
           ( write_thumb == &DCraw::jpeg_thumb ||
             write_thumb == &DCraw::ppm_thumb ||
             thumb_load_raw == &DCraw::kodak_thumb_load_raw ));
}

bool
RawImage::get_thumbSwap() const
{
  return ((order == 0x4949) == (ntohs(0x1234) == 0x1234)) ? true : false;
}

