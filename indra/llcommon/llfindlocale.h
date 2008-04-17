/** 
 * @file llfindlocale.h
 * @brief Detect system language setting
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 * 
 * Copyright (c) 2001-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef __findlocale_h_
#define __findlocale_h_

typedef const char* FL_Lang;
typedef const char* FL_Country;
typedef const char* FL_Variant;

typedef struct {
  FL_Lang    lang;
  FL_Country country;
  FL_Variant variant;
} FL_Locale;

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
FL_Success FL_FindLocale(FL_Locale **locale, FL_Domain domain);
/* This should be used to free the struct written by FL_FindLocale */
void FL_FreeLocale(FL_Locale **locale);

#endif /*__findlocale_h_*/
