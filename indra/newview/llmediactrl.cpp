/**
 * @file LLMediaCtrl.cpp
 * @brief Web browser UI control
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

#include "llviewerprecompiledheaders.h"
#include "lltooltip.h"

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
#include "lldebugmessagebox.h"
#include "llweb.h"
#include "llrender.h"
#include "llpluginclassmedia.h"
#include "llslurl.h"
#include "lluictrlfactory.h"    // LLDefaultChildRegistry
#include "llkeyboard.h"
#include "llviewermenu.h"
#include "llviewermenufile.h" // LLFilePickerThread

// linden library includes
#include "llfocusmgr.h"
#include "llsdutil.h"
#include "lllayoutstack.h"
#include "lliconctrl.h"
#include "llhttpconstants.h"
#include "lltextbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "lllineeditor.h"
#include "llfloaterwebcontent.h"
#include "llwindowshade.h"
#include "lleventapi.h"
#include "llui.h"

extern bool gRestoreGL;

const std::string PAGE_TEXT_EXTRACT_MARKER = "PAGE_TEXT_EXTRACT:";

class LLMediaCtrlListener;

static LLDefaultChildRegistry::Register<LLMediaCtrl> r("web_browser");

LLMediaCtrl::Params::Params()
:   start_url("start_url"),
    border_visible("border_visible", true),
    decouple_texture_size("decouple_texture_size", false),
    texture_width("texture_width", 1024),
    texture_height("texture_height", 1024),
    caret_color("caret_color"),
    initial_mime_type("initial_mime_type"),
    error_page_url("error_page_url"),
    media_id("media_id"),
    trusted_content("trusted_content", false),
    focus_on_click("focus_on_click", true)
{
}

LLMediaCtrl::LLMediaCtrl( const Params& p) :
    LLPanel( p ),
    LLInstanceTracker<LLMediaCtrl, LLUUID>(LLUUID::generateNewID()),
    mTextureDepthBytes( 4 ),
    mBorder(NULL),
    mFrequentUpdates( true ),
    mForceUpdate( false ),
    mHomePageUrl( "" ),
    mAlwaysRefresh( false ),
    mMediaSource( 0 ),
    mTakeFocusOnClick( p.focus_on_click ),
    mCurrentNavUrl( "" ),
    mStretchToFill( true ),
    mMaintainAspectRatio ( true ),
    mDecoupleTextureSize ( false ),
    mUpdateScrolls( false ),
    mTextureWidth ( 1024 ),
    mTextureHeight ( 1024 ),
    mLoadingState( LOADING_STATE_INITIALIZING ),
    mClearCache(false),
    mHomePageMimeType(p.initial_mime_type),
    mErrorPageURL(p.error_page_url),
    mTrusted(p.trusted_content),
    mWindowShade(NULL),
    mHoverTextChanged(false),
    mAllowFileDownload(false)
{
    {
        LLColor4 color = p.caret_color().get();
        setCaretColor( (unsigned int)color.mV[0], (unsigned int)color.mV[1], (unsigned int)color.mV[2] );
    }

    setHomePageUrl(p.start_url, p.initial_mime_type);

    setBorderVisible(p.border_visible);

    setDecoupleTextureSize(p.decouple_texture_size);

    setTextureSize(p.texture_width, p.texture_height);

    if(!getDecoupleTextureSize())
    {
        S32 screen_width = ll_round((F32)getRect().getWidth() * LLUI::getScaleFactor().mV[VX]);
        S32 screen_height = ll_round((F32)getRect().getHeight() * LLUI::getScaleFactor().mV[VY]);

        setTextureSize(screen_width, screen_height);
    }

    mMediaTextureID = getKey();

    // We don't need to create the media source up front anymore unless we have a non-empty home URL to navigate to.
    if(!mHomePageUrl.empty())
    {
        navigateHome();
    }

    LLWindowShade::Params params;
    params.name = "notification_shade";
    params.rect = getLocalRect();
    params.follows.flags = FOLLOWS_ALL;
    params.modal = true;

    mWindowShade = LLUICtrlFactory::create<LLWindowShade>(params);

    addChild(mWindowShade);
}

LLMediaCtrl::~LLMediaCtrl()
{
    auto menu = mContextMenuHandle.get();
    if (menu)
    {
        menu->die();
        mContextMenuHandle.markDead();
    }

    if (mMediaSource)
    {
        mMediaSource->remObserver( this );
        mMediaSource = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::setBorderVisible( bool border_visible )
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
//
bool LLMediaCtrl::handleHover( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleHover(x, y, mask)) return true;
    convertInputCoords(x, y);

    if (mMediaSource)
    {
        mMediaSource->mouseMove(x, y, mask);
        gViewerWindow->setCursor(mMediaSource->getLastSetCursor());
    }

    // TODO: Is this the right way to handle hover text changes driven by the plugin?
    if(mHoverTextChanged)
    {
        mHoverTextChanged = false;
        handleToolTip(x, y, mask);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleScrollWheel( S32 x, S32 y, S32 clicks )
{
    if (LLPanel::handleScrollWheel(x, y, clicks)) return true;
    if (mMediaSource && (mMediaSource->hasMedia() || mMediaSource->isUsingEmbeddedBrowser()))
    {
        convertInputCoords(x, y);
        mMediaSource->scrollWheel(x, y, 0, clicks, gKeyboard->currentMask(true));
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleScrollHWheel(S32 x, S32 y, S32 clicks)
{
    if (LLPanel::handleScrollHWheel(x, y, clicks)) return true;
    if (mMediaSource && (mMediaSource->hasMedia() || mMediaSource->isUsingEmbeddedBrowser()))
    {
        convertInputCoords(x, y);
        mMediaSource->scrollWheel(x, y, clicks, 0, gKeyboard->currentMask(true));
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//  virtual
bool LLMediaCtrl::handleToolTip(S32 x, S32 y, MASK mask)
{
    std::string hover_text;

    if (mMediaSource && mMediaSource->hasMedia())
        hover_text = mMediaSource->getMediaPlugin()->getHoverText();

    if(hover_text.empty())
    {
        return false;
    }
    else
    {
        S32 screen_x, screen_y;

        localPointToScreen(x, y, &screen_x, &screen_y);
        LLRect sticky_rect_screen;
        sticky_rect_screen.setCenterAndSize(screen_x, screen_y, 20, 20);

        LLToolTipMgr::instance().show(LLToolTip::Params()
            .message(hover_text)
            .sticky_rect(sticky_rect_screen));
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleMouseUp( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleMouseUp(x, y, mask)) return true;
    convertInputCoords(x, y);

    if (mMediaSource)
    {
        mMediaSource->mouseUp(x, y, mask);
    }

    gFocusMgr.setMouseCapture( NULL );

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleMouseDown( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleMouseDown(x, y, mask)) return true;
    convertInputCoords(x, y);

    if (mMediaSource)
        mMediaSource->mouseDown(x, y, mask);

    gFocusMgr.setMouseCapture( this );

    if (mTakeFocusOnClick)
    {
        setFocus( true );
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleRightMouseUp( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleRightMouseUp(x, y, mask)) return true;
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

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleRightMouseDown( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleRightMouseDown(x, y, mask)) return true;

    S32 media_x = x, media_y = y;
    convertInputCoords(media_x, media_y);

    if (mMediaSource)
        mMediaSource->mouseDown(media_x, media_y, mask, 1);

    gFocusMgr.setMouseCapture( this );

    if (mTakeFocusOnClick)
    {
        setFocus( true );
    }

    auto menu = mContextMenuHandle.get();
    if (!menu)
    {
        LLUICtrl::CommitCallbackRegistry::ScopedRegistrar registar;
        registar.add("Open.WebInspector", boost::bind(&LLMediaCtrl::onOpenWebInspector, this));
        registar.add("Open.ShowSource", boost::bind(&LLMediaCtrl::onShowSource, this));

        // stinson 05/05/2014 : use this as the parent of the context menu if the static menu
        // container has yet to be created
        LLPanel* menuParent = (LLMenuGL::sMenuContainer != NULL) ? dynamic_cast<LLPanel*>(LLMenuGL::sMenuContainer) : dynamic_cast<LLPanel*>(this);
        llassert(menuParent != NULL);
        menu = LLUICtrlFactory::getInstance()->createFromFile<LLContextMenu>(
            "menu_media_ctrl.xml", menuParent, LLViewerMenuHolderGL::child_registry_t::instance());
        if (menu)
        {
            mContextMenuHandle = menu->getHandle();
        }
    }

    if (menu)
    {
        // hide/show debugging options
        bool media_plugin_debugging_enabled = gSavedSettings.getBOOL("MediaPluginDebugging");
        menu->setItemVisible("debug_separator", media_plugin_debugging_enabled);
        menu->setItemVisible("open_webinspector", media_plugin_debugging_enabled );
        menu->setItemVisible("show_page_source", media_plugin_debugging_enabled);

        menu->show(x, y);
        LLMenuGL::showPopup(this, menu, x, y);
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleDoubleClick( S32 x, S32 y, MASK mask )
{
    if (LLPanel::handleDoubleClick(x, y, mask)) return true;
    convertInputCoords(x, y);

    if (mMediaSource)
        mMediaSource->mouseDoubleClick( x, y, mask);

    gFocusMgr.setMouseCapture( this );

    if (mTakeFocusOnClick)
    {
        setFocus( true );
    }

    return true;
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
bool LLMediaCtrl::postBuild ()
{
    setVisibleCallback(boost::bind(&LLMediaCtrl::onVisibilityChanged, this, _2));

    return true;
}

void LLMediaCtrl::onOpenWebInspector()
{
    if (mMediaSource && mMediaSource->hasMedia())
        mMediaSource->getMediaPlugin()->showWebInspector( true );
}

void LLMediaCtrl::onShowSource()
{
    if (mMediaSource && mMediaSource->hasMedia())
        mMediaSource->getMediaPlugin()->showPageSource();
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleKeyHere( KEY key, MASK mask )
{
    bool result = false;

    if (mMediaSource)
    {
        result = mMediaSource->handleKeyHere(key, mask);
    }

    if ( ! result )
        result = LLPanel::handleKeyHere(key, mask);

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleKeyUpHere(KEY key, MASK mask)
{
    bool result = false;

    if (mMediaSource)
    {
        result = mMediaSource->handleKeyUpHere(key, mask);
    }

    if (!result)
        result = LLPanel::handleKeyUpHere(key, mask);

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onVisibilityChange ( bool new_visibility )
{
    LL_INFOS() << "visibility changed to " << (new_visibility?"true":"false") << LL_ENDL;
    if(mMediaSource)
    {
        mMediaSource->setVisible( new_visibility );
    }
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::handleUnicodeCharHere(llwchar uni_char)
{
    bool result = false;

    if (mMediaSource)
    {
        result = mMediaSource->handleUnicodeCharHere(uni_char);
    }

    if ( ! result )
        result = LLPanel::handleUnicodeCharHere(uni_char);

    return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::onVisibilityChanged ( const LLSD& new_visibility )
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
void LLMediaCtrl::reshape( S32 width, S32 height, bool called_from_parent )
{
    if(!getDecoupleTextureSize())
    {
        S32 screen_width = ll_round((F32)width * LLUI::getScaleFactor().mV[VX]);
        S32 screen_height = ll_round((F32)height * LLUI::getScaleFactor().mV[VY]);

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
void LLMediaCtrl::navigateStop()
{
    if (mMediaSource && mMediaSource->hasMedia())
    {
        mMediaSource->getMediaPlugin()->browse_stop();
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
void LLMediaCtrl::navigateTo( std::string url_in, std::string mime_type, bool clean_browser)
{
    // don't browse to anything that starts with secondlife:// or sl://
    const std::string protocol1 = "secondlife://";
    const std::string protocol2 = "sl://";
    if ((LLStringUtil::compareInsensitive(url_in.substr(0, protocol1.length()), protocol1) == 0) ||
        (LLStringUtil::compareInsensitive(url_in.substr(0, protocol2.length()), protocol2) == 0))
    {
        // TODO: Print out/log this attempt?
        // LL_INFOS() << "Rejecting attempt to load restricted website :" << urlIn << LL_ENDL;
        return;
    }

    if (ensureMediaSourceExists())
    {
        mCurrentNavUrl = url_in;
        mMediaSource->setSize(mTextureWidth, mTextureHeight);
        mMediaSource->navigateTo(url_in, mime_type, mime_type.empty(), false, clean_browser);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::navigateToLocalPage( const std::string& subdir, const std::string& filename_in )
{
    std::string filename(gDirUtilp->add(subdir, filename_in));
    std::string expanded_filename = gDirUtilp->findSkinnedFilename("html", filename);

    if (expanded_filename.empty())
    {
        LL_WARNS() << "File " << filename << "not found" << LL_ENDL;
        return;
    }
    if (ensureMediaSourceExists())
    {
        mCurrentNavUrl = expanded_filename;
        mMediaSource->setSize(mTextureWidth, mTextureHeight);
        mMediaSource->navigateTo(expanded_filename, HTTP_CONTENT_TEXT_HTML, false);
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
void LLMediaCtrl::setHomePageUrl( const std::string& urlIn, const std::string& mime_type )
{
    mHomePageUrl = urlIn;
    if (mMediaSource)
    {
        mMediaSource->setHomeURL(mHomePageUrl, mime_type);
    }
}

void LLMediaCtrl::setTarget(const std::string& target)
{
    mTarget = target;
    if (mMediaSource)
    {
        mMediaSource->setTarget(mTarget);
    }
}

void LLMediaCtrl::setErrorPageURL(const std::string& url)
{
    mErrorPageURL = url;
}

const std::string& LLMediaCtrl::getErrorPageURL()
{
    return mErrorPageURL;
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
    return  mHomePageUrl;
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaCtrl::ensureMediaSourceExists()
{
    if(mMediaSource.isNull())
    {
        // If we don't already have a media source, try to create one.
        mMediaSource = LLViewerMedia::getInstance()->newMediaImpl(mMediaTextureID, mTextureWidth, mTextureHeight);
        if ( mMediaSource )
        {
            mMediaSource->setUsedInUI(true);
            mMediaSource->setHomeURL(mHomePageUrl, mHomePageMimeType);
            mMediaSource->setTarget(mTarget);
            mMediaSource->setVisible( isInVisibleChain() );
            mMediaSource->addObserver( this );
            mMediaSource->setBackgroundColor( getBackgroundColor() );
            mMediaSource->setTrustedBrowser(mTrusted);

            F32 scale_factor = LLUI::getScaleFactor().mV[ VX ];
            if (scale_factor != mMediaSource->getPageZoomFactor())
            {
                mMediaSource->setPageZoomFactor( scale_factor );
                mUpdateScrolls = true;
            }

            if(mClearCache)
            {
                mMediaSource->clearCache();
                mClearCache = false;
            }
        }
        else
        {
            LL_WARNS() << "media source create failed " << LL_ENDL;
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
    F32 alpha = getDrawContext().mAlpha;

    if ( gRestoreGL == 1 || mUpdateScrolls)
    {
        LLRect r = getRect();
        reshape( r.getWidth(), r.getHeight(), false );
        mUpdateScrolls = false;
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

    LLViewerMediaTexture* media_texture = NULL;

    if (mMediaSource && mMediaSource->isTextureReady())
    {
        media_texture = LLViewerTextureManager::findMediaTexture(mMediaTextureID);
        if(media_texture)
        {
            draw_media = true;
        }
    }

    bool background_visible = isBackgroundVisible();
    bool background_opaque = isBackgroundOpaque();

    if(draw_media)
    {
        gGL.pushUIMatrix();
        {
            F32 scale_factor = LLUI::getScaleFactor().mV[ VX ];
            if (scale_factor != mMediaSource->getPageZoomFactor())
            {
                mMediaSource->setPageZoomFactor( scale_factor );
                mUpdateScrolls = true;
            }

            // scale texture to fit the space using texture coords
            gGL.getTexUnit(0)->bind(media_texture);
            LLColor4 media_color = LLColor4::white % alpha;
            gGL.color4fv( media_color.mV );
            F32 max_u = ( F32 )mMediaSource->getMediaWidth() / ( F32 )mMediaSource->getMediaTextureWidth();
            F32 max_v = ( F32 )mMediaSource->getMediaHeight() / ( F32 )mMediaSource->getMediaTextureHeight();

            S32 x_offset, y_offset, width, height;
            calcOffsetsAndSize(&x_offset, &y_offset, &width, &height);

            // draw the browser
            gGL.begin(LLRender::TRIANGLES);
            if (! mMediaSource->getMediaTextureCoordsOpenGL())
            {
                // render using web browser reported width and height, instead of trying to invert GL scale
                gGL.texCoord2f( max_u, 0.f );
                gGL.vertex2i( x_offset + width, y_offset + height );

                gGL.texCoord2f( 0.f, 0.f );
                gGL.vertex2i( x_offset, y_offset + height );

                gGL.texCoord2f( 0.f, max_v );
                gGL.vertex2i( x_offset, y_offset );

                gGL.texCoord2f(max_u, 0.f);
                gGL.vertex2i(x_offset + width, y_offset + height);

                gGL.texCoord2f(0.f, max_v);
                gGL.vertex2i(x_offset, y_offset);

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

                gGL.texCoord2f(max_u, max_v);
                gGL.vertex2i(x_offset + width, y_offset + height);

                gGL.texCoord2f(0.f, 0.f);
                gGL.vertex2i(x_offset, y_offset);

                gGL.texCoord2f( max_u, 0.f );
                gGL.vertex2i( x_offset + width, y_offset );
            }
            gGL.end();
        }
        gGL.popUIMatrix();

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
void LLMediaCtrl::calcOffsetsAndSize(S32 *x_offset, S32 *y_offset, S32 *width, S32 *height)
{
    const LLRect &r = getRect();
    *x_offset = *y_offset = 0;

    if (mStretchToFill)
    {
        // The embedded-browser backend has no intrinsic aspect ratio to preserve --
        // unlike a video plugin's fixed source resolution, a CEF browser always renders
        // at exactly the size it was last told to (see setSize()/resize()), so
        // getMediaWidth()/getMediaHeight() here is really "the size of the last frame
        // that happened to arrive," not a property of the content itself. Letterboxing
        // against it is not just unnecessary for this backend, it's actively wrong
        // whenever that value is momentarily stale relative to a just-requested resize
        // (e.g. the producer hasn't caught up yet) -- always fill the rect instead and
        // let the already-in-flight resize converge the actual browser size to match.
        if (mMaintainAspectRatio && mMediaSource && !mMediaSource->isUsingEmbeddedBrowser() &&
            mMediaSource->getMediaWidth() > 0 && mMediaSource->getMediaHeight() > 0)
        {
            F32 media_aspect = (F32)(mMediaSource->getMediaWidth()) / (F32)(mMediaSource->getMediaHeight());
            F32 view_aspect = (F32)(r.getWidth()) / (F32)(r.getHeight());
            if (media_aspect > view_aspect)
            {
                // max width, adjusted height
                *width = r.getWidth();
                *height = llmin(llmax(ll_round(*width / media_aspect), 0), r.getHeight());
            }
            else
            {
                // max height, adjusted width
                *height = r.getHeight();
                *width = llmin(llmax(ll_round(*height * media_aspect), 0), r.getWidth());
            }
        }
        else
        {
            *width = r.getWidth();
            *height = r.getHeight();
        }
    }
    else if (mMediaSource)
    {
        *width = llmin(mMediaSource->getMediaWidth(), r.getWidth());
        *height = llmin(mMediaSource->getMediaHeight(), r.getHeight());
    }
    else
    {
        *width = r.getWidth();
        *height = r.getHeight();
    }

    *x_offset = (r.getWidth() - *width) / 2;
    *y_offset = (r.getHeight() - *height) / 2;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaCtrl::convertInputCoords(S32& x, S32& y)
{
    S32 x_offset, y_offset, width, height;
    calcOffsetsAndSize(&x_offset, &y_offset, &width, &height);

    x -= x_offset;
    y -= y_offset;

    // Backend-agnostic (see LLViewerMediaImpl::getMediaTextureCoordsOpenGL()) -- going
    // through getMediaPlugin() directly (as this used to) always reads false for the
    // embedded-browser backend, since mMediaSource->getMediaPlugin() is null there by
    // construction (there's no real LLPluginClassMedia), taking the wrong branch below
    // and inverting mouse Y for every embedded-browser click/move/scroll.
    bool coords_opengl = false;

    if(mMediaSource)
    {
        coords_opengl = mMediaSource->getMediaTextureCoordsOpenGL();
    }

    F32 scale_x = LLUI::getScaleFactor().mV[VX];
    F32 scale_y = LLUI::getScaleFactor().mV[VY];

    // The embedded-browser backend's displayed content is always stretched to fill
    // (width, height) regardless of the browser's own actual current render size (see
    // calcOffsetsAndSize(), which skips aspect-ratio letterboxing for this backend) --
    // getMediaWidth()/getMediaHeight() there is "the size of the last frame that
    // happened to arrive," which can genuinely differ from the widget's current rect
    // for as long as an in-flight resize hasn't caught up yet (see
    // LLEmbeddedBrowserTab::resize()'s own comment: it doesn't update the reported
    // size until a frame published at the new size actually arrives). Scale by the
    // real ratio between the browser's own size and the displayed rect, not just the
    // UI scale factor, so a click at the edge of the DISPLAYED image lands at the edge
    // of the browser's ACTUAL content instead of drifting short of/past it by however
    // stale that gap currently is. The drift grows with distance from the origin,
    // which is why this shows up as "increasingly wrong toward the bottom" of a widget
    // rather than a constant, easily-dismissed offset.
    if (mMediaSource && mMediaSource->isUsingEmbeddedBrowser() && width > 0 && height > 0)
    {
        const S32 media_width  = mMediaSource->getMediaWidth();
        const S32 media_height = mMediaSource->getMediaHeight();
        if (media_width > 0 && media_height > 0)
        {
            scale_x = (F32)media_width  / (F32)width;
            scale_y = (F32)media_height / (F32)height;
        }
    }

    x = ll_round((F32)x * scale_x);
    if ( ! coords_opengl )
    {
        y = ll_round((F32)(y) * scale_y);
    }
    else
    {
        // height, not getRect().getHeight(): y was already shifted onto the aspect-
        // corrected content rect above (y -= y_offset), which is smaller than the full
        // widget rect whenever mMaintainAspectRatio (on by default -- see the ctor) is
        // actually centering letterboxed/pillarboxed content. Flipping against the wrong
        // (larger) height here throws Y off by exactly the centering offset -- invisible
        // when the widget's aspect ratio happens to match the media's (no centering), and
        // growing right along with the letterbox band otherwise.
        y = ll_round((F32)(height - y) * scale_y);
    };
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
            reshape( r.getWidth(), r.getHeight(), false );
        };
        break;

        case MEDIA_EVENT_CURSOR_CHANGED:
        {
            // self is nullptr for an embedded-browser-originated event (see
            // LLViewerMediaImpl::updateEmbeddedBrowserEvents()) -- it has no
            // LLPluginClassMedia to ask for a cursor name.
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << (self ? self->getCursorName() : "(embedded browser)") << LL_ENDL;
        }
        break;

        case MEDIA_EVENT_NAVIGATE_BEGIN:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_BEGIN, url is " << (self ? self->getNavigateURI() : mMediaSource->getCurrentMediaURL()) << LL_ENDL;
            hideNotification();
            mLoadingState = LOADING_STATE_LOADING;
        };
        break;

        case MEDIA_EVENT_NAVIGATE_COMPLETE:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_COMPLETE, result string is: " << (self ? self->getNavigateResultString() : "(embedded browser)") << LL_ENDL;
            if(mHidingInitialLoad)
            {
                mHidingInitialLoad = false;
            }
            mLoadingState = LOADING_STATE_LOADED;
        };
        break;

        case MEDIA_EVENT_PROGRESS_UPDATED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PROGRESS_UPDATED, loading at " << self->getProgressPercent() << "%" << LL_ENDL;
        };
        break;

        case MEDIA_EVENT_STATUS_TEXT_CHANGED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_STATUS_TEXT_CHANGED, new status text is: " << (self ? self->getStatusText() : mMediaSource->getStatusText()) << LL_ENDL;
        };
        break;

        case MEDIA_EVENT_LOCATION_CHANGED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LOCATION_CHANGED, new uri is: " << (self ? self->getLocation() : mMediaSource->getCurrentMediaURL()) << LL_ENDL;
        };
        break;

        case MEDIA_EVENT_NAVIGATE_ERROR_PAGE:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAVIGATE_ERROR_PAGE" << LL_ENDL;
            if ( mErrorPageURL.length() > 0 )
            {
                navigateTo(mErrorPageURL, HTTP_CONTENT_TEXT_HTML);
            };
            mLoadingState = LOADING_STATE_ERROR;
        };
        break;

        case MEDIA_EVENT_CLICK_LINK_HREF:
        {
            // retrieve the event parameters. self is null for an embedded-browser-originated
            // event (see LLViewerMediaImpl::updateEmbeddedBrowserEvents()) -- it has no
            // isOverrideClickTarget()/getOverrideClickTarget() override state (that's only
            // ever set on a real LLPluginClassMedia, see llfloatertos.cpp), so embedded just
            // uses its own plain click target.
            std::string url = self ? self->getClickURL() : mMediaSource->getClickURL();
            std::string target = self ? (self->isOverrideClickTarget() ? self->getOverrideClickTarget() : self->getClickTarget())
                                       : mMediaSource->getClickTarget();
            std::string uuid = self ? self->getClickUUID() : mMediaSource->getClickUUID();
            LL_DEBUGS("Media") << "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, target is \"" << target << "\", uri is " << url << LL_ENDL;

            // try as slurl first
            if (!LLURLDispatcher::dispatch(url, "clicked", NULL, mTrusted))
            {
                LLWeb::loadURL(url, target, uuid);
            }

            // CP: removing this code because we no longer support popups so this breaks the flow.
            //     replaced with a bare call to LLWeb::LoadURL(...)
            //LLNotification::Params notify_params;
            //notify_params.name = "PopupAttempt";
            //notify_params.payload = LLSD().with("target", target).with("url", url).with("uuid", uuid).with("media_id", mMediaTextureID);
            //notify_params.functor.function = boost::bind(&LLMediaCtrl::onPopup, this, _1, _2);

            //if (mTrusted)
            //{
            //  LLNotifications::instance().forceResponse(notify_params, 0);
            //}
            //else
            //{
            //  LLNotifications::instance().add(notify_params);
            //}
            break;
        };

        case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is " << (self ? self->getClickURL() : mMediaSource->getClickURL()) << LL_ENDL;
        };
        break;

        case MEDIA_EVENT_PLUGIN_FAILED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED" << LL_ENDL;
            mLoadingState = LOADING_STATE_ERROR;
        };
        break;

        case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED_LAUNCH" << LL_ENDL;
            mLoadingState = LOADING_STATE_ERROR;
        };
        break;

        case MEDIA_EVENT_NAME_CHANGED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_NAME_CHANGED" << LL_ENDL;
        };
        break;

        case MEDIA_EVENT_CLOSE_REQUEST:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_CLOSE_REQUEST" << LL_ENDL;
        }
        break;

        case MEDIA_EVENT_PICK_FILE_REQUEST:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_PICK_FILE_REQUEST" << LL_ENDL;
        }
        break;

        case MEDIA_EVENT_GEOMETRY_CHANGE:
        {
            LL_DEBUGS("Media") << "Media event:  MEDIA_EVENT_GEOMETRY_CHANGE, uuid is " << self->getClickUUID() << LL_ENDL;
        }
        break;

        case MEDIA_EVENT_AUTH_REQUEST:
        {
            LLNotification::Params auth_request_params;
            auth_request_params.name = "AuthRequest";

            // pass in host name and realm for site (may be zero length but will always exist)
            LLSD args;
            LLURL raw_url( self->getAuthURL().c_str() );
            args["HOST_NAME"] = raw_url.getAuthority();
            args["REALM"] = self->getAuthRealm();
            auth_request_params.substitutions = args;

            auth_request_params.payload = LLSD().with("media_id", mMediaTextureID);
            auth_request_params.functor.function = boost::bind(&LLViewerMedia::authSubmitCallback, _1, _2);
            LLNotifications::instance().add(auth_request_params);
        };
        break;

        case MEDIA_EVENT_LINK_HOVERED:
        {
            LL_DEBUGS("Media") <<  "Media event:  MEDIA_EVENT_LINK_HOVERED, hover text is: " << self->getHoverText() << LL_ENDL;
            mHoverTextChanged = true;
        };
        break;

        case MEDIA_EVENT_FILE_DOWNLOAD:
        {
            // self is nullptr for an embedded-browser-originated event -- see
            // LLViewerMediaImpl::getFileDownloadFilename()/respondToFileDialog().
            if (mAllowFileDownload)
            {
                // pick a file from SAVE FILE dialog
                // for now the only thing that should be allowed to save is 360s
                std::string suggested_filename = self ? self->getFileDownloadFilename() : mMediaSource->getFileDownloadFilename();
                LLFilePicker::ESaveFilter filter = LLFilePicker::FFSAVE_ALL;
                if (suggested_filename.find(".jpg") != std::string::npos || suggested_filename.find(".jpeg") != std::string::npos)
                    filter = LLFilePicker::FFSAVE_JPEG;
                if (suggested_filename.find(".png") != std::string::npos)
                    filter = LLFilePicker::FFSAVE_PNG;

                if (self)
                {
                    (new LLMediaFilePicker(self, filter, suggested_filename))->getFile();
                }
                else
                {
                    (new LLEmbeddedMediaFilePicker(mMediaSource, filter, suggested_filename))->getFile();
                }
            }
            else
            {
                // Media might be blocked, waiting for a file,
                // send an empty response to unblock it
                const std::vector<std::string> empty_response;
                if (self)
                {
                    self->sendPickFileResponse(empty_response);
                }
                else
                {
                    mMediaSource->respondToFileDialog(empty_response);
                }

                LLNotificationsUtil::add("MediaFileDownloadUnsupported");
            }
        };
        break;

        case MEDIA_EVENT_DEBUG_MESSAGE:
        {
            // self is nullptr for an embedded-browser-originated event.
            std::string debug_text = self ? self->getDebugMessageText() : mMediaSource->getDebugMessageText();
            LL_INFOS("media") << debug_text << LL_ENDL;

            // Handle text extraction responses
            if (debug_text.find(PAGE_TEXT_EXTRACT_MARKER) != std::string::npos)
            {
                if (LLPluginClassMedia* plugin = getMediaPlugin())
                {
                    // Disable plugin debugging if it was used just for text extraction
                    static LLCachedControl<bool> media_debugging(gSavedSettings, "MediaPluginDebugging", false);
                    plugin->enableMediaPluginDebugging(media_debugging);
                }
                // Extract the pump name and page text
                size_t marker_pos = debug_text.find(PAGE_TEXT_EXTRACT_MARKER);
                if (marker_pos != std::string::npos)
                {
                    std::string remaining = debug_text.substr(marker_pos + PAGE_TEXT_EXTRACT_MARKER.length());
                    size_t colon_pos = remaining.find(':');
                    if (colon_pos != std::string::npos)
                    {
                        std::string pump_name = remaining.substr(0, colon_pos);
                        std::string page_text = remaining.substr(colon_pos + 1);

                        // Send the response directly to the specified pump
                        LLEventPumps::instance().obtain(pump_name).post(LLSD().with("text", page_text));
                    }
                }
            }
        };
        break;
    };

    // chain all events to any potential observers of this object.
    emitEvent(self, event);
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLMediaCtrl::getCurrentNavUrl()
{
    return mCurrentNavUrl;
}

std::string LLMediaCtrl::getMediaName()
{
    if (mMediaSource)
        return mMediaSource->getMediaName();
    else
        return LLStringUtil::null;
}

std::string LLMediaCtrl::getStatusText()
{
    if (mMediaSource)
        return mMediaSource->getStatusText();
    else
        return LLStringUtil::null;
}

void LLMediaCtrl::onPopup(const LLSD& notification, const LLSD& response)
{
    if (response["open"])
    {
        LLWeb::loadURL(notification["payload"]["url"], notification["payload"]["target"], notification["payload"]["uuid"]);
    }
    else
    {
        // Make sure the opening instance knows its window open request was denied, so it can clean things up.
        LLViewerMedia::getInstance()->proxyWindowClosed(notification["payload"]["uuid"]);
    }
}

void LLMediaCtrl::showNotification(LLNotificationPtr notify)
{
    LLWindowShade* shade = getChild<LLWindowShade>("notification_shade");

    if (notify->getIcon() == "Popup_Caution")
    {
        shade->setBackgroundImage(LLUI::getUIImage("Yellow_Gradient"));
        shade->setTextColor(LLColor4::black);
        shade->setCanClose(true);
    }
    else if (notify->getName() == "AuthRequest")
    {
        shade->setBackgroundImage(LLUI::getUIImage("Yellow_Gradient"));
        shade->setTextColor(LLColor4::black);
        shade->setCanClose(false);
    }
    else
    {
        //HACK: make this a property of the notification itself, "cancellable"
        shade->setCanClose(false);
        shade->setTextColor(LLUIColorTable::instance().getColor("LabelTextColor"));
    }

    mWindowShade->show(notify);
}

void LLMediaCtrl::hideNotification()
{
    if (mWindowShade)
    {
        mWindowShade->hide();
    }
}

void LLMediaCtrl::setTrustedContent(bool trusted)
{
    mTrusted = trusted;
    if (mMediaSource)
    {
        mMediaSource->setTrustedBrowser(trusted);
    }
}

bool LLMediaCtrl::wantsKeyUpKeyDown() const
{
    return true;
}

bool LLMediaCtrl::wantsReturnKey() const
{
    return true;
}

std::string LLMediaCtrl::getMediaMimeType()
{
    return mMediaSource ? mMediaSource->getMimeType() : "unknown";
}

std::string LLMediaCtrl::getMediaLoadingStatus()
{
    if (!mMediaSource)
    {
        return "error";
    }

    switch (mLoadingState)
    {
        case LOADING_STATE_INITIALIZING:
            return "initializing";
        case LOADING_STATE_LOADING:
            return "loading";
        case LOADING_STATE_LOADED:
            return "loaded";
        case LOADING_STATE_ERROR:
        default:
            return "error";
    }
}

std::string LLMediaCtrl::getMediaTitle()
{
    if (mMediaSource)
    {
        if (LLPluginClassMedia* plugin = mMediaSource->getMediaPlugin())
        {
            return plugin->getMediaName();
        }
    }
    return "unknown";
}

bool LLMediaCtrl::executeJavaScript(const std::string& script)
{
    if (mMediaSource && (mMediaSource->hasMedia() || mMediaSource->isUsingEmbeddedBrowser()))
    {
        mMediaSource->executeJavaScript(script);
        return true;
    }
    return false;
}

class LLMediaCtrlListener: public LLEventAPI
{
public:
    LLMediaCtrlListener();

private:
    void getMediaInfo(const LLSD& request);
    void getMediaText(const LLSD& request);
    void replyError(const LLSD& request, const std::string& error);
    LLMediaCtrl* findMediaCtrl(const std::string& path);
};

LLMediaCtrlListener::LLMediaCtrlListener():
    LLEventAPI("LLMediaAPI", "Acces to LLMediaCtrl(web_browse widget) info")
{
    add("getMediaInfo",
        "Get information about the web_browser widget specified by [\"path\"].\n"
        "Returns URL, MIME type, and loading status of the widget.",
        &LLMediaCtrlListener::getMediaInfo,
        llsd::map("path", LLSD(), "reply", LLSD()));

    add("getMediaText",
        "Get text content from the web_browser widget specified by [\"path\"].\n"
        "Returns the text content of the page or a portion of it.",
        &LLMediaCtrlListener::getMediaText,
        llsd::map("path", LLSD(), "reply", LLSD()));
}

LLMediaCtrl* LLMediaCtrlListener::findMediaCtrl(const std::string& path)
{
    LLView* view = LLUI::getInstance()->resolvePath(LLUI::getInstance()->getRootView(), path);
    if (!view)
    {
        return nullptr;
    }
    return dynamic_cast<LLMediaCtrl*>(view);
}

void LLMediaCtrlListener::getMediaInfo(const LLSD& request)
{
    Response reply(LLSD(), request);
    std::string path = request["path"];

    LLMediaCtrl* media_ctrl = findMediaCtrl(path);
    if (!media_ctrl)
    {
        reply["error"] = "Could not find web_browser widget at path: " + path;
        return;
    }

    reply["url"] = media_ctrl->getCurrentNavUrl();
    reply["mime_type"] = media_ctrl->getMediaMimeType();
    reply["status"] = media_ctrl->getMediaLoadingStatus();
    reply["title"] = media_ctrl->getMediaTitle();
}

void LLMediaCtrlListener::replyError(const LLSD& request, const std::string& error)
{
    Response reply(LLSD(), request);
    reply["error"] = error;
}

void LLMediaCtrlListener::getMediaText(const LLSD& request)
{
    std::string path = request["path"];

    LLMediaCtrl* media_ctrl = findMediaCtrl(path);
    if (!media_ctrl)
    {
        replyError(request, "Could not find web_browser widget at path: " + path);
        return;
    }

    // Enable plugin debugging to capture console messages -- embedded browser has no
    // equivalent toggle (its console-message callback always forwards), so this only
    // applies to the legacy plugin path; executeJavaScript() below is what actually
    // gates on whether there's any media to run this against, for either backend.
    if (LLPluginClassMedia* plugin = media_ctrl->getMediaPlugin())
    {
        plugin->enableMediaPluginDebugging(true);
    }
    std::string pump_name = request["reply"].asString();

    // Execute JavaScript to extract page text, embedding pump name in the marker
    const std::string text_extract_script = "console.log('" + PAGE_TEXT_EXTRACT_MARKER + pump_name + ":' + "
                          "(document.body ? (document.body.innerText ? document.body.innerText.substring(0, 1000).replace(/\\s+/g, ' ').trim() : "
                          "'No text content') : 'Document body not ready'));";

    if (!media_ctrl->executeJavaScript(text_extract_script))
    {
        replyError(request, "Failed to execute JavaScript for text extraction");
    }
}

static LLMediaCtrlListener sMediaCtrlListener;
