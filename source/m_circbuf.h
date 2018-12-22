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

#ifndef M_CIRCBUF_H_
#define M_CIRCBUF_H_

//
// Circular buffer, constant size. Trying to overflow will autoremove the oldest entry
//
template<typename T, int N>
class CircularBuffer : public ZoneObject
{
public:
   CircularBuffer() : start(), end()
   {
   }
   void push(const T &item)
   {
      data[end++] = item;
      if(end == N)
         end = 0;
      if(end == start && ++start == N)
         start = 0;
   }
   void clear()
   {
      end = start;
   }
   int length() const
   {
      int n = end - start;
      return n < 0 ? n + N : n;
   }
   T &operator[](int ind)
   {
      int n = start + ind;
      return data[n >= N ? n - N : n];
   }
   const T &operator[](int ind) const
   {
      int n = start + ind;
      return data[n >= N ? n - N : n];
   }
private:
   T data[N];
   int start, end;
};

#endif
