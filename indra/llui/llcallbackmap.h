/** 
 * @file llcallbackmap.h
 * @brief LLCallbackMap base class
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2007, Linden Research, Inc.
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

// llcallbackmap.h
//
// Copyright 2006, Linden Research, Inc.

#ifndef LL_CALLBACK_MAP_H
#define LL_CALLBACK_MAP_H

#include <map>
#include "llstring.h"

class LLCallbackMap
{
public:
	// callback definition.
	typedef void* (*callback_t)(void* data);
	
	typedef std::map<LLString, LLCallbackMap> map_t;
	typedef map_t::iterator map_iter_t;
	typedef map_t::const_iterator map_const_iter_t;

	LLCallbackMap() : mCallback(NULL), mData(NULL) { }
	LLCallbackMap(callback_t callback, void* data) : mCallback(callback), mData(data) { }

	callback_t	mCallback;
	void*		mData;
};

#endif // LL_CALLBACK_MAP_H
