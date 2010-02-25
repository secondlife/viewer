/**
 * @file LLMediaCtrl.cpp
 * @brief Web browser UI control
 *
 * $LicenseInfo:firstyear=2006&license=viewergpl$
 * 
 * Copyright (c) 2006-2009, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"


#include "llmediactrl.h"

// viewer includes
#include "llfloaterworldmap.h"
#include "lluictrlfactory.h"
#include "llurldispatcher.h"
#include "llviewborder.h"
#include "llviewercontrol.h"
#include "llviewermedia.h"
#include "llviewertexture.h"
#include "llviewerwindow.h"
#include "llnotificationsutil.h"
#include "llweb.h"
#include "llrender.h"
#include "llpluginclassmedia.h"
#include "llslurl.h"
#include "lluictrlfactory.h"	// LLDefaultChildRegistry
#include "llkeyboard.h"

// linden library includes
#include "llfocusmgr.h"

extern BOOL gRestoreGL;

static LLDefaultChildRegistry::Register<LLMediaCtrl> r("web_browser");

LLMediaCtrl::Params::Params()
:	start_url("start_url"),
	border_visible("border_visible", true),
	ignore_ui_scale("ignore_ui_scale", true),
	hide_loading("hide_loading", false),
	decouple_texture_size("decouple_texture_size", false),
	texture_width("texture_width", 1024),
	texture_height("texture_height", 1024),
	caret_color("caret_color")
{
	tab_stop(false);
}

LLMediaCtrl::LLMediaCtrl( const Params& p) :
	LLPanel( p ),
	mTextureDepthBytes( 4 ),
	mBorder(NULL),
	mFrequentUpdates( true ),
	mForceUpdate( false ),
	mOpenLinksInExternalBrowser( false ),
	mOpenLinksInInternalBrowser( false ),
	mTrusted( false ),
	mHomePageUrl( "" ),
	mIgnoreUIScale( true ),
	mAlwaysRefresh( false ),
	mMediaSource( 0 ),
	mTakeFocusOnClick( true ),
	mCurrentNavUrl( "" ),
	mStretchToFill( true ),
	mMaintainAspectRatio ( true ),
	mHideLoading (false),
	mHidingInitialLoad (false),
	mDecoupleTextureSize ( false ),
	mTextureWidth ( 1024 ),
	mTextureHeight ( 1024 ),
	mClearCache(false)
{
	{
		LLColor4 color = p.caret_color().get();
		setCaretColor( (unsigned int)color.mV[0], (unsigned int)color.mV[1], (unsigned int)color.mV[2] );
	}

	setIgnoreUIScale(p.ignore_ui_scale);
	
	setHomePageUrl(p.start_url);
	
	setBorderVisible(p.border_visible);
	
	mHideLoading = p.hide_loading;
	
	setDecoupleTextureSize(p.decouple_texture_size);
	
	setTextureSize(p.texture_width, p.texture_height);

	if(!getDecoupleTextureSize())
	{
		S32 screen_width = mIgnoreUIScale ? 
			llround((F32)getRect().getWidth() * LLUI::sGLScaleFactor.mV[VX]) : getRect().getWidth();
		S32 screen_height = mIgnoreUIScale ? 
			llround((F32)getRect().getHeight() * LLUI::sGLScaleFactor.mV[VY]) : getRect().getHeight();
			
		setTextureSize(screen_width, screen_height);
	}
	
	mMediaTextureID.generate();
	
	// We don't need to create the media source up front anymore unless we have a non-empty home URL to navigate to.
	if(!mHomePageUrl.empty())
	{
		navigateHome();
	}
		
	// FIXME: How do we create a bevel now?
//	LLRect border_rect( 0, getRect().getHeight() + 2, getRect().getWidth() + 2, 0 );
//	mBorder = new LLViewBorder( std::string("web control border"), border_rect, LLViewBorder::BEVEL_IN );
//	addChild( mBorder );
}

////////////////////////////////////////////////////////////////////////////////
// note: this is now a singleton and destruction happens via initClass() now
LLMediaCtrl::~LLMediaCtrl()
{

	if (mMediaSource)
	{
		mMediaSource->remObserver( this );
		mMediaSource = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setBorderVisible( BOOL border_visible )
{
	if ( mBorder )
	{
		mBorder->setVisible( border_visible );
	};
};

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setTakeFocusOnClick( bool take_focus )
{
	mTakeFocusOnClick = take_focus;
}

////////////////////////////////////////////////////////////////////////////////
// set flag that forces the embedded browser to open links in the external system browser
void LLMediaCtrl::setOpenInExternalBrowser( bool valIn )
{
	mOpenLinksInExternalBrowser = valIn;
};

////////////////////////////////////////////////////////////////////////////////
// set flag that forces the embedded browser to open links in the internal browser floater
void LLMediaCtrl::setOpenInInternalBrowser( bool valIn )
{
	mOpenLinksInInternalBrowser = valIn;
};

////////////////////////////////////////////////////////////////////////////////
void LLMediaCtrl::setTrusted( bool valIn )
{
	mTrusted = valIn;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleHover( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseMove(x, y, mask);
		gViewerWindow->setCursor(mMediaSource->getLastSetCursor());
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
	if (mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->scrollEvent(0, clicks, gKeyboard->currentMask(TRUE));

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleMouseUp( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseUp(x, y, mask);

		// *HACK: LLMediaImplLLMozLib automatically takes focus on mouseup,
		// in addition to the onFocusReceived() call below.  Undo this. JC
		if (!mTakeFocusOnClick)
		{
			mMediaSource->focus(false);
			gViewerWindow->focusClient();
		}
	}
	
	gFocusMgr.setMouseCapture( NULL );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleMouseDown( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDown(x, y, mask);
	
	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleRightMouseUp( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
	{
		mMediaSource->mouseUp(x, y, mask, 1);

		// *HACK: LLMediaImplLLMozLib automatically takes focus on mouseup,
		// in addition to the onFocusReceived() call below.  Undo this. JC
		if (!mTakeFocusOnClick)
		{
			mMediaSource->focus(false);
			gViewerWindow->focusClient();
		}
	}
	
	gFocusMgr.setMouseCapture( NULL );

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDown(x, y, mask, 1);
	
	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleDoubleClick( S32 x, S32 y, MASK mask )
{
	convertInputCoords(x, y);

	if (mMediaSource)
		mMediaSource->mouseDoubleClick( x, y, mask);

	gFocusMgr.setMouseCapture( this );

	if (mTakeFocusOnClick)
	{
		setFocus( TRUE );
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onFocusReceived()
{
	if (mMediaSource)
	{
		mMediaSource->focus(true);
		
		// Set focus for edit menu items
		LLEditMenuHandler::gEditMenuHandler = mMediaSource;
	}
	
	LLPanel::onFocusReceived();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onFocusLost()
{
	if (mMediaSource)
	{
		mMediaSource->focus(false);

		if( LLEditMenuHandler::gEditMenuHandler == mMediaSource )
		{
			// Clear focus for edit menu items
			LLEditMenuHandler::gEditMenuHandler = NULL;
		}
	}

	gViewerWindow->focusClient();

	LLPanel::onFocusLost();
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::postBuild ()
{
	setVisibleCallback(boost::bind(&LLMediaCtrl::onVisibilityChange, this, _2));
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleKeyHere( KEY key, MASK mask )
{
	BOOL result = FALSE;
	
	if (mMediaSource)
	{
		result = mMediaSource->handleKeyHere(key, mask);
	}
		
	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::handleVisibilityChange ( BOOL new_visibility )
{
	llinfos << "visibility changed to " << (new_visibility?"true":"false") << llendl;
	if(mMediaSource)
	{
		mMediaSource->setVisible( new_visibility );
	}
}

////////////////////////////////////////////////////////////////////////////////
//
BOOL LLMediaCtrl::handleUnicodeCharHere(llwchar uni_char)
{
	BOOL result = FALSE;
	
	if (mMediaSource)
	{
		result = mMediaSource->handleUnicodeCharHere(uni_char);
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onVisibilityChange ( const LLSD& new_visibility )
{
	// set state of frequent updates automatically if visibility changes
	if ( new_visibility.asBoolean() )
	{
		mFrequentUpdates = true;
	}
	else
	{
		mFrequentUpdates = false;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::reshape( S32 width, S32 height, BOOL called_from_parent )
{
	if(!getDecoupleTextureSize())
	{
		S32 screen_width = mIgnoreUIScale ? llround((F32)width * LLUI::sGLScaleFactor.mV[VX]) : width;
		S32 screen_height = mIgnoreUIScale ? llround((F32)height * LLUI::sGLScaleFactor.mV[VY]) : height;

		// when floater is minimized, these sizes are negative
		if ( screen_height > 0 && screen_width > 0 )
		{
			setTextureSize(screen_width, screen_height);
		}
	}
	
	LLUICtrl::reshape( width, height, called_from_parent );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateBack()
{
	if (mMediaSource && mMediaSource->hasMedia())
	{
		mMediaSource->getMediaPlugin()->browse_back();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateForward()
{
	if (mMediaSource && mMediaSource->hasMedia())
	{
		mMediaSource->getMediaPlugin()->browse_forward();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::canNavigateBack()
{
	if (mMediaSource)
		return mMediaSource->canNavigateBack();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::canNavigateForward()
{
	if (mMediaSource)
		return mMediaSource->canNavigateForward();
	else
		return false;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::set404RedirectUrl( std::string redirect_url )
{
	if(mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->set_status_redirect( 404, redirect_url );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::clr404RedirectUrl()
{
	if(mMediaSource && mMediaSource->hasMedia())
		mMediaSource->getMediaPlugin()->set_status_redirect(404, "");
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::clearCache()
{
	if(mMediaSource)
	{
		mMediaSource->clearCache();
	}
	else
	{
		mClearCache = true;
	}

}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateTo( std::string url_in, std::string mime_type)
{
	// don't browse to anything that starts with secondlife:// or sl://
	const std::string protocol1 = "secondlife://";
	const std::string protocol2 = "sl://";
	if ((LLStringUtil::compareInsensitive(url_in.substr(0, protocol1.length()), protocol1) == 0) ||
	    (LLStringUtil::compareInsensitive(url_in.substr(0, protocol2.length()), protocol2) == 0))
	{
		// TODO: Print out/log this attempt?
		// llinfos << "Rejecting attempt to load restricted website :" << urlIn << llendl;
		return;
	}
	
	if (ensureMediaSourceExists())
	{
		mCurrentNavUrl = url_in;
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateTo(url_in, mime_type, mime_type.empty());
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateToLocalPage( const std::string& subdir, const std::string& filename_in )
{
	std::string language = LLUI::getLanguage();
	std::string delim = gDirUtilp->getDirDelimiter();
	std::string filename;

	filename += subdir;
	filename += delim;
	filename += filename_in;

	std::string expanded_filename = gDirUtilp->findSkinnedFilename("html", language, filename);

	if (! gDirUtilp->fileExists(expanded_filename))
	{
		if (language != "en-us")
		{
			expanded_filename = gDirUtilp->findSkinnedFilename("html", "en-us", filename);
			if (! gDirUtilp->fileExists(expanded_filename))
			{
				llwarns << "File " << subdir << delim << filename_in << "not found" << llendl;
				return;
			}
		}
		else
		{
			llwarns << "File " << subdir << delim << filename_in << "not found" << llendl;
			return;
		}
	}
	if (ensureMediaSourceExists())
	{
		mCurrentNavUrl = expanded_filename;
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateTo(expanded_filename, "text/html", false);
	}

}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateHome()
{
	if (ensureMediaSourceExists())
	{
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mMediaSource->navigateHome();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setHomePageUrl( const std::string urlIn )
{
	mHomePageUrl = urlIn;
	if (mMediaSource)
	{
		mMediaSource->setHomeURL(mHomePageUrl);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::setCaretColor(unsigned int red, unsigned int green, unsigned int blue)
{
	//NOOP
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setTextureSize(S32 width, S32 height)
{
	mTextureWidth = width;
	mTextureHeight = height;
	
	if(mMediaSource)
	{
		mMediaSource->setSize(mTextureWidth, mTextureHeight);
		mForceUpdate = true;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLMediaCtrl::getHomePageUrl()
{
	return 	mHomePageUrl;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::ensureMediaSourceExists()
{	
	if(mMediaSource.isNull())
	{
		// If we don't already have a media source, try to create one.
		mMediaSource = LLViewerMedia::newMediaImpl(mMediaTextureID, mTextureWidth, mTextureHeight);
		if ( mMediaSource )
		{
			mMediaSource->setUsedInUI(true);
			mMediaSource->setHomeURL(mHomePageUrl);
			mMediaSource->setVisible( getVisible() );
			mMediaSource->addObserver( this );
			mMediaSource->setBackgroundColor( getBackgroundColor() );
			if(mClearCache)
			{
				mMediaSource->clearCache();
				mClearCache = false;
			}
			
			if(mHideLoading)
			{
				mHidingInitialLoad = true;
			}
		}
		else
		{
			llwarns << "media source create failed " << llendl;
			// return;
		}
	}
	
	return !mMediaSource.isNull();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::unloadMediaSource()
{
	mMediaSource = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
LLPluginClassMedia* LLMediaCtrl::getMediaPlugin()
{ 
	return mMediaSource.isNull() ? NULL : mMediaSource->getMediaPlugin(); 
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::draw()
{
	if ( gRestoreGL == 1 )
	{
		LLRect r = getRect();
		reshape( r.getWidth(), r.getHeight(), FALSE );
		return;
	}

	// NOTE: optimization needed here - probably only need to do this once
	// unless tearoffs change the parent which they probably do.
	const LLUICtrl* ptr = findRootMostFocusRoot();
	if ( ptr && ptr->hasFocus() )
	{
		setFrequentUpdates( true );
	}
	else
	{
		setFrequentUpdates( false );
	};
	
	bool draw_media = false;
	
	LLPluginClassMedia* media_plugin = NULL;
	LLViewerMediaTexture* media_texture = NULL;
	
	if(mMediaSource && mMediaSource->hasMedia())
	{
		media_plugin = mMediaSource->getMediaPlugin();

		if(media_plugin && (media_plugin->textureValid()))
		{
			media_texture = LLViewerTextureManager::findMediaTexture(mMediaTextureID);
			if(media_texture)
			{
				draw_media = true;
			}
		}
	}
	
	if(mHidingInitialLoad)
	{
		// If we're hiding loading, don't draw at all.
		draw_media = false;
	}
	
	bool background_visible = isBackgroundVisible();
	bool background_opaque = isBackgroundOpaque();
	
	if(draw_media)
	{
		// alpha off for this
		LLGLSUIDefault gls_ui;
		LLGLDisable gls_alphaTest( GL_ALPHA_TEST );

		gGL.pushMatrix();
		{
			if (mIgnoreUIScale)
			{
				glLoadIdentity();
				// font system stores true screen origin, need to scale this by UI scale factor
				// to get render origin for this view (with unit scale)
				gGL.translatef(floorf(LLFontGL::sCurOrigin.mX * LLUI::sGLScaleFactor.mV[VX]), 
							floorf(LLFontGL::sCurOrigin.mY * LLUI::sGLScaleFactor.mV[VY]), 
							LLFontGL::sCurOrigin.mZ);
			}

			// scale texture to fit the space using texture coords
			gGL.getTexUnit(0)->bind(media_texture);
			gGL.color4fv( LLColor4::white.mV );
			F32 max_u = ( F32 )media_plugin->getWidth() / ( F32 )media_plugin->getTextureWidth();
			F32 max_v = ( F32 )media_plugin->getHeight() / ( F32 )media_plugin->getTextureHeight();

			LLRect r = getRect();
			S32 width, height;
			S32 x_offset = 0;
			S32 y_offset = 0;
			
			if(mStretchToFill)
			{
				if(mMaintainAspectRatio)
				{
					F32 media_aspect = (F32)(media_plugin->getWidth()) / (F32)(media_plugin->getHeight());
					F32 view_aspect = (F32)(r.getWidth()) / (F32)(r.getHeight());
					if(media_aspect > view_aspect)
					{
						// max width, adjusted height
						width = r.getWidth();
						height = llmin(llmax(llround(width / media_aspect), 0), r.getHeight());
					}
					else
					{
						// max height, adjusted width
						height = r.getHeight();
						width = llmin(llmax(llround(height * media_aspect), 0), r.getWidth());
					}
				}
				else
				{
					width = r.getWidth();
					height = r.getHeight();
				}
			}
			else
			{
				width = llmin(media_plugin->getWidth(), r.getWidth());
				height = llmin(media_plugin->getHeight(), r.getHeight());
			}
			
			x_offset = (r.getWidth() - width) / 2;
			y_offset = (r.getHeight() - height) / 2;		

			if(mIgnoreUIScale)
			{
				x_offset = llround((F32)x_offset * LLUI::sGLScaleFactor.mV[VX]);
				y_offset = llround((F32)y_offset * LLUI::sGLScaleFactor.mV[VY]);
				width = llround((F32)width * LLUI::sGLScaleFactor.mV[VX]);
				height = llround((F32)height * LLUI::sGLScaleFactor.mV[VY]);
			}

			// draw the browser
			gGL.setSceneBlendType(LLRender::BT_REPLACE);
			gGL.begin( LLRender::QUADS );
			if (! media_plugin->getTextureCoordsOpenGL())
			{
				// render using web browser reported width and height, instead of trying to invert GL scale
				gGL.texCoord2f( max_u, 0.f );
				gGL.vertex2i( x_offset + width, y_offset + height );

				gGL.texCoord2f( 0.f, 0.f );
				gGL.vertex2i( x_offset, y_offset + height );

				gGL.texCoord2f( 0.f, max_v );
				gGL.vertex2i( x_offset, y_offset );

				gGL.texCoord2f( max_u, max_v );
				gGL.vertex2i( x_offset + width, y_offset );
			}
			else
			{
				// render using web browser reported width and height, instead of trying to invert GL scale
				gGL.texCoord2f( max_u, max_v );
				gGL.vertex2i( x_offset + width, y_offset + height );

				gGL.texCoord2f( 0.f, max_v );
				gGL.vertex2i( x_offset, y_offset + height );

				gGL.texCoord2f( 0.f, 0.f );
				gGL.vertex2i( x_offset, y_offset );

				gGL.texCoord2f( max_u, 0.f );
				gGL.vertex2i( x_offset + width, y_offset );
			}
			gGL.end();
			gGL.setSceneBlendType(LLRender::BT_ALPHA);
		}
		gGL.popMatrix();
	
	}
	else
	{
		// Setting these will make LLPanel::draw draw the opaque background color.
		setBackgroundVisible(true);
		setBackgroundOpaque(true);
	}
	
	// highlight if keyboard focus here. (TODO: this needs some work)
	if ( mBorder && mBorder->getVisible() )
		mBorder->setKeyboardFocusHighlight( gFocusMgr.childHasKeyboardFocus( this ) );

	
	LLPanel::draw();

	// Restore the previous values
	setBackgroundVisible(background_visible);
	setBackgroundOpaque(background_opaque);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::convertInputCoords(S32& x, S32& y)
{
	bool coords_opengl = false;
	
	if(mMediaSource && mMediaSource->hasMedia())
	{
		coords_opengl = mMediaSource->getMediaPlugin()->getTextureCoordsOpenGL();
	}
	
	x = mIgnoreUIScale ? llround((F32)x * LLUI::sGLScaleFactor.mV[VX]) : x;
	if ( ! coords_opengl )
	{
		y = mIgnoreUIScale ? llround((F32)(y) * LLUI::sGLScaleFactor.mV[VY]) : y;
	}
	else
	{
		y = mIgnoreUIScale ? llround((F32)(getRect().getHeight() - y) * LLUI::sGLScaleFactor.mV[VY]) : getRect().getHeight() - y;
	};
}

////////////////////////////////////////////////////////////////////////////////
// static 
bool LLMediaCtrl::onClickLinkExternalTarget(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotificationsUtil::getSelectedOption(notification, response);
	if ( 0 == option )
	{
		LLSD payload = notification["payload"];
		std::string url = payload["url"].asString();
		S32 target_type = payload["target_type"].asInteger();

		switch (target_type)
		{
		case LLPluginClassMedia::TARGET_EXTERNAL:
			// load target in an external browser
			LLWeb::loadURLExternal(url);
			break;

		case LLPluginClassMedia::TARGET_BLANK:
			// load target in the user's preferred browser
			LLWeb::loadURL(url);
			break;

		default:
			// unsupported link target - shouldn't happen
			LL_WARNS("LinkTarget") << "Unsupported link target type" << LL_ENDL;
			break;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
// inherited from LLViewerMediaObserver
//virtual 
void LLMediaCtrl::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	switch(event)
	{
		case MEDIA_EVENT_CONTENT_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CONTENT_UPDATED " << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_TIME_DURATION_UPDATED:
		{
			// LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_TIME_DURATION_UPDATED, time is " << self->getCurrentTime() << " of " << self->getDuration() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_SIZE_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_SIZE_CHANGED " << LL_ENDL;
			LLRect r = getRect();
			reshape( r.getWidth(), r.getHeight(), FALSE );
		};
		break;
		
		case MEDIA_EVENT_CURSOR_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << self->getCursorName() << LL_ENDL;
		}
		break;
			
		case MEDIA_EVENT_NAVIGATE_BEGIN:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_BEGIN, url is " << self->getNavigateURI() << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAVIGATE_COMPLETE:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_COMPLETE, result string is: " << self->getNavigateResultString() << LL_ENDL;
			if(mHidingInitialLoad)
			{
				mHidingInitialLoad = false;
			}
		};
		break;

		case MEDIA_EVENT_PROGRESS_UPDATED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PROGRESS_UPDATED, loading at " << self->getProgressPercent() << "%" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_STATUS_TEXT_CHANGED, new status text is: " << self->getStatusText() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_LOCATION_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LOCATION_CHANGED, new uri is: " << self->getLocation() << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_CLICK_LINK_HREF:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << self->getClickTarget() << "\", uri is " << self->getClickURL() << LL_ENDL;
			onClickLinkHref(self);
		};
		break;
		
		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is " << self->getClickURL() << LL_ENDL;
			onClickLinkNoFollow(self);
		};
		break;

		case MEDIA_EVENT_PLUGIN_FAILED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED" << LL_ENDL;
		};
		break;

		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED_LAUNCH" << LL_ENDL;
		};
		break;
		
		case MEDIA_EVENT_NAME_CHANGED:
		{
			LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAME_CHANGED" << LL_ENDL;
		};
		break;
	};

	// chain all events to any potential observers of this object.
	emitEvent(self, event);
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLMediaCtrl::onClickLinkHref( LLPluginClassMedia* self )
{
	// retrieve the event parameters
	std::string url = self->getClickURL();
	U32 target_type = self->getClickTargetType();
	
	// is there is a target specified for the link?
	if (target_type == LLPluginClassMedia::TARGET_EXTERNAL ||
		target_type == LLPluginClassMedia::TARGET_BLANK)
	{
		LLSD payload;
		payload["url"] = url;
		payload["target_type"] = LLSD::Integer(target_type);
		LLNotificationsUtil::add( "WebLaunchExternalTarget", LLSD(), payload, onClickLinkExternalTarget);
		return;
	}

	const std::string protocol1( "http://" );
	const std::string protocol2( "https://" );
	if( mOpenLinksInExternalBrowser )
	{
		if ( !url.empty() )
		{
			if ( LLStringUtil::compareInsensitive( url.substr( 0, protocol1.length() ), protocol1 ) == 0 ||
				 LLStringUtil::compareInsensitive( url.substr( 0, protocol2.length() ), protocol2 ) == 0 )
			{
				LLWeb::loadURLExternal( url );
			}
		}
	}
	else
	if( mOpenLinksInInternalBrowser )
	{
		if ( !url.empty() )
		{
			if ( LLStringUtil::compareInsensitive( url.substr( 0, protocol1.length() ), protocol1 ) == 0 ||
				 LLStringUtil::compareInsensitive( url.substr( 0, protocol2.length() ), protocol2 ) == 0 )
			{
				llwarns << "Dead, unimplemented path that we used to send to the built-in browser long ago." << llendl;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
// 
void LLMediaCtrl::onClickLinkNoFollow( LLPluginClassMedia* self )
{
	// let the dispatcher handle blocking/throttling of SLURLs
	std::string url = self->getClickURL();
	LLURLDispatcher::dispatch(url, this, mTrusted);
}

////////////////////////////////////////////////////////////////////////////////
// 
std::string LLMediaCtrl::getCurrentNavUrl()
{
	return mCurrentNavUrl;
}

