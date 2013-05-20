/** 
 * @file llfloatertexturefetchdebugger.h
 * @brief texture fetching debugger window, debug use only
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_FLOATER_TEXTURE_FETCH_DEBUGGER__H
#define LL_FLOATER_TEXTURE_FETCH_DEBUGGER__H

#include "llfloater.h"
class LLTextureFetchDebugger;

class LLFloaterTextureFetchDebugger : public LLFloater
{
	friend class LLFloaterReg;
public:
	/// initialize all the callbacks for the menu

	virtual BOOL postBuild() ;
	virtual void draw() ;
	
	void onChangeTexelPixelRatio();
	
	void onClickStart();
	void onClickClear();
	void onClickClose();

	void onClickCacheRead();
	void onClickCacheWrite();
	void onClickHTTPLoad();
	void onClickDecode();
	void onClickGLTexture();

	void onClickRefetchVisCache();
	void onClickRefetchVisHTTP();
	void onClickRefetchAllCache();
	void onClickRefetchAllHTTP();
public:
	void idle() ;

private:	
	LLFloaterTextureFetchDebugger(const LLSD& key);
	virtual ~LLFloaterTextureFetchDebugger();

	void updateButtons();
	void disableButtons();

	void setStartStatus(S32 status);
	bool idleStart();
private:	
	LLTextureFetchDebugger* mDebugger;
	std::map<std::string, bool> mButtonStateMap;
	S32 mStartStatus;
};

#endif // LL_FLOATER_TEXTURE_FETCH_DEBUGGER__H
