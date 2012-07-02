/**
 * @file httpresponse.cpp
 * @brief 
 *
 * $LicenseInfo:firstyear=2012&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2012, Linden Research, Inc.
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

#include "httpresponse.h"
#include "bufferarray.h"
#include "httpheaders.h"


namespace LLCore
{


HttpResponse::HttpResponse()
	: LLCoreInt::RefCounted(true),
	  mReplyOffset(0U),
	  mReplyLength(0U),
	  mReplyFullLength(0U),
	  mBufferArray(NULL),
	  mHeaders(NULL)
{}


HttpResponse::~HttpResponse()
{
	setBody(NULL);
	setHeaders(NULL);
}


void HttpResponse::setBody(BufferArray * ba)
{
	if (mBufferArray == ba)
		return;
	
	if (mBufferArray)
	{
		mBufferArray->release();
	}

	if (ba)
	{
		ba->addRef();
	}
	
	mBufferArray = ba;
}


void HttpResponse::setHeaders(HttpHeaders * headers)
{
	if (mHeaders == headers)
		return;
	
	if (mHeaders)
	{
		mHeaders->release();
	}

	if (headers)
	{
		headers->addRef();
	}
	
	mHeaders = headers;
}


}   // end namespace LLCore
