/**
 * @file LLMediaPluginTest.cpp
 * @brief Primary test application for LLMedia (Separate Process) Plugin system
 *
 * $LicenseInfo:firstyear=2008&license=viewerlgpl$
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

#ifndef LL_MEDIA_PLUGIN_TEST_H
#define LL_MEDIA_PLUGIN_TEST_H

#include <vector>
#include <string>
#include "llpluginclassmedia.h"
#include "llgl.h"

// Forward declarations
class GLUI_Rotation;
class GLUI_Translation;
class GLUI_Listbox;
class GLUI_EditText;
class GLUI_StaticText;
class GLUI;
class GLUI_Button;

////////////////////////////////////////////////////////////////////////////////
//
struct mediaPanel
{
	public:
		mediaPanel();
		~mediaPanel();
		int mId;
		std::string mStartUrl;
		std::string mMimeType;
		std::string mTarget;
		LLPluginClassMedia *mMediaSource;
		int mMediaWidth;
		int mMediaHeight;
		int mTextureWidth;
		int mTextureHeight;
		double mTextureScaleX;
		double mTextureScaleY;
		GLuint mMediaTextureHandle;
		GLuint mPickTextureHandle;
		unsigned char* mPickTexturePixels;
		bool mAppTextureCoordsOpenGL;
		bool mReadyToRender;
};

////////////////////////////////////////////////////////////////////////////////
//
class LLMediaPluginTest : public LLPluginClassMediaOwner
{
	public:
		LLMediaPluginTest( int app_window, int window_width, int window_height );
		~LLMediaPluginTest();

		void reshape( int width, int height );
		void display();
		void idle();
		void gluiCallback( int control_id );
		void keyboard( int key );
		void mousePassive( int x, int y );
		void mouseButton( int button, int state, int x, int y );
		void mouseMove( int x, int y );

		void bindTexture(GLuint texture, GLint row_length = 0, GLint alignment = 1);
		bool checkGLError(const char *name = "OpenGL");
		void drawGeometry( int panel, bool selected );
		void startPanelHighlight( float red, float green, float blue, float line_width );
		void endPanelHighlight();
		enum { DrawTypePickTexture, DrawTypeMediaTexture };
		void draw( int draw_type );
		void windowPosToTexturePos( int window_x, int window_y, int& media_x, int& media_y, int& id );

		mediaPanel* addMediaPanel( std::string url );
		void updateMediaPanel( mediaPanel* panel );
		void remMediaPanel( mediaPanel* panel );
		mediaPanel* replaceMediaPanel( mediaPanel* panel, std::string url );
		void getRandomMediaSize( int& width, int& height, std::string mime_type );
		void navigateToNewURI( std::string uri );
        void initUrlHistory( std::string uri );
		void selectPanelById( int id );
		void selectPanel( mediaPanel* panel );
		mediaPanel* findMediaPanel( LLPluginClassMedia* panel );
		mediaPanel* findMediaPanel( const std::string &target_name );
		void makePickTexture( int id, GLuint* texture_handle, unsigned char** texture_pixels );
		void makeChrome();
		void resetView();

		void dumpPanelInfo();
		void updateStatusBar();

		GLfloat distanceToCamera( GLfloat point_x, GLfloat point_y, GLfloat point_z );
		

	// Inherited from LLPluginClassMediaOwner
	/*virtual*/ void handleMediaEvent(LLPluginClassMedia* self, LLPluginClassMediaOwner::EMediaEvent);

	private:
		const int mVersionMajor;
		const int mVersionMinor;
		const int mVersionPatch;
		const int mMaxPanels;
		int mAppWindow;
		int mWindowWidth;
		int mWindowHeight;
		int mCurMouseX;
		int mCurMouseY;
		unsigned char mPixelReadColor[ 3 ];
		bool mFuzzyMedia;
		const std::string mHomeWebUrl;

		std::vector< mediaPanel* > mMediaPanels;
		mediaPanel* mSelectedPanel;
		std::string mimeTypeFromUrl( std::string& url );
		std::string pluginNameFromMimeType( std::string& mime_type );

		GLUI_Rotation* mViewRotationCtrl;
		GLUI_Translation* mViewScaleCtrl;
		GLUI_Translation* mViewTranslationCtrl;
		float mViewportAspect;
		float mViewPos[ 3 ];
		float mViewRotation[ 16 ];

		float mDistanceCameraToSelectedGeometry;

		int mIdControlAddPanel;
		int mIdControlRemPanel;

		std::vector< std::pair< std::string, std::string > > mBookmarks;
		GLUI_Listbox* mBookmarkList;
		int mIdBookmarks;
		int mIdUrlEdit;
		GLUI_EditText* mUrlEdit;
        int mIdUrlInitHistoryEdit;
		GLUI_EditText* mUrlInitHistoryEdit;
		int mSelBookmark;
		int mIdRandomPanelCount;
		int mRandomPanelCount;
		int mIdRandomBookmarks;
		int mRandomBookmarks;
		int mIdDisableTimeout;
		int mDisableTimeout;
		int mIdUsePluginReadThread;
		int mUsePluginReadThread;
		int mIdLargePanelSpacing;
		int mLargePanelSpacing;
		int mIdControlCrashPlugin;
		int mIdControlHangPlugin;
		int mIdControlExitApp;

		GLUI* mGluiMediaTimeControlWindow;
		int mIdMediaTimeControlPlay;
		int mIdMediaTimeControlLoop;
		int mIdMediaTimeControlPause;
		int mIdMediaTimeControlStop;
		int mIdMediaTimeControlSeek;
		int mIdMediaTimeControlVolume;
		int mMediaTimeControlVolume;
		int mIdMediaTimeControlSeekSeconds;
		int mMediaTimeControlSeekSeconds;
		int mIdMediaTimeControlRewind;
		int mIdMediaTimeControlFastForward;

		GLUI* mGluiMediaBrowserControlWindow;
		int mIdMediaBrowserControlBack;
		GLUI_Button* mMediaBrowserControlBackButton;
		int mIdMediaBrowserControlStop;
		int mIdMediaBrowserControlForward;
		GLUI_Button* mMediaBrowserControlForwardButton;
		bool mGluiMediaTimeControlWindowFlag;
		bool mGluiMediaBrowserControlWindowFlag;
		bool mMediaBrowserControlBackButtonFlag;
		bool mMediaBrowserControlForwardButtonFlag;
		int mIdMediaBrowserControlHome;
		int mIdMediaBrowserControlReload;
		int mIdMediaBrowserControlClearCache;
		int mIdMediaBrowserControlClearCookies;
		int mIdMediaBrowserControlEnableCookies;
		int mMediaBrowserControlEnableCookies;

		GLUI* mBottomGLUIWindow;
		GLUI_StaticText* mStatusText;
};

#endif	// LL_MEDIA_PLUGIN_TEST_H

