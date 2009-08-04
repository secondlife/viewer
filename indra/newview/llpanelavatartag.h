/** 
 * @file llpanelavatartag.h
 * @brief Avatar row panel
 *
 * $LicenseInfo:firstyear=2009&license=viewergpl$
 * 
 * Copyright (c) 2009, Linden Research, Inc.
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

#ifndef LL_LLPANELAVATARTAG_H
#define LL_LLPANELAVATARTAG_H

#include "llpanel.h"
#include "llavatarpropertiesprocessor.h"

class LLAvatarIconCtrl;
class LLTextBox;

/**
 * Avatar Tag panel.
 * 
 * Test.
 * 
 * Contains avatar name 
 * Provide methods for setting avatar id, state, muted status and speech power.
 */ 
class LLPanelAvatarTag : public LLPanel
{
public:
	LLPanelAvatarTag(const LLUUID& key, const std::string im_time);
	virtual ~LLPanelAvatarTag();
	
	/**
	 * Set avatar ID.
	 * 
	 * After the ID is set, it is possible to track the avatar status and get its name.
	 */
	void 			setAvatarId(const LLUUID& avatar_id);
	void            setTime    (const std::string& time);

	const LLUUID&	getAvatarId() const	{ return mAvatarId; }

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();	
	
	virtual boost::signals2::connection setLeftButtonClickCallback(
																   const commit_callback_t& cb);
	virtual BOOL handleMouseDown(S32 x, S32 y, MASK mask);

	void onClick();
private:
	void setName(const std::string& name);

	/**
	 * Called by LLCacheName when the avatar name gets updated.
	 */
	void nameUpdatedCallback(
		const LLUUID& id,
		const std::string& first,
		const std::string& last,
		BOOL is_group);
	
	LLAvatarIconCtrl*		mIcon;			/// status tracking avatar icon
	LLTextBox*				mName;			/// displays avatar name
	LLTextBox*				mTime;			/// displays time
	LLUUID					mAvatarId;
//	LLFrameTimer			mFadeTimer;
};

#endif
