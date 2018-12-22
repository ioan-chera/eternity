// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright (C) 2013 James Haley et al.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/
//
//--------------------------------------------------------------------------
//
// Flat ripple warping
//
// Takes a normal, flat texture and distorts it like the water distortion
// in Quake.
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "doomdef.h"
#include "doomstat.h"
#include "tables.h"

#include "r_defs.h"
#include "r_data.h"
#include "r_draw.h"
#include "w_wad.h"
#include "v_video.h"
#include "z_zone.h"

// swirl factors determine the number of waves per flat width
// 1 cycle per 64 units

#define SWIRLFACTOR (8192/64)

// 1 cycle per 32 units (2 in 64)
#define SWIRLFACTOR2 (8192/32)

static byte *normalflat;
int r_swirl;       // hack

#if 0
// DEBUG: draw lines on the flat to see the distortion
static void R_DrawLines(void)
{
   int x,y;
   
   for(x = 0; x < 64; ++x)
      for(y = 0; y < 64; ++y)
         if((!x % 8) || (!y % 8)) 
            normalflat[(y<<6)+x] = 0;
}
#endif

#define AMP 2
#define AMP2 2
#define SPEED 40


//
// R_DistortedFlat
//
// Generates a distorted flat from a normal one using a two-dimensional
// sine wave pattern.
//
byte *R_DistortedFlat(int texnum, bool usegametic)
{
   static int lasttex = -1;
   static int swirltic = -1;
   static int *offset;
   static int offsetSize;
   static byte *distortedflat;
   static int lastsize;

   int i;
   int reftime = usegametic ? gametic : leveltime;
   int leveltic = reftime;
   texture_t *tex = R_CacheTexture(texnum);

   // NOTE: these are transposed because of the swirling formula
   int16_t h = tex->width;
   int16_t w = tex->height;
   int cursize = w*h;

   if(cursize > offsetSize)
   {
      offsetSize = cursize * 2;
      offset = erealloc(int *, offset, offsetSize * sizeof(*offset));
      distortedflat = erealloc(byte *, distortedflat, offsetSize * sizeof(*distortedflat));
   }
   // Already swirled this one?
   if(reftime == swirltic && lasttex == texnum)
      return distortedflat;
      
   lasttex = texnum;

   // built this tic?
   if(reftime != swirltic || cursize != lastsize)
   {
      int x, y;

      if(offset)  // just to silence the static analyzer
      {
         for(x = 0; x < w; ++x)
         {
            for(y = 0; y < h; ++y)
            {
               int x1, y1;
               int sinvalue, sinvalue2;

               sinvalue = (y * SWIRLFACTOR + leveltic*SPEED*5 + 900) & 8191;
               sinvalue2 = (x * SWIRLFACTOR2 + leveltic*SPEED*4 + 300) & 8191;
               x1 = x + 128
                    + ((finesine[sinvalue]*AMP) >> FRACBITS)
                    + ((finesine[sinvalue2]*AMP2) >> FRACBITS);

               sinvalue = (x * SWIRLFACTOR + leveltic*SPEED*3 + 700) & 8191;
               sinvalue2 = (y * SWIRLFACTOR2 + leveltic*SPEED*4 + 1200) & 8191;
               y1 = y + 128
                    + ((finesine[sinvalue]*AMP) >> FRACBITS)
                    + ((finesine[sinvalue2]*AMP2) >> FRACBITS);

               x1 %= w;
               y1 %= h;

               offset[(y*w) + x] = (y1*w) + x1;
            }
         }
      }
      
      swirltic = reftime;
      lastsize = cursize;
   }
   
   normalflat = tex->buffer;
   
   for(i = 0; i < cursize; ++i)
      distortedflat[i] = normalflat[offset[i]];
   
   return distortedflat;
}

// EOF

