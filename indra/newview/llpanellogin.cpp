/** 
 * @file llpanellogin.cpp
 * @brief Login dialog and logo display
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanellogin.h"
#include "llpanelgeneral.h"

#include "indra_constants.h"		// for key and mask constants
#include "llfontgl.h"
#include "llmd5.h"
#include "llsecondlifeurls.h"
#include "llwindow.h"			// shell_open()
#include "llversion.h"
#include "v4color.h"

#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "llviewercontrol.h"
#include "llfloaterabout.h"
#include "llfloaterpreference.h"
#include "llfocusmgr.h"
#include "lllineeditor.h"
#include "lltextbox.h"
#include "llui.h"
#include "lluiconstants.h"
#include "llviewerbuild.h"
#include "llviewerimagelist.h"
#include "llviewermenu.h"			// for handle_preferences()
#include "llviewernetwork.h"
#include "llviewerwindow.h"			// to link into child list
#include "llnotify.h"
#include "viewer.h"					// for gHideLinks
#include "llvieweruictrlfactory.h"
#include "llhttpclient.h"
#include "llweb.h"
#include "llwebbrowserctrl.h"

#include "llfloaterhtmlhelp.h"
#include "llfloatertos.h"

#include "llglheaders.h"

const S32 BLACK_BORDER_HEIGHT = 160;
const S32 MAX_PASSWORD = 16;

LLPanelLogin *LLPanelLogin::sInstance = NULL;
BOOL LLPanelLogin::sCapslockDidNotification = FALSE;

// helper class that trys to download a URL from a web site and calls a method 
// on parent class indicating if the web server is working or not
class LLIamHereLogin : public LLHTTPClient::Responder
{
	private:
		LLIamHereLogin( LLPanelLogin* parent ) :
		   mParent( parent )
		{}

		LLPanelLogin* mParent;

	public:
		static boost::intrusive_ptr< LLIamHereLogin > build( LLPanelLogin* parent )
		{
			return boost::intrusive_ptr< LLIamHereLogin >( new LLIamHereLogin( parent ) );
		};

		virtual void  setParent( LLPanelLogin* parentIn )
		{
			mParent = parentIn;
		};
		
		virtual void result( const LLSD& content )
		{
			if ( mParent )
				mParent->setSiteIsAlive( true );
		};

		virtual void error( U32 status, const std::string& reason )
		{
			if ( mParent )
				mParent->setSiteIsAlive( false );
		};
};

// this is global and not a class member to keep crud out of the header file
namespace {
	boost::intrusive_ptr< LLIamHereLogin > gResponsePtr = 0;
};

//---------------------------------------------------------------------------
// Public methods
//---------------------------------------------------------------------------
LLPanelLogin::LLPanelLogin(const LLRect &rect,
						 BOOL show_server,
						 void (*callback)(S32 option, void* user_data),
						 void *cb_data)
:	LLPanel("panel_login", LLRect(0,600,800,0), FALSE),		// not bordered
	mLogoImage(),
	mCallback(callback),
	mCallbackData(cb_data),
	mHtmlAvailable( TRUE )
{
	mIsFocusRoot = TRUE;
	mMungedPassword[0] = '\0';
	mIncomingPassword[0] = '\0';

	setBackgroundVisible(FALSE);
	setBackgroundOpaque(TRUE);

	// instance management
	if (LLPanelLogin::sInstance)
	{
		llwarns << "Duplicate instance of login view deleted" << llendl;
		delete LLPanelLogin::sInstance;
	}

	LLPanelLogin::sInstance = this;

	// add to front so we are the bottom-most child
	gViewerWindow->getRootView()->addChildAtEnd(this);

	// Logo
	mLogoImage = gImageList.getImage("startup_logo.tga", LLUUID::null, MIPMAP_FALSE, TRUE);

	gUICtrlFactory->buildPanel(this, "panel_login.xml");
	setRect(rect);
	reshape(rect.getWidth(), rect.getHeight());

	childSetPrevalidate("first_name_edit", LLLineEditor::prevalidatePrintableNoSpace);
	childSetPrevalidate("last_name_edit", LLLineEditor::prevalidatePrintableNoSpace);

	childSetCommitCallback("password_edit", mungePassword);
	childSetKeystrokeCallback("password_edit", onPassKey, this);
	childSetUserData("password_edit", this);

	LLLineEditor* edit = LLUICtrlFactory::getLineEditorByName(this, "password_edit");
	if (edit) edit->setDrawAsterixes(TRUE);

	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(this, "start_location_combo");
	if (combo)
	{
		combo->setAllowTextEntry(TRUE, 128, FALSE);

		// The XML file loads the combo with the following labels:
		// 0 - "My Home"
		// 1 - "My Last Location"
		// 2 - "<Type region name>"

		BOOL login_last = gSavedSettings.getBOOL("LoginLastLocation");
		LLString sim_string = LLURLSimString::sInstance.mSimString;
		if (!sim_string.empty())
		{
			// Replace "<Type region name>" with this region name
			combo->remove(2);
			combo->add( sim_string );
			combo->setTextEntry(sim_string);
			combo->setCurrentByIndex( 2 );
		}
		else if (login_last)
		{
			combo->setCurrentByIndex( 1 );
		}
		else
		{
			combo->setCurrentByIndex( 0 );
		}

		combo->setCommitCallback( &LLPanelGeneral::set_start_location );
	}
	
	// Specific servers added later.
	childSetVisible("server_combo", show_server);

	childSetAction("new_account_btn", onClickNewAccount, this);
	childSetVisible("new_account_btn", !gHideLinks);

	childSetAction("connect_btn", onClickConnect, this);

	setDefaultBtn("connect_btn");

	childSetAction("preferences_btn", LLFloaterPreference::show, this);

	childSetAction("quit_btn", onClickQuit, this);

	LLTextBox* text = LLUICtrlFactory::getTextBoxByName(this, "version_text");
	if (text)
	{
		LLString version = llformat("%d.%d.%d (%d)",
			LL_VERSION_MAJOR,
			LL_VERSION_MINOR,
			LL_VERSION_PATCH,
			LL_VIEWER_BUILD );
		text->setText(version);
		text->setClickedCallback(onClickVersion);
		text->setCallbackUserData(this);

		// HACK
		S32 right = getRect().mRight;
		LLRect r = text->getRect();
		const S32 PAD = 2;
		r.setOriginAndSize( right - r.getWidth() - PAD, PAD, 
			r.getWidth(), r.getHeight() );
		text->setRect(r);
	}

	// get the web browser control
	#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "login_html");
	if ( web_browser )
	{
		// don't make it a tab stop until SL-27594 is fixed
		web_browser->setTabStop(FALSE);

		// painfully build the path to the loading screen
		std::string loading_path( gDirUtilp->getExpandedFilename( LL_PATH_SKINS, "" ) );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "html" );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "loading" );
		loading_path.append( gDirUtilp->getDirDelimiter() );
		loading_path.append( "loading.html" );
		web_browser->navigateTo( loading_path.c_str() );

		// make links open in external browser
		web_browser->setOpenInExternalBrowser( true );

		// force the size to be correct (XML doesn't seem to be sufficient to do this) (with some padding so the other login screen doesn't show through)
		LLRect htmlRect = mRect;
		htmlRect.setCenterAndSize( mRect.getCenterX() - 2, mRect.getCenterY() + 40, mRect.getWidth() + 6, mRect.getHeight() - 78 );
		web_browser->setRect( htmlRect );
		web_browser->reshape( htmlRect.getWidth(), htmlRect.getHeight(), TRUE );
		reshape( mRect.getWidth(), mRect.getHeight(), 1 );

		// kick off a request to grab the url manually
		gResponsePtr = LLIamHereLogin::build( this );
		LLHTTPClient::get( childGetValue( "real_url" ).asString(), gResponsePtr );
	};
	#else
		mHtmlAvailable = FALSE;
	#endif

	// Initialize visibility (and don't force visibility - use prefs)
	refreshLocation( false );
}

void LLPanelLogin::setSiteIsAlive( bool alive )
{
#if LL_LIBXUL_ENABLED
	LLWebBrowserCtrl* web_browser = LLUICtrlFactory::getWebBrowserCtrlByName(this, "login_html");
	// if the contents of the site was retrieved
	if ( alive )
	{
		if ( web_browser )
		{
			// navigate to the "real" page 
			web_browser->navigateTo( childGetValue( "real_url" ).asString() );

			// mark as available
			mHtmlAvailable = TRUE;
		};
	}
	else
	// the site is not available (missing page, server down, other badness)
	{
		if ( web_browser )
		{
			// hide browser control (revealing default one)
			web_browser->setVisible( FALSE );

			// mark as unavailable
			mHtmlAvailable = FALSE;
		};
	};
#else
	mHtmlAvailable = FALSE;
#endif
}

void LLPanelLogin::mungePassword(LLUICtrl* caller, void* user_data)
{
	LLPanelLogin* self = (LLPanelLogin*)user_data;
	LLLineEditor* editor = (LLLineEditor*)caller;
	std::string password = editor->getText();

	// Re-md5 if we've changed at all
	if (password != self->mIncomingPassword)
	{
		LLMD5 pass((unsigned char *)password.c_str());
		pass.hex_digest(self->mMungedPassword);
	}
}

LLPanelLogin::~LLPanelLogin()
{
	LLPanelLogin::sInstance = NULL;

	// tell the responder we're not here anymore
	if ( gResponsePtr )
		gResponsePtr->setParent( 0 );

	// We know we're done with the image, so be rid of it.
	gImageList.deleteImage( mLogoImage );
}

// virtual
void LLPanelLogin::draw()
{
	if (!getVisible()) return;

	BOOL target_fullscreen;
	S32 target_width;
	S32 target_height;
	gViewerWindow->getTargetWindow(target_fullscreen, target_width, target_height);

	childSetVisible("full_screen_text", target_fullscreen);

	glPushMatrix();
	{
		F32 image_aspect = 1.333333f;
		F32 view_aspect = (F32)mRect.getWidth() / (F32)mRect.getHeight();
		// stretch image to maintain aspect ratio
		if (image_aspect > view_aspect)
		{
			glTranslatef(-0.5f * (image_aspect / view_aspect - 1.f) * mRect.getWidth(), 0.f, 0.f);
			glScalef(image_aspect / view_aspect, 1.f, 1.f);
		}
		// Don't maintain aspect ratio if screen wider than image.  This results in the
		// hand being partially cut off.  JC
		//else
		//{
		//	glTranslatef(0.f, -0.5f * (view_aspect / image_aspect - 1.f) * (F32)BLACK_BORDER_HEIGHT, 0.f);
		//	glScalef(1.f, view_aspect / image_aspect, 1.f);
		//}

		S32 width = mRect.getWidth();
		S32 height = mRect.getHeight();

		if ( mHtmlAvailable )
		{
			// draw a background box in black
			gl_rect_2d( 0, height - 264, width, 264, LLColor4( 0.0f, 0.0f, 0.0f, 1.f ) );

			// draw the bottom part of the background image - just the blue background to the native client UI
			gl_draw_scaled_image(0, -264, width + 8, mLogoImage->getHeight(), mLogoImage);
		}
		else
		{
			// the HTML login page is not available so default to the original screen
			S32 offscreen_part = height / 3;
			gl_draw_scaled_image(0, -offscreen_part, width, height+offscreen_part, mLogoImage);
		};
	}
	glPopMatrix();

	LLPanel::draw();
}

// virtual
BOOL LLPanelLogin::handleKeyHere(KEY key, MASK mask, BOOL called_from_parent)
{
	if (getVisible() && getEnabled())
	{
		if (( KEY_RETURN == key ) && (MASK_ALT == mask))
		{
			gViewerWindow->toggleFullscreen(FALSE);
			return TRUE;
		}

		if (('P' == key) && (MASK_CONTROL == mask))
		{
			LLFloaterPreference::show(NULL);
			return TRUE;
		}

		#if LL_LIBXUL_ENABLED
		if ( KEY_F1 == key )
		{
			llinfos << "Spawning HTML help window" << llendl;
			LLHtmlHelp::show( );
			return TRUE;
		};
			#if ! LL_RELEASE_FOR_DOWNLOAD
			if ( KEY_F2 == key )
			{
				llinfos << "Spawning floater TOS window" << llendl;
				LLFloaterTOS* tos_dialog = LLFloaterTOS::show(LLFloaterTOS::TOS_TOS,"");
				tos_dialog->startModal();
				return TRUE;
			};
			#endif
		#endif

		if (!called_from_parent)
		{
			if (KEY_RETURN == key && MASK_NONE == mask)
			{
				// let the panel handle UICtrl processing: calls onClickConnect()
				return LLPanel::handleKeyHere(key, mask, called_from_parent);
			}
		}
	}

	return LLPanel::handleKeyHere(key, mask, called_from_parent);
}

// virtual 
void LLPanelLogin::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			LLPanelLogin::giveFocus();
		}
		else
		{
			LLPanel::setFocus(b);
		}
	}
}

// static
void LLPanelLogin::giveFocus()
{
	if( sInstance )
	{
		// Grab focus and move cursor to first blank input field
		std::string first = sInstance->childGetText("first_name_edit");
		std::string pass = sInstance->childGetText("password_edit");

		BOOL have_first = !first.empty();
		BOOL have_pass = !pass.empty();

		LLLineEditor* edit = NULL;
		if (have_first && !have_pass)
		{
			// User saved his name but not his password.  Move
			// focus to password field.
			edit = LLUICtrlFactory::getLineEditorByName(sInstance, "password_edit");
		}
		else
		{
			// User doesn't have a name, so start there.
			edit = LLUICtrlFactory::getLineEditorByName(sInstance, "first_name_edit");
		}

		if (edit)
		{
			edit->setFocus(TRUE);
			edit->selectAll();
		}
	}
}


// static
void LLPanelLogin::show(const LLRect &rect,
						BOOL show_server,
						void (*callback)(S32 option, void* user_data),
						void* callback_data)
{
	new LLPanelLogin(rect, show_server, callback, callback_data);

	if( !gFocusMgr.getKeyboardFocus() )
	{
		// Grab focus and move cursor to first enabled control
		sInstance->setFocus(TRUE);
		// make sure that focus always goes here
		gFocusMgr.setDefaultKeyboardFocus(sInstance);
	}
}

// static
void LLPanelLogin::setFields(const std::string& firstname, const std::string& lastname, const std::string& password,
							 BOOL remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted fillFields with no login view shown" << llendl;
		return;
	}

	sInstance->childSetText("first_name_edit", firstname);
	sInstance->childSetText("last_name_edit", lastname);

	// Max "actual" password length is 16 characters.
	// Hex digests are always 32 characters.
	if (password.length() == 32)
	{
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD characters so we get a 
		// nice row of asterixes.
		const char* filler = "123456789!123456";
		sInstance->childSetText("password_edit", filler);
		strcpy(sInstance->mIncomingPassword, filler); 		/*Flawfinder: ignore*/
		strcpy(sInstance->mMungedPassword, password.c_str());	/*Flawfinder: ignore*/
	}
	else
	{
		// this is a normal text password
		sInstance->childSetText("password_edit", password);
		strncpy(sInstance->mIncomingPassword, password.c_str(), sizeof(sInstance->mIncomingPassword) -1);    /*Flawfinder: ignore*/
                sInstance->mIncomingPassword[sizeof(sInstance->mIncomingPassword) -1] = '\0';
		LLMD5 pass((unsigned char *)password.c_str());
		pass.hex_digest(sInstance->mMungedPassword);
	}

	sInstance->childSetValue("remember_check", remember);
}


// static
void LLPanelLogin::addServer(const char *server, S32 domain_name)
{
	if (!sInstance)
	{
		llwarns << "Attempted addServer with no login view shown" << llendl;
		return;
	}

	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(sInstance, "server_combo");
	if (combo)
	{
		combo->add(server, LLSD(domain_name) );
		combo->setCurrentByIndex(0);
	}
}

// static
void LLPanelLogin::getFields(LLString &firstname, LLString &lastname, LLString &password,
							BOOL &remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted getFields with no login view shown" << llendl;
		return;
	}

	firstname = sInstance->childGetText("first_name_edit");
	LLString::trim(firstname);

	lastname = sInstance->childGetText("last_name_edit");
	LLString::trim(lastname);

	password.assign(  sInstance->mMungedPassword );
	remember = sInstance->childGetValue("remember_check");
}


// static
void LLPanelLogin::getServer(LLString &server, S32 &domain_name)
{
	if (!sInstance)
	{
		llwarns << "Attempted getServer with no login view shown" << llendl;
		return;
	}

	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(sInstance, "server_combo");
	if (combo)
	{
		LLSD combo_val = combo->getValue();
		if (LLSD::TypeInteger == combo_val.type())
		{
			domain_name = combo->getValue().asInteger();

			if ((S32)USERSERVER_OTHER == domain_name)
			{
				server = gUserServerName;
			}
		}
		else
		{
			// no valid selection, return other
			domain_name = (S32)USERSERVER_OTHER;
			server = combo_val.asString();
		}
	}
}

// static
void LLPanelLogin::getLocation(LLString &location)
{
	if (!sInstance)
	{
		llwarns << "Attempted getLocation with no login view shown" << llendl;
		return;
	}
	
	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(sInstance, "start_location_combo");
	if (combo)
	{
		location = combo->getValue().asString();
	}
}

// static
void LLPanelLogin::refreshLocation( bool force_visible )
{
	if (!sInstance) return;

	LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(sInstance, "start_location_combo");
	if (!combo) return;

	LLString sim_string = LLURLSimString::sInstance.mSimString;
	if (!sim_string.empty())
	{
		combo->setCurrentByIndex( 3 );		// BUG?  Maybe 2?
		combo->setTextEntry(sim_string);
	}
	else
	{
		BOOL login_last = gSavedSettings.getBOOL("LoginLastLocation");
		combo->setCurrentByIndex( login_last ? 1 : 0 );
	}

	BOOL show_start = TRUE;

	if ( ! force_visible )
		show_start = gSavedSettings.getBOOL("ShowStartLocation");

	sInstance->childSetVisible("start_location_combo", show_start);
	sInstance->childSetVisible("start_location_text", show_start);
}

// static
void LLPanelLogin::close()
{
	if (sInstance)
	{
		gViewerWindow->getRootView()->removeChild( LLPanelLogin::sInstance );
		
		gFocusMgr.setDefaultKeyboardFocus(NULL);

		delete sInstance;
		sInstance = NULL;
	}
}


//---------------------------------------------------------------------------
// Protected methods
//---------------------------------------------------------------------------

// static
void LLPanelLogin::onClickConnect(void *)
{
	if (sInstance && sInstance->mCallback)
	{
		// tell the responder we're not here anymore
		if ( gResponsePtr )
			gResponsePtr->setParent( 0 );

		// JC - Make sure the fields all get committed.
		sInstance->setFocus(FALSE);

		LLString first = sInstance->childGetText("first_name_edit");
		LLString last  = sInstance->childGetText("last_name_edit");
		if (!first.empty() && !last.empty())
		{
			// has both first and last name typed

			// store off custom server entry, if currently selected
			LLComboBox* combo = LLUICtrlFactory::getComboBoxByName(sInstance, "server_combo");
			if (combo)
			{
				S32 selected_server = combo->getValue();
				if (selected_server == USERSERVER_NONE)
				{
					LLString custom_server = combo->getValue().asString();
					gSavedSettings.setString("CustomServer", custom_server);
				}
			}
			sInstance->mCallback(0, sInstance->mCallbackData);
		}
		else
		{
			// empty first or last name
			// same as clicking new account
			onClickNewAccount(NULL);
		}
	}
}


// static
void LLPanelLogin::newAccountAlertCallback(S32 option, void*)
{
	if (0 == option)
	{
		llinfos << "Going to account creation URL" << llendl;
		LLWeb::loadURL( CREATE_ACCOUNT_URL );
	}
	else
	{
		sInstance->setFocus(TRUE);
	}
}


// static
void LLPanelLogin::onClickNewAccount(void*)
{
	if (gHideLinks)
	{
		gViewerWindow->alertXml("MustHaveAccountToLogInNoLinks");
	}
	else
	{
		gViewerWindow->alertXml("MustHaveAccountToLogIn",
								LLPanelLogin::newAccountAlertCallback);
	}
}


// static
void LLPanelLogin::onClickQuit(void*)
{
	if (sInstance && sInstance->mCallback)
	{
		// tell the responder we're not here anymore
		if ( gResponsePtr )
			gResponsePtr->setParent( 0 );

		sInstance->mCallback(1, sInstance->mCallbackData);
	}
}


// static
void LLPanelLogin::onClickVersion(void*)
{
	LLFloaterAbout::show(NULL);
}

// static
void LLPanelLogin::onPassKey(LLLineEditor* caller, void* user_data)
{
	if (gKeyboard->getKeyDown(KEY_CAPSLOCK) && sCapslockDidNotification == FALSE)
	{
		LLNotifyBox::showXml("CapsKeyOn");
		sCapslockDidNotification = TRUE;
	}
}
