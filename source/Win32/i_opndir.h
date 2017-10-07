// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//
// Implementation of POSIX opendir for Visual C++.
// Derived from the MinGW C Library Extensions Source (released to the
//  public domain).
//
//-----------------------------------------------------------------------------

#ifndef I_OPNDIR_H__
#define I_OPNDIR_H__

#ifndef FILENAME_MAX
#define FILENAME_MAX 260
#endif

#include "../d_io.h"

struct dirent
{
   long		  d_ino;    /* Always zero. */
   unsigned short d_reclen; /* Always zero. */
   unsigned short d_namlen; /* Length of name in d_name. */
   char           d_name[FILENAME_MAX]; /* File name. */
};

/*
 * This is an internal data structure. Good programmers will not use it
 * except as an argument to one of the functions below.
 * dd_stat field is now int (was short in older versions).
 */
typedef struct
{
   /* disk transfer area for this dir */
   struct _finddata_t dd_dta;

   /* dirent struct to return from dir (NOTE: this makes this thread
    * safe as long as only one thread uses a particular DIR struct at
    * a time) */
   struct dirent dd_dir;

   /* _findnext handle */
   intptr_t	dd_handle;

   /*
    * Status of search:
    *   0 = not started yet (next entry to read is first entry)
    *  -1 = off the end
    *   positive = 0 based index of next entry
    */
   int dd_stat;

   /* given path for dir with search pattern (struct is extended) */
   char dd_name[1];
} DIR;

DIR *opendir(const char *);
struct dirent *readdir(DIR *);
int closedir(DIR *);
void rewinddir(DIR *);
long telldir(DIR *);
void seekdir(DIR *, long);

#endif

// EOF

