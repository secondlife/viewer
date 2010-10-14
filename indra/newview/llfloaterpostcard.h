/** 
 * @file llfloaterpostcard.h
 * @brief Postcard send floater, allows setting name, e-mail address, etc.
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

#ifndef LL_LLFLOATERPOSTCARD_H
#define LL_LLFLOATERPOSTCARD_H

#include "llfloater.h"
#include "llcheckboxctrl.h"

#include "llpointer.h"

class LLTextEditor;
class LLLineEditor;
class LLButton;
class LLViewerTexture;
class LLImageJPEG;

class LLFloaterPostcard 
: public LLFloater
{
public:
	LLFloaterPostcard(const LLSD& key);
	virtual ~LLFloaterPostcard();

	virtual BOOL postBuild();
	virtual void draw();

	static LLFloaterPostcard* showFromSnapshot(LLImageJPEG *jpeg, LLViewerTexture *img, const LLVector2& img_scale, const LLVector3d& pos_taken_global);

	static void onClickCancel(void* data);
	static void onClickSend(void* data);

	static void uploadCallback(const LLUUID& asset_id,
							   void *user_data,
							   S32 result, LLExtStat ext_status);

	static void updateUserInfo(const std::string& email);

	static void onMsgFormFocusRecieved(LLFocusableElement* receiver, void* data);
	bool missingSubjMsgAlertCallback(const LLSD& notification, const LLSD& response);

	void sendPostcard();

private:
	
	LLPointer<LLImageJPEG> mJPEGImage;
	LLPointer<LLViewerTexture> mViewerImage;
	LLTransactionID mTransactionID;
	LLAssetID mAssetID;
	LLVector2 mImageScale;
	LLVector3d mPosTakenGlobal;
	bool mHasFirstMsgFocus;
};


#endif // LL_LLFLOATERPOSTCARD_H
