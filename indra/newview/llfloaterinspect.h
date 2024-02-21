/** 
* @file llfloaterinspect.h
* @author Cube
* @date 2006-12-16
* @brief Declaration of class for displaying object attributes
*
* $LicenseInfo:firstyear=2006&license=viewerlgpl$
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

#ifndef LL_LLFLOATERINSPECT_H
#define LL_LLFLOATERINSPECT_H

#include "llavatarname.h"
#include "llfloater.h"

//class LLTool;
class LLObjectSelection;
class LLScrollListCtrl;
class LLUICtrl;

class LLFloaterInspect : public LLFloater
{
	friend class LLFloaterReg;
public:

//	static void show(void* ignored = NULL);
	void onOpen(const LLSD& key);
	virtual bool postBuild();
	void dirty();
	LLUUID getSelectedUUID();
	virtual void draw();
	virtual void refresh();
//	static bool isVisible();
	virtual void onFocusReceived();
	void onClickCreatorProfile();
	void onClickOwnerProfile();
	void onSelectObject();

	LLScrollListCtrl* mObjectList;
protected:
	// protected members
	void setDirty() { mDirty = true; }
	bool mDirty;

private:
	void onGetOwnerNameCallback();
	void onGetCreatorNameCallback();
	
	LLFloaterInspect(const LLSD& key);
	virtual ~LLFloaterInspect(void);

	LLSafeHandle<LLObjectSelection> mObjectSelection;
	boost::signals2::connection mOwnerNameCacheConnection;
	boost::signals2::connection mCreatorNameCacheConnection;
};

#endif //LL_LLFLOATERINSPECT_H
