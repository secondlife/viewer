/** 
 * @file llcallbackmap.h
 * @brief LLCallbackMap base class
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
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
