/** 
 * @file llfloaterpostcard.h
 * @brief Postcard send floater, allows setting name, e-mail address, etc.
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERPOSTCARD_H
#define LL_LLFLOATERPOSTCARD_H

#include "llfloater.h"
#include "llcheckboxctrl.h"

#include "llmemory.h"
#include "llimagegl.h"

class LLTextEditor;
class LLLineEditor;
class LLButton;

class LLFloaterPostcard 
: public LLFloater
{
public:
	LLFloaterPostcard(LLImageJPEG* jpeg, LLImageGL *img, const LLVector2& img_scale, const LLVector3d& pos_taken_global);
	virtual ~LLFloaterPostcard();

	virtual void init();
	virtual BOOL postBuild();
	virtual void draw();

	static LLFloaterPostcard* showFromSnapshot(LLImageJPEG *jpeg, LLImageGL *img, const LLVector2& img_scale, const LLVector3d& pos_taken_global);

	static void onClickCancel(void* data);
	static void onClickSend(void* data);
	static void onClickPublishHelp(void *data);

	static void uploadCallback(const LLUUID& asset_id,
							   void *user_data,
							   S32 result, LLExtStat ext_status);

	static void updateUserInfo(const char *email);

	static void onMsgFormFocusRecieved(LLUICtrl* receiver, void* data);
	static void missingSubjMsgAlertCallback(S32 option, void* data);

	void sendPostcard();

protected:
	
	LLPointer<LLImageJPEG> mJPEGImage;
	LLPointer<LLImageGL> mViewerImage;
	LLTransactionID mTransactionID;
	LLAssetID mAssetID;
	LLVector2 mImageScale;
	LLVector3d mPosTakenGlobal;
	boolean mHasFirstMsgFocus;
	
	static LLLinkedList<LLFloaterPostcard> sInstances;
};


#endif // LL_LLFLOATERPOSTCARD_H
