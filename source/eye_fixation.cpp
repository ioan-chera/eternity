//
// The Eternity Engine
// Copyright (C) 2018 James Haley et al.
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
// Additional terms and conditions compatible with the GPLv3 apply. See the
// file COPYING-EE for details.
//
// Purpose: Tobii eye tracker Stream Engine support
// Authors: Ioan Chera
//

#include "z_zone.h"
#include "eye_fixation.h"
#include "hal/i_timer.h"

//
// Various constants. May need tuning.
//
static const double MAX_VARIANCE = 1e-3;

//
// Tries getting fixation coordinates, if available
//
bool Fixation::get(v2double_t &point) const
{
   int length = mSights.length();
   int64_t latest = INT64_MIN;
   
   int count = 0;
   v2double_t mean = {}, mean2 = {};  // for mean and variance
   bool brokeOut = false;
   for(int i = length; i --> 0;)
   {
      const gazeevent_t &event = mSights[i];
      if(latest == INT64_MIN)
         latest = event.timestamp_us;
      if(latest - event.timestamp_us > mTime_us)
      {
         brokeOut = true;
         break;   // no longer measure
      }
      mean += event.point;
      mean2.x += event.point.x * event.point.x;
      mean2.y += event.point.y * event.point.y;
      ++count;
   }
   if(!brokeOut)
      return false;  // not enough samples collected
   mean /= count;
   mean2 /= count;
   v2double_t variance = { mean2.x - mean.x * mean.x, mean2.y - mean.y * mean.y };

   if(variance.chebnorm() > MAX_VARIANCE)
      return false;  // too much variance means no fixation
   point = mean;
   return true;
}

// EOF
