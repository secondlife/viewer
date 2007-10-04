/** 
 * @file llprogressview.cpp
 * @brief LLProgressView class implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2007, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
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

#include "llprogressview.h"

#include "indra_constants.h"
#include "llmath.h"
#include "llgl.h"
#include "llui.h"
#include "llfontgl.h"
#include "llimagegl.h"
#include "lltimer.h"
#include "llglheaders.h"

#include "llagent.h"
#include "llbutton.h"
#include "llfocusmgr.h"
#include "llstartup.h"
#include "llviewercontrol.h"
#include "llviewerimagelist.h"
#include "llviewerwindow.h"
#include "viewer.h"

LLProgressView* LLProgressView::sInstance = NULL;

S32 gStartImageWidth = 1;
S32 gStartImageHeight = 1;
const F32 FADE_IN_TIME = 1.f;

const LLString ANIMATION_FILENAME = "Login Sequence ";
const LLString ANIMATION_SUFFIX = ".jpg";
const F32 TOTAL_LOGIN_TIME = 10.f;	// seconds, wild guess at time from GL context to actual world view
S32 gLastStartAnimationFrame = 0;	// human-style indexing, first image = 1
const S32 ANIMATION_FRAMES = 1; //13;

// XUI:translate
LLProgressView::LLProgressView(const std::string& name, const LLRect &rect) 
: LLPanel(name, rect, FALSE)
{
	mPercentDone = 0.f;
	mDrawBackground = TRUE;

	const S32 CANCEL_BTN_WIDTH = 70;
	const S32 CANCEL_BTN_OFFSET = 16;
	LLRect r;
	r.setOriginAndSize( 
		mRect.getWidth() - CANCEL_BTN_OFFSET - CANCEL_BTN_WIDTH, CANCEL_BTN_OFFSET,
		CANCEL_BTN_WIDTH, BTN_HEIGHT );
	
	mCancelBtn = new LLButton( 
		"Quit",
		r,
		"",
		LLProgressView::onCancelButtonClicked,
		NULL );
	mCancelBtn->setFollows( FOLLOWS_RIGHT | FOLLOWS_BOTTOM );
	addChild( mCancelBtn );
	mFadeTimer.stop();
	setVisible(FALSE);

	sInstance = this;
}


LLProgressView::~LLProgressView()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

EWidgetType LLProgressView::getWidgetType() const
{
	return WIDGET_TYPE_PROGRESS_VIEW;
}

LLString LLProgressView::getWidgetTag() const
{
	return LL_PROGRESS_VIEW_TAG;
}

BOOL LLProgressView::handleHover(S32 x, S32 y, MASK mask)
{
	if( childrenHandleHover( x, y, mask ) == NULL )
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLProgressView" << llendl;
		gViewerWindow->setCursor(UI_CURSOR_WAIT);
	}
	return TRUE;
}


BOOL LLProgressView::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if( getVisible() )
	{
		// Suck up all keystokes except CTRL-Q.
		if( ('Q' == key) && (MASK_CONTROL == mask) )
		{
			app_user_quit();
		}
		return TRUE;
	}
	return FALSE;
}

void LLProgressView::setVisible(BOOL visible)
{
	if (getVisible() && !visible)
	{
		mFadeTimer.start();
	}
	else if (!getVisible() && visible)
	{
		gFocusMgr.setTopCtrl(this);
		mFadeTimer.stop();
		mProgressTimer.start();
		LLView::setVisible(visible);
	}
}


void LLProgressView::draw()
{
	static LLTimer timer;

	if (gNoRender)
	{
		return;
	}

	// Make sure the progress view always fills the entire window.
	S32 width = gViewerWindow->getWindowWidth();
	S32 height = gViewerWindow->getWindowHeight();
	if( (width != mRect.getWidth()) || (height != mRect.getHeight()) )
	{
		reshape( width, height );
	}

	// Paint bitmap if we've got one
	if (mDrawBackground)
	{
		glPushMatrix();
		if (gStartImageGL)
		{
			LLGLSUIDefault gls_ui;
			LLViewerImage::bindTexture(gStartImageGL);
			glColor4f(1.f, 1.f, 1.f, mFadeTimer.getStarted() ? clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, FADE_IN_TIME, 1.f, 0.f) : 1.f);
			F32 image_aspect = (F32)gStartImageWidth / (F32)gStartImageHeight;
			F32 view_aspect = (F32)width / (F32)height;
			// stretch image to maintain aspect ratio
			if (image_aspect > view_aspect)
			{
				glTranslatef(-0.5f * (image_aspect / view_aspect - 1.f) * width, 0.f, 0.f);
				glScalef(image_aspect / view_aspect, 1.f, 1.f);
			}
			else
			{
				glTranslatef(0.f, -0.5f * (view_aspect / image_aspect - 1.f) * height, 0.f);
				glScalef(1.f, view_aspect / image_aspect, 1.f);
			}
			gl_rect_2d_simple_tex( mRect.getWidth(), mRect.getHeight() );
			gStartImageGL->unbindTexture(0, GL_TEXTURE_2D);
		}
		else
		{
			LLGLSNoTexture gls_no_texture;
			glColor4f(0.f, 0.f, 0.f, 1.f);
			gl_rect_2d(mRect);
		}
		glPopMatrix();
	}

	if (mFadeTimer.getStarted())
	{
		LLView::draw();
		if (mFadeTimer.getElapsedTimeF32() > FADE_IN_TIME)
		{
			gFocusMgr.removeTopCtrlWithoutCallback(this);
			LLView::setVisible(FALSE);
			gStartImageGL = NULL;
		}
		return;
	}

	S32 line_x = mRect.getWidth() / 2;
	S32 line_one_y = mRect.getHeight() / 2 + 64;
	const S32 LINE_SPACING = 25;
	S32 line_two_y = line_one_y - LINE_SPACING;
	const LLFontGL* font = LLFontGL::sSansSerif;

	LLViewerImage* shadow_imagep = gImageList.getImage(LLUUID(gViewerArt.getString("rounded_square_soft.tga")), MIPMAP_FALSE, TRUE);
	LLViewerImage* bar_imagep = gImageList.getImage(LLUUID(gViewerArt.getString("rounded_square.tga")), MIPMAP_FALSE, TRUE);

	//LLColor4 background_color = gColors.getColor("DefaultShadowLight");
	LLColor4 background_color = LLColor4(0.3254f, 0.4f, 0.5058f, 1.0f);

	F32 alpha = 0.5f + 0.5f*0.5f*(1.f + (F32)sin(3.f*timer.getElapsedTimeF32()));
	// background_color.mV[3] = background_color.mV[3]*alpha;

	LLString top_line = gSecondLife;

	font->renderUTF8(top_line, 0,
		line_x, line_one_y,
		LLColor4::white,
		LLFontGL::HCENTER, LLFontGL::BASELINE,
		LLFontGL::DROP_SHADOW);
	font->renderUTF8(mText, 0,
		line_x, line_two_y,
		LLColor4::white,
		LLFontGL::HCENTER, LLFontGL::BASELINE,
		LLFontGL::DROP_SHADOW);

	S32 bar_bottom = line_two_y - 30;
	S32 bar_height = 18;
	S32 bar_width = mRect.getWidth() * 2 / 3;
	S32 bar_left = (mRect.getWidth() / 2) - (bar_width / 2);

	gl_draw_scaled_image_with_border(
		bar_left + 2, 
		bar_bottom - 2, 
		16, 
		16,
		bar_width, 
		bar_height,
		shadow_imagep,
		gColors.getColor("ColorDropShadow"));

	gl_draw_scaled_image_with_border(
		bar_left, 
		bar_bottom, 
		16, 
		16,
		bar_width, 
		bar_height,
		bar_imagep,
		LLColor4(0.7f, 0.7f, 0.8f, 1.0f));

	gl_draw_scaled_image_with_border(bar_left + 2, bar_bottom + 2, 16, 16,
		bar_width - 4, bar_height - 4,
		bar_imagep,
		background_color);

	LLColor4 bar_color = LLColor4(0.5764f, 0.6627f, 0.8352f, 1.0f);
	bar_color.mV[3] = alpha;
	gl_draw_scaled_image_with_border(bar_left + 2, bar_bottom + 2, 16, 16,
		llround((bar_width - 4) * (mPercentDone / 100.f)), bar_height - 4,
		bar_imagep,
		bar_color);

	S32 line_three_y = line_two_y - LINE_SPACING * 3;
	
	// draw the message if there is one
	if(!mMessage.empty())
	{
		LLWString wmessage = utf8str_to_wstring(mMessage);
		const F32 MAX_PIXELS = 640.0f;
		S32 chars_left = wmessage.length();
		S32 chars_this_time = 0;
		S32 msgidx = 0;
		while(chars_left > 0)
		{
			chars_this_time = font->maxDrawableChars(wmessage.substr(msgidx).c_str(),
													 MAX_PIXELS,
													 MAX_STRING - 1,
													 TRUE);
			LLWString wbuffer = wmessage.substr(msgidx, chars_this_time);
			font->render(wbuffer, 0,
						 (F32)line_x, (F32)line_three_y,
						 LLColor4::white,
						 LLFontGL::HCENTER, LLFontGL::BASELINE,
						 LLFontGL::DROP_SHADOW);
			msgidx += chars_this_time;
			chars_left -= chars_this_time;
			line_three_y -= LINE_SPACING;
		}
	}

	// draw children
	LLView::draw();
}

void LLProgressView::setText(const LLString& text)
{
	mText = text;
}

void LLProgressView::setPercent(const F32 percent)
{
	mPercentDone = llclamp(percent, 0.f, 100.f);
}

void LLProgressView::setMessage(const LLString& msg)
{
	mMessage = msg;
}

void LLProgressView::setCancelButtonVisible(BOOL b, const LLString& label)
{
	mCancelBtn->setVisible( b );
	mCancelBtn->setEnabled( b );
	mCancelBtn->setLabelSelected(label);
	mCancelBtn->setLabelUnselected(label);
}

// static
void LLProgressView::onCancelButtonClicked(void*)
{
	if (gAgent.getTeleportState() == LLAgent::TELEPORT_NONE)
	{
		app_request_quit();
	}
	else
	{
		gAgent.teleportCancel();
		sInstance->mCancelBtn->setEnabled(FALSE);
		sInstance->setVisible(FALSE);
	}
}
