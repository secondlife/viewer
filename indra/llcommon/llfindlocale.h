/** 
 * @file llfindlocale.h
 * @brief Detect system language setting
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef __findlocale_h_
#define __findlocale_h_

typedef const char* FL_Lang;
typedef const char* FL_Country;
typedef const char* FL_Variant;

struct FL_Locale {
  FL_Lang    lang;
  FL_Country country;
  FL_Variant variant;
};

typedef enum {
  /* for some reason we failed to even guess: this should never happen */
  FL_FAILED        = 0,
  /* couldn't query locale -- returning a guess (almost always English) */
  FL_DEFAULT_GUESS = 1,
  /* the returned locale type was found by successfully asking the system */
  FL_CONFIDENT     = 2
} FL_Success;

typedef enum {
  FL_MESSAGES = 0
} FL_Domain;

/* This allocates/fills in a FL_Locale structure with pointers to
   strings (which should be treated as static), or NULL for inappropriate /
   undetected fields. */
LL_COMMON_API FL_Success FL_FindLocale(FL_Locale **locale, FL_Domain domain);
/* This should be used to free the struct written by FL_FindLocale */
LL_COMMON_API void FL_FreeLocale(FL_Locale **locale);

#endif /*__findlocale_h_*/
