/** 
 * @file lluiconstants.h
 * @brief Compile-time configuration for UI
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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

#ifndef LL_LLUICONSTANTS_H
#define LL_LLUICONSTANTS_H

// spacing for small font lines of text, like LLTextBoxes
const S32 LINE = 16;

// spacing for larger lines of text
const S32 LINE_BIG = 24;

// default vertical padding
const S32 VPAD = 4;

// default horizontal padding
const S32 HPAD = 4;

// Account History, how far to look into past
const S32 SUMMARY_INTERVAL = 7;     // one week
const S32 SUMMARY_MAX = 8;          // 
const S32 DETAILS_INTERVAL = 1;     // one day
const S32 DETAILS_MAX = 30;         // one month
const S32 TRANSACTIONS_INTERVAL = 1;// one day
const S32 TRANSACTIONS_MAX = 30;    // one month

#endif
