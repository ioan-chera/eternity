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
// Purpose: Eyetracker fixation system
// Authors: Ioan Chera
//

#ifndef EYE_FIXATION_H_
#define EYE_FIXATION_H_

#include "m_circbuf.h"
#include "m_vector.h"

//
// Timed gaze. TODO: use Tobii gaze
//
struct gazeevent_t
{
   v2double_t point;
   int64_t timestamp_us;
};

//
// Eyetracker fixation helper
//
class Fixation : public ZoneObject
{
public:
   Fixation(int64_t time_us) : mTime_us(time_us)
   {
   }
   //
   // Pushes a new sight event
   //
   void push(const gazeevent_t &event)
   {
      mSights.push(event);
   }

   bool get(v2double_t &point) const;
   void clear()
   {
      mSights.clear();
   }

private:
   enum
   {
      GAZE_SAMPLES = 400
   };

   CircularBuffer<gazeevent_t, GAZE_SAMPLES> mSights;
   int64_t mTime_us;
};

#endif
