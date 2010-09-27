/** 
 * @file llsrv.cpp
 * @brief Wrapper for DNS SRV record lookups
 *
 * $LicenseInfo:firstyear=2007&license=viewergpl$
 * 
 * Copyright (c) 2007-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

#include "llviewerprecompiledheaders.h"

#include "llsrv.h"
#include "llares.h"

struct Responder : public LLAres::UriRewriteResponder
{
	std::vector<std::string> mUris;
	void rewriteResult(const std::vector<std::string> &uris) {
		for (size_t i = 0; i < uris.size(); i++)
		{
			llinfos << "[" << i << "] " << uris[i] << llendl;
		}
		mUris = uris;
	}
};

std::vector<std::string> LLSRV::rewriteURI(const std::string& uri)
{
	LLPointer<Responder> resp = new Responder;

	gAres->rewriteURI(uri, resp);
 	gAres->processAll();

	// It's been observed in deployment that c-ares can return control
	// to us without firing all of our callbacks, in which case the
	// returned vector will be empty, instead of a singleton as we
	// might wish.

	if (!resp->mUris.empty())
	{
		return resp->mUris;
	}

	std::vector<std::string> uris;
	uris.push_back(uri);
	return uris;
}
