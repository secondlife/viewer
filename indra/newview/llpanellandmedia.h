/**
 * @file llpanellandmedia.h
 * @brief Allows configuration of "media" for a land parcel,
 *   for example movies, web pages, and audio.
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
	void setMediaType(const std::string& media_type);
	void setMediaURL(const std::string& media_type);
	std::string getMediaURL();

private:
	void populateMIMECombo();
	static void onCommitAny(LLUICtrl* ctrl, void *userdata);
	static void onCommitType(LLUICtrl* ctrl, void *userdata);
	static void onSetBtn(void* userdata);
	static void onResetBtn(void* userdata);
	
private:
	LLLineEditor*	mMediaURLEdit;
	LLLineEditor*	mMediaDescEdit;
	LLComboBox*		mMediaTypeCombo;
	LLButton*		mSetURLButton;
	LLSpinCtrl*		mMediaHeightCtrl;
	LLSpinCtrl*		mMediaWidthCtrl;
	LLTextBox*		mMediaResetCtrlLabel;
	LLTextBox*		mMediaSizeCtrlLabel;
	LLTextureCtrl*	mMediaTextureCtrl;
	LLCheckBoxCtrl*	mMediaAutoScaleCheck;
	LLCheckBoxCtrl*	mMediaLoopCheck;
	LLCheckBoxCtrl* mMediaUrlCheck;
	LLHandle<LLFloater>	mURLEntryFloater;


	
	LLSafeHandle<LLParcelSelection>&	mParcel;
};

#endif
