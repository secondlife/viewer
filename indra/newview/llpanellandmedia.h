/** 
 * @file llpanellandmedia.h
 * @author Callum Prentice, Sam Kolb, James Cook
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
 *
 * Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
 * $License$
 */
#ifndef LLPANELLANDMEDIA_H
#define LLPANELLANDMEDIA_H

#include "lllineeditor.h"
#include "llpanel.h"
#include "llparcelselection.h"
#include "lluifwd.h"	// widget pointer types

class LLPanelLandMedia
:	public LLPanel
{
public:
	LLPanelLandMedia(LLSafeHandle<LLParcelSelection>& parcelp);
	/*virtual*/ ~LLPanelLandMedia();
	/*virtual*/ BOOL postBuild();
	void refresh();
	void setMediaType(const LLString& media_type);
	void setMediaURL(const LLString& media_type);
	const LLString& getMediaURL() { return mMediaURLEdit->getText(); }

private:
	void populateMIMECombo();
	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onCommitType(LLUICtrl* ctrl, void *userdata);
	static void onSetBtn(void* userdata);

private:
	LLCheckBoxCtrl* mCheckSoundLocal;
	LLRadioGroup*	mRadioVoiceChat;
	LLLineEditor*	mMusicURLEdit;
	LLLineEditor*	mMediaURLEdit;
	LLLineEditor*	mMediaDescEdit;
	LLComboBox*		mMediaTypeCombo;
	LLButton*		mSetURLButton;
	LLSpinCtrl*		mMediaHeightCtrl;
	LLSpinCtrl*		mMediaWidthCtrl;
	LLTextBox*		mMediaSizeCtrlLabel;
	LLTextureCtrl*	mMediaTextureCtrl;
	LLCheckBoxCtrl*	mMediaAutoScaleCheck;
	LLCheckBoxCtrl*	mMediaLoopCheck;
	LLCheckBoxCtrl* mMediaUrlCheck;
	LLCheckBoxCtrl* mMusicUrlCheck;
	LLHandle<LLFloater>	mURLEntryFloater;

	LLSafeHandle<LLParcelSelection>&	mParcel;
};

#endif
