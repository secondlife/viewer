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
#include "llglimmediate.h"
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
#include "llappviewer.h"
#include "llweb.h"

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
:	LLPanel(name, rect, FALSE),
	mPercentDone( 0.f ),
	mMouseDownInActiveArea( false )
{
	const S32 CANCEL_BTN_WIDTH = 70;
	const S32 CANCEL_BTN_OFFSET = 16;
	LLRect r;
	r.setOriginAndSize( 
		getRect().getWidth() - CANCEL_BTN_OFFSET - CANCEL_BTN_WIDTH, CANCEL_BTN_OFFSET,
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

	mOutlineRect.set( 0, 0, 0, 0 );

	sInstance = this;
}


LLProgressView::~LLProgressView()
{
	gFocusMgr.releaseFocusIfNeeded( this );

	sInstance = NULL;
}

BOOL LLProgressView::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if ( mOutlineRect.pointInRect( x, y ) )
	{
		mMouseDownInActiveArea = TRUE;
		return TRUE;
	};

	return LLPanel::handleMouseDown(x, y, mask);
}

BOOL LLProgressView::handleMouseUp(S32 x, S32 y, MASK mask)
{
	if ( mOutlineRect.pointInRect( x, y ) )
	{
		if ( mMouseDownInActiveArea )
		{
			if ( ! mMessage.empty() )
			{
				std::string url_to_open( "" );

				size_t start_pos = mMessage.find( "http://" );
				if ( start_pos != std::string::npos )
				{
					size_t end_pos = mMessage.find_first_of( " \n\r\t", start_pos );
					if ( end_pos != std::string::npos )
						url_to_open = mMessage.substr( start_pos, end_pos - start_pos );
					else
						url_to_open = mMessage.substr( start_pos );

					LLWeb::loadURLExternal( url_to_open );
				};
			};
			return TRUE;
		};
	};

	return LLPanel::handleMouseUp(x, y, mask);
}

BOOL LLProgressView::handleHover(S32 x, S32 y, MASK mask)
{
	if( childrenHandleHover( x, y, mask ) == NULL )
	{
		lldebugst(LLERR_USER_INPUT) << "hover handled by LLProgressView" << llendl;
		if ( mOutlineRect.pointInRect( x, y ) )
		{
			gViewerWindow->setCursor(UI_CURSOR_ARROW);
		}
		else
		{
			gViewerWindow->setCursor(UI_CURSOR_WAIT);
		}
	}
	return TRUE;
}


BOOL LLProgressView::handleKeyHere(KEY key, MASK mask)
{
	// Suck up all keystokes except CTRL-Q.
	if( ('Q' == key) && (MASK_CONTROL == mask) )
	{
		LLAppViewer::instance()->userQuit();
	}
	return TRUE;
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
		setFocus(TRUE);
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
	if( (width != getRect().getWidth()) || (height != getRect().getHeight()) )
	{
		reshape( width, height );
	}

	// Paint bitmap if we've got one
	glPushMatrix();
	if (gStartImageGL)
	{
		LLGLSUIDefault gls_ui;
		LLViewerImage::bindTexture(gStartImageGL);
		gGL.color4f(1.f, 1.f, 1.f, mFadeTimer.getStarted() ? clamp_rescale(mFadeTimer.getElapsedTimeF32(), 0.f, FADE_IN_TIME, 1.f, 0.f) : 1.f);
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
		gl_rect_2d_simple_tex( getRect().getWidth(), getRect().getHeight() );
		gStartImageGL->unbindTexture(0, GL_TEXTURE_2D);
	}
	else
	{
		LLGLSNoTexture gls_no_texture;
		gGL.color4f(0.f, 0.f, 0.f, 1.f);
		gl_rect_2d(getRect());
	}
	glPopMatrix();

	// Handle fade-in animation
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

	S32 line_x = getRect().getWidth() / 2;
	S32 line_one_y = getRect().getHeight() / 2 + 64;
	const S32 LINE_SPACING = 25;
	S32 line_two_y = line_one_y - LINE_SPACING;
	const LLFontGL* font = LLFontGL::sSansSerif;

	LLUIImagePtr shadow_imagep = LLUI::getUIImage("rounded_square_soft.tga");
	LLUIImagePtr bar_imagep = LLUI::getUIImage("rounded_square.tga");

	//LLColor4 background_color = gColors.getColor("DefaultShadowLight");
	LLColor4 background_color = LLColor4(0.3254f, 0.4f, 0.5058f, 1.0f);

	F32 alpha = 0.5f + 0.5f*0.5f*(1.f + (F32)sin(3.f*timer.getElapsedTimeF32()));
	// background_color.mV[3] = background_color.mV[3]*alpha;

	LLString top_line = LLAppViewer::instance()->getSecondLifeTitle();

	S32 bar_bottom = line_two_y - 30;
	S32 bar_height = 18;
	S32 bar_width = getRect().getWidth() * 2 / 3;
	S32 bar_left = (getRect().getWidth() / 2) - (bar_width / 2);

	// translucent outline box
	S32 background_box_left = ( ( ( getRect().getWidth() / 2 ) - ( bar_width / 2 ) ) / 4 ) * 3;
	S32 background_box_top = ( getRect().getHeight() / 2 ) + LINE_SPACING * 5;
	S32 background_box_right = getRect().getWidth() - background_box_left;
	S32 background_box_bottom = ( getRect().getHeight() / 2 ) - LINE_SPACING * 5;
	S32 background_box_width = background_box_right - background_box_left + 1;
	S32 background_box_height = background_box_top - background_box_bottom + 1;

	shadow_imagep->draw( background_box_left + 2, 
									background_box_bottom - 2, 
									background_box_width, 
									background_box_height,
									gColors.getColor( "ColorDropShadow" ) );
	bar_imagep->draw( background_box_left, 
									background_box_bottom, 
									background_box_width, 
									background_box_height,
									LLColor4( 0.0f, 0.0f, 0.0f, 0.4f ) );

	bar_imagep->draw( background_box_left + 1,
									background_box_bottom + 1, 
									background_box_width - 2,
									background_box_height - 2,
									LLColor4( 0.4f, 0.4f, 0.4f, 0.3f ) );

	// we'll need this later for catching a click if it looks like it contains a link
	if ( mMessage.find( "http://" ) != std::string::npos )
		mOutlineRect.set( background_box_left, background_box_top, background_box_right, background_box_bottom );
	else
		mOutlineRect.set( 0, 0, 0, 0 );

	// draw loading bar
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

	shadow_imagep->draw(
		bar_left + 2, 
		bar_bottom - 2, 
		bar_width, 
		bar_height,
		gColors.getColor("ColorDropShadow"));

	bar_imagep->draw(
		bar_left, 
		bar_bottom, 
		bar_width, 
		bar_height,
		LLColor4(0.7f, 0.7f, 0.8f, 1.0f));

	bar_imagep->draw(
		bar_left + 2, 
		bar_bottom + 2,
		bar_width - 4, 
		bar_height - 4,
		background_color);

	LLColor4 bar_color = LLColor4(0.5764f, 0.6627f, 0.8352f, 1.0f);
	bar_color.mV[3] = alpha;
	bar_imagep->draw(
		bar_left + 2, 
		bar_bottom + 2,
		llround((bar_width - 4) * (mPercentDone / 100.f)), 
		bar_height - 4,
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
		LLAppViewer::instance()->requestQuit();
	}
	else
	{
		gAgent.teleportCancel();
		sInstance->mCancelBtn->setEnabled(FALSE);
		sInstance->setVisible(FALSE);
	}
}
