/** 
 * @file llfloateravatartextures.h
 * @brief Debugging view showing underlying avatar textures and baked textures.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERAVATARTEXTURES_H
#define LL_LLFLOATERAVATARTEXTURES_H

#include "llfloater.h"
#include "lluuid.h"
#include "llstring.h"

class LLTextureCtrl;

class LLFloaterAvatarTextures : public LLFloater
{
public:
	LLFloaterAvatarTextures(const LLUUID& id);
	virtual ~LLFloaterAvatarTextures();

	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void refresh();

	static LLFloaterAvatarTextures* show(const LLUUID& id);

private:
	static void onClickDump(void*);

private:
	LLUUID	mID;
	LLString mTitle;
	LLTextureCtrl* mBakedHead;
	LLTextureCtrl* mBakedEyes;
	LLTextureCtrl* mBakedUpper;
	LLTextureCtrl* mBakedLower;
	LLTextureCtrl* mBakedSkirt;
	LLTextureCtrl* mHair;
	LLTextureCtrl* mMakeup;
	LLTextureCtrl* mEye;
	LLTextureCtrl* mShirt;
	LLTextureCtrl* mUpperTattoo;
	LLTextureCtrl* mUpperJacket;
	LLTextureCtrl* mGloves;
	LLTextureCtrl* mUndershirt;
	LLTextureCtrl* mPants;
	LLTextureCtrl* mLowerTattoo;
	LLTextureCtrl* mShoes;
	LLTextureCtrl* mSocks;
	LLTextureCtrl* mJacket;
	LLTextureCtrl* mUnderpants;
	LLTextureCtrl* mSkirt;
};

#endif
