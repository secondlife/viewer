/**
 * @file LLMediaPluginTest.cpp
 * @brief Primary test application for LLMedia (Separate Process) Plugin system
 *
 * $LicenseInfo:firstyear=2008&license=viewergpl$
 *
 * Copyright (c) 2009, Linden Research, Inc.
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

#include "linden_common.h"
#include "indra_constants.h"

#include "llapr.h"
#include "llerrorcontrol.h"

#include <math.h>
#include <iomanip>
#include <sstream>
#include <ctime>

#include "llmediaplugintest.h"

#if __APPLE__
	#include <GLUT/glut.h>
	#include <CoreFoundation/CoreFoundation.h>
#else
	#define FREEGLUT_STATIC
	#include "GL/freeglut.h"
	#define GLUI_FREEGLUT
#endif

#if LL_WINDOWS
#pragma warning(disable: 4263)
#pragma warning(disable: 4264)
#endif
#include "glui.h"


LLMediaPluginTest* gApplication = 0;
static void gluiCallbackWrapper( int control_id );

////////////////////////////////////////////////////////////////////////////////
//
static bool isTexture( GLuint texture )
{
	bool result = false;

	// glIsTexture will sometimes return false for real textures... do this instead.
	if(texture != 0)
		result = true;

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
mediaPanel::mediaPanel()
{
	mMediaTextureHandle = 0;
	mPickTextureHandle = 0;
	mMediaSource = NULL;
	mPickTexturePixels = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
mediaPanel::~mediaPanel()
{
	// delete OpenGL texture handles
	if ( isTexture( mPickTextureHandle ) )
	{
		std::cerr << "remMediaPanel: deleting pick texture " << mPickTextureHandle << std::endl;
		glDeleteTextures( 1, &mPickTextureHandle );
		mPickTextureHandle = 0;
	}

	if ( isTexture( mMediaTextureHandle ) )
	{
		std::cerr << "remMediaPanel: deleting media texture " << mMediaTextureHandle << std::endl;
		glDeleteTextures( 1, &mMediaTextureHandle );
		mMediaTextureHandle = 0;
	}

	if(mPickTexturePixels)
	{
		delete mPickTexturePixels;
	}

	if(mMediaSource)
	{
		delete mMediaSource;
	}

}

////////////////////////////////////////////////////////////////////////////////
//
LLMediaPluginTest::LLMediaPluginTest( int app_window, int window_width, int window_height ) :
	mVersionMajor( 2 ),
	mVersionMinor( 0 ),
	mVersionPatch( 0 ),
	mMaxPanels( 25 ),
	mViewportAspect( 0 ),
	mAppWindow( app_window ),
	mCurMouseX( 0 ),
	mCurMouseY( 0 ),
	mFuzzyMedia( true ),
	mSelectedPanel( 0 ),
	mDistanceCameraToSelectedGeometry( 0.0f ),
	mMediaBrowserControlEnableCookies( 0 ),
	mMediaBrowserControlBackButton( 0 ),
	mMediaBrowserControlForwardButton( 0 ),
	mMediaTimeControlVolume( 100 ),
	mMediaTimeControlSeekSeconds( 0 ),
	mGluiMediaTimeControlWindowFlag( true ),
	mGluiMediaBrowserControlWindowFlag( true ),
	mMediaBrowserControlBackButtonFlag( true ),
	mMediaBrowserControlForwardButtonFlag( true ),
	mHomeWebUrl( "http://www.google.com/" )
{
	// debugging spam
	std::cout << std::endl << "             GLUT version: " << "3.7.6" << std::endl;	// no way to get real version from GLUT
	std::cout << std::endl << "             GLUI version: " << GLUI_Master.get_version() << std::endl;
	std::cout << std::endl << "Media Plugin Test version: " << mVersionMajor << "." << mVersionMinor << "." << mVersionPatch << std::endl;

	// bookmark title
	mBookmarks.push_back( std::pair< std::string, std::string >( "--- Bookmarks ---", "" ) );

	// insert hardcoded URLs here as required for testing
	//mBookmarks.push_back( std::pair< std::string, std::string >( "description", "url" ) );

	// read bookmarks from file.
	// note: uses command in ./CmakeLists.txt which copies bookmmarks file from source directory
	//       to app directory (WITHOUT build configuration dir) (this is cwd in Windows within MSVC)
	//		 For example, test_apps\llplugintest and not test_apps\llplugintest\Release
	//		 This may need to be changed for Mac/Linux builds.
	// See https://jira.lindenlab.com/browse/DEV-31350 for large list of media URLs from AGNI
	const std::string bookmarks_filename( "bookmarks.txt" );
	std::ifstream file_handle( bookmarks_filename.c_str() );
	if ( file_handle.is_open() )
	{
		std::cout << "Reading bookmarks for test" << std::endl;
		while( ! file_handle.eof() )
		{
			std::string line;
			std::getline( file_handle, line );
			if ( file_handle.eof() )
				break;

			if ( line.substr( 0, 1 ) != "#" )
			{
				size_t comma_pos = line.find_first_of( ',' );
				if ( comma_pos != std::string::npos )
				{
					std::string description = line.substr( 0, comma_pos );
					std::string url = line.substr( comma_pos + 1 );
					mBookmarks.push_back( std::pair< std::string, std::string >( description, url ) );
				}
				else
				{
					mBookmarks.push_back( std::pair< std::string, std::string >( line, line ) );
				};
			};
		};
		std::cout << "Read " << mBookmarks.size() << " bookmarks" << std::endl;
	}
	else
	{
		std::cout << "Unable to read bookmarks from file: " << bookmarks_filename << std::endl;
	};

	// initialize linden lab APR module
	ll_init_apr();

	// Set up llerror logging
	{
		LLError::initForApplication(".");
		LLError::setDefaultLevel(LLError::LEVEL_INFO);
		//LLError::setTagLevel("Plugin", LLError::LEVEL_DEBUG);
	}

	// lots of randomness in this app
	srand( ( unsigned int )time( 0 ) );

	// build GUI
	makeChrome();

	// OpenGL initialilzation
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClearDepth( 1.0f );
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_COLOR_MATERIAL );
	glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
	glDepthFunc( GL_LEQUAL );
	glEnable( GL_TEXTURE_2D );
	glDisable( GL_BLEND );
	glColor3f( 1.0f, 1.0f, 1.0f );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );

	// start with a sane view
	resetView();

	// initial media panel
	const int num_initial_panels = 1;
	for( int i = 0; i < num_initial_panels; ++i )
	{
		//addMediaPanel( mBookmarks[ rand() % ( mBookmarks.size() - 1 ) + 1 ].second );
		addMediaPanel( mHomeWebUrl );
	};
}

////////////////////////////////////////////////////////////////////////////////
//
LLMediaPluginTest::~LLMediaPluginTest()
{
	// delete all media panels
	for( int i = 0; i < (int)mMediaPanels.size(); ++i )
	{
		remMediaPanel( mMediaPanels[ i ] );
	};
	
	// Stop the plugin read thread if it's running.
	LLPluginProcessParent::setUseReadThread(false);
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::reshape( int width, int height )
{
	// update viewport (the active window inside the chrome)
	int viewport_x, viewport_y;
	int viewport_height, viewport_width;
	GLUI_Master.get_viewport_area( &viewport_x, &viewport_y, &viewport_width, &viewport_height );
	mViewportAspect = (float)( viewport_width ) / (float)( viewport_height );
	glViewport( viewport_x, viewport_y, viewport_width, viewport_height );

	// save these as we'll need them later
	mWindowWidth = width;
	mWindowHeight = height;

	// adjust size of URL bar so it doesn't get clipped
	mUrlEdit->set_w( mWindowWidth - 360 );

	// GLUI requires this
	if ( glutGetWindow() != mAppWindow )
		glutSetWindow( mAppWindow );

	// trigger re-display
	glutPostRedisplay();
};

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::bindTexture(GLuint texture, GLint row_length, GLint alignment)
{
	glEnable( GL_TEXTURE_2D );

	glBindTexture( GL_TEXTURE_2D, texture );
	glPixelStorei( GL_UNPACK_ROW_LENGTH, row_length );
	glPixelStorei( GL_UNPACK_ALIGNMENT, alignment );
}

////////////////////////////////////////////////////////////////////////////////
//
bool LLMediaPluginTest::checkGLError(const char *name)
{
	bool result = false;
	GLenum error = glGetError();

	if(error != GL_NO_ERROR)
	{
		// For some reason, glGenTextures is returning GL_INVALID_VALUE...
		std::cout << name << " ERROR 0x" << std::hex << error << std::dec << std::endl;
		result = true;
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
GLfloat LLMediaPluginTest::distanceToCamera( GLfloat point_x, GLfloat point_y, GLfloat point_z )
{
	GLdouble camera_pos_x = 0.0f;
	GLdouble camera_pos_y = 0.0f;
	GLdouble camera_pos_z = 0.0f;

	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	gluUnProject(
		(viewport[2]-viewport[0])/2 , (viewport[3]-viewport[1])/2,
		0.0,
		modelMatrix, projMatrix, viewport,
		&camera_pos_x, &camera_pos_y, &camera_pos_z );

	GLfloat distance =
		sqrt( ( camera_pos_x - point_x ) * ( camera_pos_x - point_x ) +
			  ( camera_pos_y - point_y ) * ( camera_pos_y - point_y ) +
			  ( camera_pos_z - point_z ) * ( camera_pos_z - point_z ) );

	return distance;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::drawGeometry( int panel, bool selected )
{
	// texture coordinates for each panel
	GLfloat non_opengl_texture_coords[ 8 ] = { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f };
	GLfloat opengl_texture_coords[ 8 ] =     { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

	GLfloat *texture_coords = mMediaPanels[ panel ]->mAppTextureCoordsOpenGL?opengl_texture_coords:non_opengl_texture_coords;

	// base coordinates for each panel
	GLfloat base_vertex_pos[ 8 ] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };

	// calculate posiitons
	const int num_panels = (int)mMediaPanels.size();
	const int num_rows = (int)sqrt( (float)num_panels );
	const int num_cols = num_panels / num_rows;
	const int panel_x = ( panel / num_rows );
	const int panel_y = ( panel % num_rows );

	// default spacing is small - make it larger if checkbox set - for testing positional audio
	float spacing = 0.1f;
	if ( mLargePanelSpacing )
		spacing = 2.0f;

	const GLfloat offset_x = num_cols * ( 1.0 + spacing ) / 2;
	const GLfloat offset_y = num_rows * ( 1.0 + spacing ) / 2;

	// Adjust for media aspect ratios
	{
		float aspect = 1.0f;

		if(mMediaPanels[ panel ]->mMediaHeight != 0)
		{
			aspect = (float)mMediaPanels[ panel ]->mMediaWidth / (float)mMediaPanels[ panel ]->mMediaHeight;
		}

		if(aspect > 1.0f)
		{
			// media is wider than it is high -- adjust the top and bottom in
			for( int corner = 0; corner < 4; ++corner )
			{
				float temp = base_vertex_pos[corner * 2 + 1];

				if(temp < 0.5f)
					temp += 0.5 - (0.5f / aspect);
				else
					temp -= 0.5 - (0.5f / aspect);

				base_vertex_pos[corner * 2 + 1] = temp;
			}
		}
		else if(aspect < 1.0f)
		{
			// media is higher than it is wide -- adjust the left and right sides in
			for( int corner = 0; corner < 4; ++corner )
			{
				float temp = base_vertex_pos[corner * 2];

				if(temp < 0.5f)
					temp += 0.5f - (0.5f * aspect);
				else
					temp -= 0.5f - (0.5f * aspect);

				base_vertex_pos[corner * 2] = temp;
			}
		}
	}

	glBegin( GL_QUADS );
	for( int corner = 0; corner < 4; ++corner )
	{
		glTexCoord2f( texture_coords[ corner * 2 ], texture_coords[ corner * 2 + 1 ] );
		GLfloat x = base_vertex_pos[ corner * 2 ] + panel_x * ( 1.0 + spacing ) - offset_x + spacing / 2.0f;
		GLfloat y = base_vertex_pos[ corner * 2 + 1 ] + panel_y * ( 1.0 + spacing ) - offset_y + spacing / 2.0f;

		glVertex3f( x, y, 0.0f );
	};
	glEnd();

	// calculate distance to this panel if it's selected
	if ( selected )
	{
		GLfloat point_x = base_vertex_pos[ 0 ] + panel_x * ( 1.0 + spacing ) - offset_x + spacing / 2.0f;
		GLfloat point_y = base_vertex_pos[ 0 + 1 ] + panel_y * ( 1.0 + spacing ) - offset_y + spacing / 2.0f;
		GLfloat point_z = 0.0f;
		mDistanceCameraToSelectedGeometry = distanceToCamera( point_x, point_y, point_z );
	};
}

//////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::startPanelHighlight( float red, float green, float blue, float line_width )
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glEnable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( -2.5f, -2.5f );
	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	glLineWidth( line_width );
	glColor3f( red, green, blue );
	glDisable( GL_TEXTURE_2D );
}

//////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::endPanelHighlight()
{
	glPopAttrib();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::draw( int draw_type )
{
	for( int panel = 0; panel < (int)mMediaPanels.size(); ++panel )
	{
		// drawing pick texture
		if ( draw_type == DrawTypePickTexture )
		{
			// only bother with pick if we have something to render
			// Actually, we need to pick even if we're not ready to render.
			// Otherwise you can't select and remove a panel which has gone bad.
			//if ( mMediaPanels[ panel ]->mReadyToRender )
			{
				glMatrixMode( GL_TEXTURE );
				glPushMatrix();

				// pick texture is a power of 2 so no need to scale
				glLoadIdentity();

				// bind to media texture
				glLoadIdentity();
				bindTexture( mMediaPanels[ panel ]->mPickTextureHandle );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

				// draw geometry using pick texture
				drawGeometry( panel, false );

				glMatrixMode( GL_TEXTURE );
				glPopMatrix();
			};
		}
		else
		if ( draw_type == DrawTypeMediaTexture )
		{
			bool texture_valid = false;
			bool plugin_exited = false;

			if(mMediaPanels[ panel ]->mMediaSource)
			{
				texture_valid = mMediaPanels[ panel ]->mMediaSource->textureValid();
				plugin_exited = mMediaPanels[ panel ]->mMediaSource->isPluginExited();
			}

			// save texture matrix (changes for each panel)
			glMatrixMode( GL_TEXTURE );
			glPushMatrix();

			// only process texture if the media is ready to draw
			// (we still want to draw the geometry)
			if ( mMediaPanels[ panel ]->mReadyToRender && texture_valid )
			{
				// bind to media texture
				bindTexture( mMediaPanels[ panel ]->mMediaTextureHandle );

				if ( mFuzzyMedia )
				{
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				}
				else
				{
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
					glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				}

				// scale to fit panel
				glScalef( mMediaPanels[ panel ]->mTextureScaleX,
							mMediaPanels[ panel ]->mTextureScaleY,
								1.0f );
			};

			float intensity = plugin_exited?0.25f:1.0f;

			// highlight the selected panel
			if ( mSelectedPanel && ( mMediaPanels[ panel ]->mId == mSelectedPanel->mId ) )
			{
				startPanelHighlight( intensity, intensity, 0.0f, 5.0f );
				drawGeometry( panel, true );
				endPanelHighlight();
			}
			else
			// this panel not able to render yet since it
			// doesn't have enough information
			if ( !mMediaPanels[ panel ]->mReadyToRender )
			{
				startPanelHighlight( intensity, 0.0f, 0.0f, 2.0f );
				drawGeometry( panel, false );
				endPanelHighlight();
			}
			else
			// just display a border around the media
			{
				startPanelHighlight( 0.0f, intensity, 0.0f, 2.0f );
				drawGeometry( panel, false );
				endPanelHighlight();
			};

			if ( mMediaPanels[ panel ]->mReadyToRender && texture_valid )
			{
				// draw visual geometry
				drawGeometry( panel, false );
			}

			// restore texture matrix (changes for each panel)
			glMatrixMode( GL_TEXTURE );
			glPopMatrix();
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::display()
{
	// GLUI requires this
	if ( glutGetWindow() != mAppWindow )
		glutSetWindow( mAppWindow );

	// start with a clean slate
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// set up OpenGL view
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glFrustum( -mViewportAspect * 0.04f, mViewportAspect * 0.04f, -0.04f, 0.04f, 0.1f, 50.0f );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glTranslatef( 0.0, 0.0, 0.0f );
	glTranslatef( mViewPos[ 0 ], mViewPos[ 1 ], -mViewPos[ 2 ] );
	glMultMatrixf( mViewRotation );

	// draw pick texture
	draw( DrawTypePickTexture );

	// read colors and get coordinate values
	glReadPixels( mCurMouseX, mCurMouseY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, mPixelReadColor );

	// clear the pick render (otherwise it may depth-fight with the textures rendered later)
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// draw visible geometry
	draw( DrawTypeMediaTexture );

	glutSwapBuffers();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::idle()
{
//	checkGLError("LLMediaPluginTest::idle");

	// GLUI requires this
	if ( glutGetWindow() != mAppWindow )
		glutSetWindow( mAppWindow );

	// random creation/destruction of panels enabled?
	const time_t panel_timeout_time = 5;
	if ( mRandomPanelCount )
	{
		// time for a change
		static time_t last_panel_time = 0;
		if ( time( NULL ) - last_panel_time > panel_timeout_time )
		{
			if ( rand() % 2 == 0 )
			{
				if ( mMediaPanels.size() < 16 )
				{
					std::cout << "Randomly adding new panel" << std::endl;
					addMediaPanel( mBookmarks[ rand() % ( mBookmarks.size() - 1 ) + 1 ].second );
				};
			}
			else
			{
				if ( mMediaPanels.size() > 0 )
				{
					std::cout << "Deleting selected panel" << std::endl;
					remMediaPanel( mSelectedPanel );
				};
			};
			time( &last_panel_time );
		};
	};

	// random selection of bookmarks enabled?
	const time_t bookmark_timeout_time = 5;
	if ( mRandomBookmarks )
	{
		// time for a change
		static time_t last_bookmark_time = 0;
		if ( time( NULL ) - last_bookmark_time > bookmark_timeout_time )
		{
			// go to a different random bookmark on each panel
			for( int panel = 0; panel < (int)mMediaPanels.size(); ++panel )
			{
				std::string uri = mBookmarks[ rand() % ( mBookmarks.size() - 1 ) + 1 ].second;

				std::cout << "Random: navigating to : " << uri << std::endl;

				std::string mime_type = mimeTypeFromUrl( uri );

				if ( mime_type != mMediaPanels[ panel ]->mMimeType )
				{
					replaceMediaPanel( mMediaPanels[ panel ], uri );
				}
				else
				{
					mMediaPanels[ panel ]->mMediaSource->loadURI( uri );
					mMediaPanels[ panel ]->mMediaSource->start();
				};
			};

			time( &last_bookmark_time );
		};
	};

	// update UI
	if ( mSelectedPanel )
	{
		// set volume based on slider if we have time media
		//if ( mGluiMediaTimeControlWindowFlag )
		//{
		//	mSelectedPanel->mMediaSource->setVolume( (float)mMediaTimeControlVolume / 100.0f );
		//};

		// NOTE: it is absurd that we need cache the state of GLUI controls
		//       but enabling/disabling controls drags framerate from 500+
		//		 down to 15. Not a problem for plugin system - only this test
		// enable/disable time based UI controls based on type of plugin
		if ( mSelectedPanel->mMediaSource->pluginSupportsMediaTime() )
		{
			if ( ! mGluiMediaTimeControlWindowFlag )
			{
				mGluiMediaTimeControlWindow->enable();
				mGluiMediaTimeControlWindowFlag = true;
			};
		}
		else
		{
			if ( mGluiMediaTimeControlWindowFlag )
			{
				mGluiMediaTimeControlWindow->disable();
				mGluiMediaTimeControlWindowFlag = false;
			};
		};

		// enable/disable browser based UI controls based on type of plugin
		if ( mSelectedPanel->mMediaSource->pluginSupportsMediaBrowser() )
		{
			if ( ! mGluiMediaBrowserControlWindowFlag )
			{
				mGluiMediaBrowserControlWindow->enable();
				mGluiMediaBrowserControlWindowFlag = true;
			};
		}
		else
		{
			if ( mGluiMediaBrowserControlWindowFlag )
			{
				mGluiMediaBrowserControlWindow->disable();
				mGluiMediaBrowserControlWindowFlag = false;
			};
		};

		// enable/disable browser back button depending on browser history
		if ( mSelectedPanel->mMediaSource->getHistoryBackAvailable()  )
		{
			if ( ! mMediaBrowserControlBackButtonFlag )
			{
				mMediaBrowserControlBackButton->enable();
				mMediaBrowserControlBackButtonFlag = true;
			};
		}
		else
		{
			if ( mMediaBrowserControlBackButtonFlag )
			{
				mMediaBrowserControlBackButton->disable();
				mMediaBrowserControlBackButtonFlag = false;
			};
		};

		// enable/disable browser forward button depending on browser history
		if ( mSelectedPanel->mMediaSource->getHistoryForwardAvailable()  )
		{
			if ( ! mMediaBrowserControlForwardButtonFlag )
			{
				mMediaBrowserControlForwardButton->enable();
				mMediaBrowserControlForwardButtonFlag = true;
			};
		}
		else
		{
			if ( mMediaBrowserControlForwardButtonFlag )
			{
				mMediaBrowserControlForwardButton->disable();
				mMediaBrowserControlForwardButtonFlag = false;
			};
		};

		// NOTE: This is *very* slow and not worth optimising
		updateStatusBar();
	};

	// update all the panels
	for( int panel_index = 0; panel_index < (int)mMediaPanels.size(); ++panel_index )
	{
		mediaPanel *panel = mMediaPanels[ panel_index ];

		// call plugins idle function so it can potentially update itself
		panel->mMediaSource->idle();

		// update each media panel
		updateMediaPanel( panel );

		LLRect dirty_rect;
		if ( ! panel->mMediaSource->textureValid() )
		{
			//std::cout << "texture invalid, skipping update..." << std::endl;
		}
		else
		if ( panel &&
			 ( panel->mMediaWidth != panel->mMediaSource->getWidth() ||
			   panel->mMediaHeight != panel->mMediaSource->getHeight() ) )
		{
			//std::cout << "Resize in progress, skipping update..." << std::endl;
		}
		else
		if ( panel->mMediaSource->getDirty( &dirty_rect ) )
		{
			const unsigned char* pixels = panel->mMediaSource->getBitsData();
			if ( pixels && isTexture(panel->mMediaTextureHandle))
			{
				int x_offset = dirty_rect.mLeft;
				int y_offset = dirty_rect.mBottom;
				int width = dirty_rect.mRight - dirty_rect.mLeft;
				int height = dirty_rect.mTop - dirty_rect.mBottom;

				if((dirty_rect.mRight <= panel->mTextureWidth) && (dirty_rect.mTop <= panel->mTextureHeight))
				{
					// Offset the pixels pointer properly
					pixels += ( y_offset * panel->mMediaSource->getTextureDepth() * panel->mMediaSource->getBitsWidth() );
					pixels += ( x_offset * panel->mMediaSource->getTextureDepth() );

					// set up texture
					bindTexture( panel->mMediaTextureHandle, panel->mMediaSource->getBitsWidth() );
					if ( mFuzzyMedia )
					{
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
					}
					else
					{
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
					};

					checkGLError("glTexParameteri");

					if(panel->mMediaSource->getTextureFormatSwapBytes())
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);
						checkGLError("glPixelStorei");
					}

					// draw portion that changes into texture
					glTexSubImage2D( GL_TEXTURE_2D, 0,
						x_offset,
						y_offset,
						width,
						height,
						panel->mMediaSource->getTextureFormatPrimary(),
						panel->mMediaSource->getTextureFormatType(),
						pixels );

					if(checkGLError("glTexSubImage2D"))
					{
						std::cerr << "    panel ID=" << panel->mId << std::endl;
						std::cerr << "    texture size = " << panel->mTextureWidth << " x " << panel->mTextureHeight << std::endl;
						std::cerr << "    media size = " << panel->mMediaWidth << " x " << panel->mMediaHeight << std::endl;
						std::cerr << "    dirty rect = " << dirty_rect.mLeft << ", " << dirty_rect.mBottom << ", " << dirty_rect.mRight << ", " << dirty_rect.mTop << std::endl;
						std::cerr << "    texture width = " << panel->mMediaSource->getBitsWidth() << std::endl;
						std::cerr << "    format primary = 0x" << std::hex << panel->mMediaSource->getTextureFormatPrimary() << std::dec << std::endl;
						std::cerr << "    format type = 0x" << std::hex << panel->mMediaSource->getTextureFormatType() << std::dec << std::endl;
						std::cerr << "    pixels = " << (void*)pixels << std::endl;
					}

					if(panel->mMediaSource->getTextureFormatSwapBytes())
					{
						glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
						checkGLError("glPixelStorei");
					}

					panel->mMediaSource->resetDirty();

					panel->mReadyToRender = true;
				}
				else
				{
					std::cerr << "dirty rect is outside current media size, skipping update" << std::endl;
				}
			};
		};
	};

	// GLUI requires this
	if ( glutGetWindow() != mAppWindow )
		glutSetWindow( mAppWindow );

	// trigger re-display
	glutPostRedisplay();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::windowPosToTexturePos( int window_x, int window_y,
											   int& media_x, int& media_y,
											   int& id )
{
	if ( ! mSelectedPanel )
	{
		media_x = 0;
		media_y = 0;
		id = 0;
		return;
	};

	// record cursor poisiton for a readback next frame
	mCurMouseX = window_x;
	// OpenGL app == coordinate system this way
	// NOTE: unrelated to settings in plugin - this
	// is just for this app
	mCurMouseY = mWindowHeight - window_y;

	// extract x (0..1023, y (0..1023) and id (0..15) from RGB components
	unsigned long pixel_read_color_bits = ( mPixelReadColor[ 0 ] << 16 ) | ( mPixelReadColor[ 1 ] << 8 ) | mPixelReadColor[ 2 ];
	int texture_x = pixel_read_color_bits & 0x3ff;
	int texture_y = ( pixel_read_color_bits >> 10 ) & 0x3ff;
	id = ( pixel_read_color_bits >> 20 ) & 0x0f;

	// scale to size of media (1024 because we use 10 bits for X and Y from 24)
	media_x = (int)( ( (float)mSelectedPanel->mMediaWidth * (float)texture_x ) / 1024.0f );
	media_y = (int)( ( (float)mSelectedPanel->mMediaHeight * (float)texture_y ) / 1024.0f );

	// we assume the plugin uses an inverted coordinate scheme like OpenGL
	// if not, the plugin code inverts the Y coordinate for us - we don't need to
	media_y = mSelectedPanel->mMediaHeight - media_y;

	if ( media_x > 0 && media_y > 0 )
	{
		//std::cout << "      mouse coords: " << mCurMouseX << " x " << mCurMouseY << " and id = " << id  << std::endl;
		//std::cout << "raw texture coords: " << texture_x << " x " << texture_y << " and id = " << id  << std::endl;
		//std::cout << "      media coords: " << media_x << " x " << media_y << " and id = " << id  << std::endl;
		//std::cout << std::endl;
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::selectPanelById( int id )
{
	for( int panel = 0; panel < (int)mMediaPanels.size(); ++panel )
	{
		if ( mMediaPanels[ panel ]->mId == id )
		{
			selectPanel(mMediaPanels[ panel ]);
			return;
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::selectPanel( mediaPanel* panel )
{
	if( mSelectedPanel == panel )
		return;

	// turn off volume before we delete it
	if( mSelectedPanel && mSelectedPanel->mMediaSource )
	{
		mSelectedPanel->mMediaSource->setVolume( 0.0f );
		mSelectedPanel->mMediaSource->setPriority( LLPluginClassMedia::PRIORITY_LOW );
	};

	mSelectedPanel = panel;

	if( mSelectedPanel && mSelectedPanel->mMediaSource )
	{
		mSelectedPanel->mMediaSource->setVolume( (float)mMediaTimeControlVolume / 100.0f );
		mSelectedPanel->mMediaSource->setPriority( LLPluginClassMedia::PRIORITY_NORMAL );

		if(!mSelectedPanel->mStartUrl.empty())
		{
			mUrlEdit->set_text(const_cast<char*>(mSelectedPanel->mStartUrl.c_str()) );
		}
	};
}

////////////////////////////////////////////////////////////////////////////////
//
mediaPanel*  LLMediaPluginTest::findMediaPanel( LLPluginClassMedia* source )
{
	mediaPanel *result = NULL;

	for( int panel = 0; panel < (int)mMediaPanels.size(); ++panel )
	{
		if ( mMediaPanels[ panel ]->mMediaSource == source )
		{
			result = mMediaPanels[ panel ];
		}
	}

	return result;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::navigateToNewURI( std::string uri )
{
	if ( uri.length() )
	{
		std::string mime_type = mimeTypeFromUrl( uri );

		if ( !mSelectedPanel->mMediaSource->isPluginExited() && (mime_type == mSelectedPanel->mMimeType) )
		{
			std::cout << "MIME type is the same" << std::endl;
			mSelectedPanel->mMediaSource->loadURI( uri );
			mSelectedPanel->mMediaSource->start();
			mBookmarkList->do_selection( 0 );
		}
		else
		{
			std::cout << "MIME type changed or plugin had exited" << std::endl;
			replaceMediaPanel( mSelectedPanel, uri );
			mBookmarkList->do_selection( 0 );
		}
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::initUrlHistory( std::string uris )
{
	if ( uris.length() > 0 )
	{
		std::cout << "init URL : " << uris << std::endl;
		LLSD historySD;

		char *cstr, *p;
		cstr = new char[uris.size()+1];
		strcpy(cstr, uris.c_str());
		const char *DELIMS = " ,;";
		p = strtok(cstr, DELIMS);
		while (p != NULL) {
			historySD.insert(0, p);
			p = strtok(NULL, DELIMS);
		}
		mSelectedPanel->mMediaSource->initializeUrlHistory(historySD);
		delete[] cstr;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::gluiCallback( int control_id )
{
	if ( control_id == mIdBookmarks )
	{
		std::string uri = mBookmarks[ mSelBookmark ].second;

		navigateToNewURI( uri );
	}
	else
    if ( control_id == mIdUrlEdit)
	{
		std::string uri = mUrlEdit->get_text();

		navigateToNewURI( uri );
	}
	else
	if ( control_id == mIdUrlInitHistoryEdit )
	{
		std::string uri = mUrlInitHistoryEdit->get_text();

		initUrlHistory( uri );
	}
	else
	if ( control_id == mIdControlAddPanel )
	{
		addMediaPanel( mBookmarks[ rand() % ( mBookmarks.size() - 1 ) + 1 ].second );
	}
	else
	if ( control_id == mIdControlRemPanel )
	{
		remMediaPanel( mSelectedPanel );
	}
	else
	if ( control_id == mIdDisableTimeout )
	{
		// Set the "disable timeout" flag for all active plugins.
		for( int i = 0; i < (int)mMediaPanels.size(); ++i )
		{
			mMediaPanels[ i ]->mMediaSource->setDisableTimeout(mDisableTimeout);
		}
	}
	else
	if ( control_id == mIdUsePluginReadThread )
	{
		LLPluginProcessParent::setUseReadThread(mUsePluginReadThread);
	}
	else
	if ( control_id == mIdControlCrashPlugin )
	{
		// send message to plugin and ask it to crash
		// (switch out for ReleaseCandidate version :) )
		if(mSelectedPanel && mSelectedPanel->mMediaSource)
		{
			mSelectedPanel->mMediaSource->crashPlugin();
		}
	}
	else
	if ( control_id == mIdControlHangPlugin )
	{
		// send message to plugin and ask it to hang
		// (switch out for ReleaseCandidate version :) )
		if(mSelectedPanel && mSelectedPanel->mMediaSource)
		{
			mSelectedPanel->mMediaSource->hangPlugin();
		}
	}
	else
	if ( control_id == mIdControlExitApp )
	{
		// text for exiting plugin system cleanly
		delete this;	// clean up
		exit( 0 );
	}
	else
	if ( control_id == mIdMediaTimeControlPlay )
	{
		if ( mSelectedPanel )
		{
			mSelectedPanel->mMediaSource->setLoop( false );
			mSelectedPanel->mMediaSource->start();
		};
	}
	else
	if ( control_id == mIdMediaTimeControlLoop )
	{
		if ( mSelectedPanel )
		{
			mSelectedPanel->mMediaSource->setLoop( true );
			mSelectedPanel->mMediaSource->start();
		};
	}
	else
	if ( control_id == mIdMediaTimeControlPause )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->pause();
	}
	else
	if ( control_id == mIdMediaTimeControlStop )
	{
		if ( mSelectedPanel )
		{
			mSelectedPanel->mMediaSource->stop();
		};
	}
	else
	if ( control_id == mIdMediaTimeControlSeek )
	{
		if ( mSelectedPanel )
		{
			// get value from spinner
			float seconds_to_seek = mMediaTimeControlSeekSeconds;
			mSelectedPanel->mMediaSource->seek( seconds_to_seek );
			mSelectedPanel->mMediaSource->start();
		};
	}
	else
	if ( control_id == mIdMediaTimeControlRewind )
	{
		if ( mSelectedPanel )
		{
			mSelectedPanel->mMediaSource->setLoop( false );
			mSelectedPanel->mMediaSource->start(-2.0f);
		};
	}
	else
	if ( control_id == mIdMediaTimeControlFastForward )
	{
		if ( mSelectedPanel )
		{
			mSelectedPanel->mMediaSource->setLoop( false );
			mSelectedPanel->mMediaSource->start(2.0f);
		};
	}
	else
	if ( control_id == mIdMediaBrowserControlBack )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->browse_back();
	}
	else
	if ( control_id == mIdMediaBrowserControlStop )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->browse_stop();
	}
	else
	if ( control_id == mIdMediaBrowserControlForward )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->browse_forward();
	}
	else
	if ( control_id == mIdMediaBrowserControlHome )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->loadURI( mHomeWebUrl );
	}
	else
	if ( control_id == mIdMediaBrowserControlReload )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->browse_reload( true );
	}
	else
	if ( control_id == mIdMediaBrowserControlClearCache )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->clear_cache();
	}
	else
	if ( control_id == mIdMediaBrowserControlClearCookies )
	{
		if ( mSelectedPanel )
			mSelectedPanel->mMediaSource->clear_cookies();
	}
	else
	if ( control_id == mIdMediaBrowserControlEnableCookies )
	{
		if ( mSelectedPanel )
		{
			if ( mMediaBrowserControlEnableCookies )
			{
				mSelectedPanel->mMediaSource->enable_cookies( true );
			}
			else
			{
				mSelectedPanel->mMediaSource->enable_cookies( false );
			}
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::keyboard( int key )
{
	//if ( key == 'a' || key == 'A' )
	//	addMediaPanel( mBookmarks[ rand() % ( mBookmarks.size() - 1 ) + 1 ].second );
	//else
	//if ( key == 'r' || key == 'R' )
	//	remMediaPanel( mSelectedPanel );
	//else
	//if ( key == 'd' || key == 'D' )
	//	dumpPanelInfo();
	//else
	if ( key == 27 )
	{
		std::cout << "Application finished - exiting..." << std::endl;
		delete this;
		exit( 0 );
	};

	mSelectedPanel->mMediaSource->keyEvent( LLPluginClassMedia::KEY_EVENT_DOWN, key, 0 , LLSD());
	mSelectedPanel->mMediaSource->keyEvent( LLPluginClassMedia::KEY_EVENT_UP, key, 0, LLSD());
};

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::mouseButton( int button, int state, int x, int y )
{
	if ( button == GLUT_LEFT_BUTTON )
	{
		if ( state == GLUT_DOWN )
		{
			int media_x, media_y, id;
			windowPosToTexturePos( x, y, media_x, media_y, id );

			if ( mSelectedPanel )
				mSelectedPanel->mMediaSource->mouseEvent( LLPluginClassMedia::MOUSE_EVENT_DOWN, 0, media_x, media_y, 0 );
		}
		else
		if ( state == GLUT_UP )
		{
			int media_x, media_y, id;
			windowPosToTexturePos( x, y, media_x, media_y, id );

			// only select a panel if we're on a panel
			// (HACK: strictly speaking this rules out clicking on
			// the origin of a panel but that's very unlikely)
			if ( media_x > 0 && media_y > 0 )
			{
				selectPanelById( id );

				if ( mSelectedPanel )
					mSelectedPanel->mMediaSource->mouseEvent( LLPluginClassMedia::MOUSE_EVENT_UP, 0, media_x, media_y, 0 );
			};
		};
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::mousePassive( int x, int y )
{
	int media_x, media_y, id;
	windowPosToTexturePos( x, y, media_x, media_y, id );

	if ( mSelectedPanel )
		mSelectedPanel->mMediaSource->mouseEvent( LLPluginClassMedia::MOUSE_EVENT_MOVE, 0, media_x, media_y, 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::mouseMove( int x, int y )
{
	int media_x, media_y, id;
	windowPosToTexturePos( x, y, media_x, media_y, id );

	if ( mSelectedPanel )
		mSelectedPanel->mMediaSource->mouseEvent( LLPluginClassMedia::MOUSE_EVENT_MOVE, 0, media_x, media_y, 0 );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::makeChrome()
{
	// IDs used by GLUI
	int start_id = 0x1000;

	// right side window - geometry manipulators
#if __APPLE__
	// the Apple GLUT implementation doesn't seem to set the graphic offset of subwindows correctly when they overlap in certain ways.
	// Use a separate controls window in this case.
	// GLUI window at right containing manipulation controls and other buttons
	int x = glutGet(GLUT_WINDOW_X) + glutGet(GLUT_WINDOW_WIDTH) + 4;
	int y = glutGet(GLUT_WINDOW_Y);
	GLUI* right_glui_window = GLUI_Master.create_glui( "", 0, x, y );
#else
	GLUI* right_glui_window = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_RIGHT );
#endif
	mViewRotationCtrl = right_glui_window->add_rotation( "Rotation", mViewRotation );
	mViewTranslationCtrl = right_glui_window->add_translation( "Translate", GLUI_TRANSLATION_XY, mViewPos );
	mViewTranslationCtrl->set_speed( 0.01f );
	mViewScaleCtrl = right_glui_window->add_translation( "Scale", GLUI_TRANSLATION_Z, &mViewPos[ 2 ] );
	mViewScaleCtrl->set_speed( 0.05f );
	right_glui_window->set_main_gfx_window( mAppWindow );

	// right side window - app controls
	mIdControlAddPanel = start_id++;
	right_glui_window->add_statictext( "" );
	right_glui_window->add_separator();
	right_glui_window->add_statictext( "" );
	right_glui_window->add_button( "Add panel", mIdControlAddPanel, gluiCallbackWrapper );
	right_glui_window->add_statictext( "" );
	mIdControlRemPanel = start_id++;
	right_glui_window->add_button( "Rem panel", mIdControlRemPanel, gluiCallbackWrapper );
	right_glui_window->add_statictext( "" );
	right_glui_window->add_separator();
	right_glui_window->add_statictext( "" );
	mIdControlCrashPlugin = start_id++;
	right_glui_window->add_button( "Crash plugin", mIdControlCrashPlugin, gluiCallbackWrapper );
	mIdControlHangPlugin = start_id++;
	right_glui_window->add_button( "Hang plugin", mIdControlHangPlugin, gluiCallbackWrapper );

	right_glui_window->add_statictext( "" );
	right_glui_window->add_separator();
	right_glui_window->add_statictext( "" );
	mIdControlExitApp = start_id++;
	right_glui_window->add_button( "Exit app", mIdControlExitApp, gluiCallbackWrapper );

	//// top window - holds bookmark UI
	mIdBookmarks = start_id++;
	mSelBookmark = 0;
	GLUI* glui_window_top = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );
	mBookmarkList = glui_window_top->add_listbox( "", &mSelBookmark, mIdBookmarks, gluiCallbackWrapper );
	// only add the first 50 bookmarks - list can be very long sometimes (30,000+)
	// when testing list of media URLs from AGNI for example
	for( unsigned int each = 0; each < mBookmarks.size() && each < 50; ++each )
		mBookmarkList->add_item( each, const_cast< char* >( mBookmarks[ each ].first.c_str() ) );
	glui_window_top->set_main_gfx_window( mAppWindow );

	glui_window_top->add_column( false );
	mIdUrlEdit = start_id++;
	mUrlEdit = glui_window_top->add_edittext( "Url:", GLUI_EDITTEXT_TEXT, 0, mIdUrlEdit, gluiCallbackWrapper );
	mUrlEdit->set_w( 600 );
	GLUI* glui_window_top2 = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );
	mIdUrlInitHistoryEdit = start_id++;
	mUrlInitHistoryEdit = glui_window_top2->add_edittext( "Init History (separate by commas or semicolons):",
		GLUI_EDITTEXT_TEXT, 0, mIdUrlInitHistoryEdit, gluiCallbackWrapper );
	mUrlInitHistoryEdit->set_w( 800 );

	// top window - media controls for "time" media types (e.g. movies)
	mGluiMediaTimeControlWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );
	mGluiMediaTimeControlWindow->set_main_gfx_window( mAppWindow );
	mIdMediaTimeControlPlay = start_id++;
	mGluiMediaTimeControlWindow->add_button( "PLAY", mIdMediaTimeControlPlay, gluiCallbackWrapper );
	mGluiMediaTimeControlWindow->add_column( false );
	mIdMediaTimeControlLoop = start_id++;
	mGluiMediaTimeControlWindow->add_button( "LOOP", mIdMediaTimeControlLoop, gluiCallbackWrapper );
	mGluiMediaTimeControlWindow->add_column( false );
	mIdMediaTimeControlPause = start_id++;
	mGluiMediaTimeControlWindow->add_button( "PAUSE", mIdMediaTimeControlPause, gluiCallbackWrapper );
	mGluiMediaTimeControlWindow->add_column( false );

	GLUI_Button  *button;
	mIdMediaTimeControlRewind = start_id++;
	button = mGluiMediaTimeControlWindow->add_button( "<<", mIdMediaTimeControlRewind, gluiCallbackWrapper );
	button->set_w(30);
	mGluiMediaTimeControlWindow->add_column( false );
	mIdMediaTimeControlFastForward = start_id++;
	button = mGluiMediaTimeControlWindow->add_button( ">>", mIdMediaTimeControlFastForward, gluiCallbackWrapper );
	button->set_w(30);

	mGluiMediaTimeControlWindow->add_column( true );

	mIdMediaTimeControlStop = start_id++;
	mGluiMediaTimeControlWindow->add_button( "STOP", mIdMediaTimeControlStop, gluiCallbackWrapper );
	mGluiMediaTimeControlWindow->add_column( false );
	mIdMediaTimeControlVolume = start_id++;
	GLUI_Spinner* spinner = mGluiMediaTimeControlWindow->add_spinner( "Volume", 2, &mMediaTimeControlVolume, mIdMediaTimeControlVolume, gluiCallbackWrapper);
	spinner->set_float_limits( 0, 100 );
	mGluiMediaTimeControlWindow->add_column( true );
	mIdMediaTimeControlSeekSeconds = start_id++;
	spinner = mGluiMediaTimeControlWindow->add_spinner( "", 2, &mMediaTimeControlSeekSeconds, mIdMediaTimeControlSeekSeconds, gluiCallbackWrapper);
	spinner->set_float_limits( 0, 200 );
	spinner->set_w( 32 );
	spinner->set_speed( 0.025f );
	mGluiMediaTimeControlWindow->add_column( false );
	mIdMediaTimeControlSeek = start_id++;
	mGluiMediaTimeControlWindow->add_button( "SEEK", mIdMediaTimeControlSeek, gluiCallbackWrapper );
	mGluiMediaTimeControlWindow->add_column( false );


	// top window - media controls for "browser" media types (e.g. web browser)
	mGluiMediaBrowserControlWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );
	mGluiMediaBrowserControlWindow->set_main_gfx_window( mAppWindow );
	mIdMediaBrowserControlBack = start_id++;
	mMediaBrowserControlBackButton = mGluiMediaBrowserControlWindow->add_button( "BACK", mIdMediaBrowserControlBack, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlStop = start_id++;
	mGluiMediaBrowserControlWindow->add_button( "STOP", mIdMediaBrowserControlStop, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlForward = start_id++;
	mMediaBrowserControlForwardButton = mGluiMediaBrowserControlWindow->add_button( "FORWARD", mIdMediaBrowserControlForward, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlHome = start_id++;
	mGluiMediaBrowserControlWindow->add_button( "HOME", mIdMediaBrowserControlHome, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlReload = start_id++;
	mGluiMediaBrowserControlWindow->add_button( "RELOAD", mIdMediaBrowserControlReload, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlClearCache = start_id++;
	mGluiMediaBrowserControlWindow->add_button( "CLEAR CACHE", mIdMediaBrowserControlClearCache, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlClearCookies = start_id++;
	mGluiMediaBrowserControlWindow->add_button( "CLEAR COOKIES", mIdMediaBrowserControlClearCookies, gluiCallbackWrapper );
	mGluiMediaBrowserControlWindow->add_column( false );
	mIdMediaBrowserControlEnableCookies = start_id++;
	mMediaBrowserControlEnableCookies = 0;
	mGluiMediaBrowserControlWindow->add_checkbox( "Enable Cookies", &mMediaBrowserControlEnableCookies, mIdMediaBrowserControlEnableCookies, gluiCallbackWrapper );

	// top window - misc controls
	GLUI* glui_window_misc_control = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_TOP );
	mIdRandomPanelCount = start_id++;
	mRandomPanelCount = 0;
	glui_window_misc_control->add_checkbox( "Randomize panel count", &mRandomPanelCount, mIdRandomPanelCount, gluiCallbackWrapper );
	glui_window_misc_control->set_main_gfx_window( mAppWindow );
	glui_window_misc_control->add_column( true );
	mIdRandomBookmarks = start_id++;
	mRandomBookmarks = 0;
	glui_window_misc_control->add_checkbox( "Randomize bookmarks", &mRandomBookmarks, mIdRandomBookmarks, gluiCallbackWrapper );
	glui_window_misc_control->set_main_gfx_window( mAppWindow );
	glui_window_misc_control->add_column( true );

	mIdDisableTimeout = start_id++;
	mDisableTimeout = 0;
	glui_window_misc_control->add_checkbox( "Disable plugin timeout", &mDisableTimeout, mIdDisableTimeout, gluiCallbackWrapper );
	glui_window_misc_control->set_main_gfx_window( mAppWindow );
	glui_window_misc_control->add_column( true );

	mIdUsePluginReadThread = start_id++;
	mUsePluginReadThread = 0;
	glui_window_misc_control->add_checkbox( "Use plugin read thread", &mUsePluginReadThread, mIdUsePluginReadThread, gluiCallbackWrapper );
	glui_window_misc_control->set_main_gfx_window( mAppWindow );
	glui_window_misc_control->add_column( true );

	mIdLargePanelSpacing = start_id++;
	mLargePanelSpacing = 0;
	glui_window_misc_control->add_checkbox( "Large Panel Spacing", &mLargePanelSpacing, mIdLargePanelSpacing, gluiCallbackWrapper );
	glui_window_misc_control->set_main_gfx_window( mAppWindow );
	glui_window_misc_control->add_column( true );

	// bottom window - status
	mBottomGLUIWindow = GLUI_Master.create_glui_subwindow( mAppWindow, GLUI_SUBWINDOW_BOTTOM );
	mStatusText = mBottomGLUIWindow->add_statictext( "" );
	mBottomGLUIWindow->set_main_gfx_window( mAppWindow );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::resetView()
{
	mViewRotationCtrl->reset();

	mViewScaleCtrl->set_x( 0.0f );
	mViewScaleCtrl->set_y( 0.0f );
	mViewScaleCtrl->set_z( 3.0f );

	mViewTranslationCtrl->set_x( 0.0f );
	mViewTranslationCtrl->set_y( 0.0f );
	mViewTranslationCtrl->set_z( 0.0f );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::makePickTexture( int id, GLuint* texture_handle, unsigned char** texture_pixels )
{
	int pick_texture_width = 1024;
	int pick_texture_height = 1024;
	int pick_texture_depth = 3;
	unsigned char* ptr = new unsigned char[ pick_texture_width * pick_texture_height * pick_texture_depth ];
	for( int y = 0; y < pick_texture_height; ++y )
	{
		for( int x = 0; x < pick_texture_width * pick_texture_depth ; x += pick_texture_depth )
		{
			unsigned long bits = 0L;
			bits |= ( id << 20 ) | ( y << 10 ) | ( x / 3 );
			unsigned char r_component = ( bits >> 16 ) & 0xff;
			unsigned char g_component = ( bits >> 8 ) & 0xff;
			unsigned char b_component = bits & 0xff;

			ptr[ y * pick_texture_width * pick_texture_depth + x + 0 ] = r_component;
			ptr[ y * pick_texture_width * pick_texture_depth + x + 1 ] = g_component;
			ptr[ y * pick_texture_width * pick_texture_depth + x + 2 ] = b_component;
		};
	};

	glGenTextures( 1, texture_handle );

	checkGLError("glGenTextures");
	std::cout << "glGenTextures returned " << *texture_handle << std::endl;

	bindTexture( *texture_handle );
	glTexImage2D( GL_TEXTURE_2D, 0,
					GL_RGB,
						pick_texture_width, pick_texture_height,
							0, GL_RGB, GL_UNSIGNED_BYTE, ptr );

	*texture_pixels = ptr;
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLMediaPluginTest::mimeTypeFromUrl( std::string& url )
{
	// default to web
	std::string mime_type = "text/html";

	// we may need a more advanced MIME type accessor later :-)
	if ( url.find( ".mov" ) != std::string::npos )	// Movies
		mime_type = "video/quicktime";
	else
	if ( url.find( ".txt" ) != std::string::npos )	// Apple Text descriptors
		mime_type = "video/quicktime";
	else
	if ( url.find( ".mp3" ) != std::string::npos )	// Apple Text descriptors
		mime_type = "video/quicktime";
	else
	if ( url.find( "example://" ) != std::string::npos )	// Example plugin
		mime_type = "example/example";

	return mime_type;
}

////////////////////////////////////////////////////////////////////////////////
//
std::string LLMediaPluginTest::pluginNameFromMimeType( std::string& mime_type )
{
#if LL_DARWIN
	std::string plugin_name( "media_plugin_null.dylib" );
	if ( mime_type == "video/quicktime" )
		plugin_name = "media_plugin_quicktime.dylib";
	else
	if ( mime_type == "text/html" )
		plugin_name = "media_plugin_webkit.dylib";

#elif LL_WINDOWS
	std::string plugin_name( "media_plugin_null.dll" );

	if ( mime_type == "video/quicktime" )
		plugin_name = "media_plugin_quicktime.dll";
	else
	if ( mime_type == "text/html" )
		plugin_name = "media_plugin_webkit.dll";
	else
	if ( mime_type == "example/example" )
		plugin_name = "media_plugin_example.dll";

#elif LL_LINUX
	std::string plugin_name( "libmedia_plugin_null.so" );

	if ( mime_type == "video/quicktime" )
		plugin_name = "libmedia_plugin_quicktime.so";
	else
	if ( mime_type == "text/html" )
		plugin_name = "libmedia_plugin_webkit.so";
#endif
	return plugin_name;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::addMediaPanel( std::string url )
{
	// Get the plugin filename using the URL
	std::string mime_type = mimeTypeFromUrl( url );
	std::string plugin_name = pluginNameFromMimeType( mime_type );

	// create a random size for the new media
	int media_width;
	int media_height;
	getRandomMediaSize( media_width, media_height, mime_type );

	// make a new plugin
	LLPluginClassMedia* media_source = new LLPluginClassMedia(this);

	// tell the plugin what size we asked for
	media_source->setSize( media_width, media_height );

	// Use the launcher start and initialize the plugin
#if LL_DARWIN || LL_LINUX
	std::string launcher_name( "SLPlugin" );
#elif LL_WINDOWS
	std::string launcher_name( "SLPlugin.exe" );
#endif

	// for this test app, use the cwd as the user data path (ugh).
#if LL_WINDOWS
	std::string user_data_path = ".\\";
#else
        char cwd[ FILENAME_MAX ];
	if (NULL == getcwd( cwd, FILENAME_MAX - 1 ))
	{
		std::cerr << "Couldn't get cwd - probably too long - failing to init." << std::endl;
		return;
	}
	std::string user_data_path = std::string( cwd ) + "/";
#endif
	media_source->setUserDataPath(user_data_path);
	media_source->init( launcher_name, plugin_name, false );
	media_source->setDisableTimeout(mDisableTimeout);

	// make a new panel and save parameters
	mediaPanel* panel = new mediaPanel;
	panel->mMediaSource = media_source;
	panel->mStartUrl = url;
	panel->mMimeType = mime_type;
	panel->mMediaWidth = media_width;
	panel->mMediaHeight = media_height;
	panel->mTextureWidth = 0;
	panel->mTextureHeight = 0;
	panel->mTextureScaleX = 0;
	panel->mTextureScaleY = 0;
	panel->mMediaTextureHandle = 0;
	panel->mPickTextureHandle = 0;
	panel->mAppTextureCoordsOpenGL = false;	// really need an 'undefined' state here too
	panel->mReadyToRender = false;

	// look through current media panels to find an unused index number
	bool id_exists = true;
	for( int nid = 0; nid < mMaxPanels; ++nid )
	{
		// does this id exist already?
		id_exists = false;
		for( int pid = 0; pid < (int)mMediaPanels.size(); ++pid )
		{
			if ( nid == mMediaPanels[ pid ]->mId )
			{
				id_exists = true;
				break;
			};
		};

		// id wasn't found so we can use it
		if ( ! id_exists )
		{
			panel->mId = nid;
			break;
		};
	};

	// if we get here and this flag is set, there is no room for any more panels
	if ( id_exists )
	{
		std::cout << "No room for any more panels" << std::endl;
	}
	else
	{
		// now we have the ID we can use it to make the
		// pick texture (id is baked into texture pixels)
		makePickTexture( panel->mId, &panel->mPickTextureHandle, &panel->mPickTexturePixels );

		// save this in the list of panels
		mMediaPanels.push_back( panel );

		// select the panel that was just created
		selectPanel( panel );

		// load and start the URL
		panel->mMediaSource->loadURI( url );
		panel->mMediaSource->start();

		std::cout << "Adding new media panel for " << url << "(" << media_width << "x" << media_height << ") with index " << panel->mId << " - total panels = " << mMediaPanels.size() << std::endl;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::updateMediaPanel( mediaPanel* panel )
{
//	checkGLError("LLMediaPluginTest::updateMediaPanel");

	if ( ! panel )
		return;

	if(!panel->mMediaSource || !panel->mMediaSource->textureValid())
	{
		panel->mReadyToRender = false;
		return;
	}

	// take a reference copy of the plugin values since they
	// might change during this lifetime of this function
	int plugin_media_width = panel->mMediaSource->getWidth();
	int plugin_media_height = panel->mMediaSource->getHeight();
	int plugin_texture_width = panel->mMediaSource->getBitsWidth();
	int plugin_texture_height = panel->mMediaSource->getBitsHeight();

	// If the texture isn't created or the media or texture dimensions changed AND
	// the sizes are valid then we need to delete the old media texture (if necessary)
	// then make a new one.
	if ((panel->mMediaTextureHandle == 0 ||
		 panel->mMediaWidth != plugin_media_width ||
		 panel->mMediaHeight != plugin_media_height ||
		 panel->mTextureWidth != plugin_texture_width ||
		 panel->mTextureHeight != plugin_texture_height) &&
		( plugin_media_width > 0 && plugin_media_height > 0 &&
		  plugin_texture_width > 0 && plugin_texture_height > 0 ) )
	{
		std::cout << "Valid media size (" <<  plugin_media_width << " x " << plugin_media_height
				<< ") and texture size (" <<  plugin_texture_width << " x " << plugin_texture_height
				<< ") for panel with ID=" << panel->mId << " - making texture" << std::endl;

		// delete old GL texture
		if ( isTexture( panel->mMediaTextureHandle ) )
		{
			std::cerr << "updateMediaPanel: deleting texture " << panel->mMediaTextureHandle << std::endl;
			glDeleteTextures( 1, &panel->mMediaTextureHandle );
			panel->mMediaTextureHandle = 0;
		}

		std::cerr << "before: pick texture is " << panel->mPickTextureHandle << ", media texture is " << panel->mMediaTextureHandle << std::endl;

		// make a GL texture based on the dimensions the plugin told us
		GLuint new_texture = 0;
		glGenTextures( 1, &new_texture );

		checkGLError("glGenTextures");

		std::cout << "glGenTextures returned " << new_texture << std::endl;

		panel->mMediaTextureHandle = new_texture;

		bindTexture( panel->mMediaTextureHandle );

		std::cout << "Setting texture size to " << plugin_texture_width << " x " << plugin_texture_height << std::endl;
		glTexImage2D( GL_TEXTURE_2D, 0,
			GL_RGB,
				plugin_texture_width, plugin_texture_height,
					0, GL_RGB, GL_UNSIGNED_BYTE,
						0 );


		std::cerr << "after: pick texture is " << panel->mPickTextureHandle << ", media texture is " << panel->mMediaTextureHandle << std::endl;
	};

	// update our record of the media and texture dimensions
	// NOTE: do this after we we check for sizes changes
	panel->mMediaWidth = plugin_media_width;
	panel->mMediaHeight = plugin_media_height;
	panel->mTextureWidth = plugin_texture_width;
	panel->mTextureHeight = plugin_texture_height;
	if ( plugin_texture_width > 0 )
	{
		panel->mTextureScaleX = (double)panel->mMediaWidth / (double)panel->mTextureWidth;
	};
	if ( plugin_texture_height > 0 )
	{
		panel->mTextureScaleY = (double)panel->mMediaHeight / (double)panel->mTextureHeight;
	};

	// update the flag which tells us if the media source uses OprnGL coords or not.
	panel->mAppTextureCoordsOpenGL = panel->mMediaSource->getTextureCoordsOpenGL();

	// Check to see if we have enough to render this panel.
	// If we do, set a flag that the display functions use so
	// they only render a panel with media if it's ready.
	if ( panel->mMediaWidth < 0 ||
		 panel->mMediaHeight < 0 ||
		 panel->mTextureWidth < 1 ||
		 panel->mTextureHeight < 1 ||
		 panel->mMediaTextureHandle == 0 )
	{
		panel->mReadyToRender = false;
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::replaceMediaPanel( mediaPanel* panel, std::string url )
{
	// no media panels so we can't change anything - have to add
	if ( mMediaPanels.size() == 0 )
		return;

	// sanity check
	if ( ! panel )
		return;

	int index;
	for(index = 0; index < (int)mMediaPanels.size(); index++)
	{
		if(mMediaPanels[index] == panel)
			break;
	}

	if(index >= (int)mMediaPanels.size())
	{
		// panel isn't in mMediaPanels
		return;
	}

	std::cout << "Replacing media panel with index " << panel->mId << std::endl;

	int panel_id = panel->mId;

	if(mSelectedPanel == panel)
		mSelectedPanel = NULL;

	delete panel;

	// Get the plugin filename using the URL
	std::string mime_type = mimeTypeFromUrl( url );
	std::string plugin_name = pluginNameFromMimeType( mime_type );

	// create a random size for the new media
	int media_width;
	int media_height;
	getRandomMediaSize( media_width, media_height, mime_type );

	// make a new plugin
	LLPluginClassMedia* media_source = new LLPluginClassMedia(this);

	// tell the plugin what size we asked for
	media_source->setSize( media_width, media_height );

	// Use the launcher start and initialize the plugin
#if LL_DARWIN || LL_LINUX
	std::string launcher_name( "SLPlugin" );
#elif LL_WINDOWS
	std::string launcher_name( "SLPlugin.exe" );
#endif

	// for this test app, use the cwd as the user data path (ugh).
#if LL_WINDOWS
	std::string user_data_path = ".\\";
#else
        char cwd[ FILENAME_MAX ];
	if (NULL == getcwd( cwd, FILENAME_MAX - 1 ))
	{
		std::cerr << "Couldn't get cwd - probably too long - failing to init." << std::endl;
		return;
	}
	std::string user_data_path = std::string( cwd ) + "/";
#endif

	media_source->setUserDataPath(user_data_path);
	media_source->init( launcher_name, plugin_name, false );
	media_source->setDisableTimeout(mDisableTimeout);

	// make a new panel and save parameters
	panel = new mediaPanel;
	panel->mMediaSource = media_source;
	panel->mStartUrl = url;
	panel->mMimeType = mime_type;
	panel->mMediaWidth = media_width;
	panel->mMediaHeight = media_height;
	panel->mTextureWidth = 0;
	panel->mTextureHeight = 0;
	panel->mTextureScaleX = 0;
	panel->mTextureScaleY = 0;
	panel->mMediaTextureHandle = 0;
	panel->mPickTextureHandle = 0;
	panel->mAppTextureCoordsOpenGL = false;	// really need an 'undefined' state here too
	panel->mReadyToRender = false;

	panel->mId = panel_id;

	// Replace the entry in the panels array
	mMediaPanels[index] = panel;

	// now we have the ID we can use it to make the
	// pick texture (id is baked into texture pixels)
	makePickTexture( panel->mId, &panel->mPickTextureHandle, &panel->mPickTexturePixels );

	// select the panel that was just created
	selectPanel( panel );

	// load and start the URL
	panel->mMediaSource->loadURI( url );
	panel->mMediaSource->start();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::getRandomMediaSize( int& width, int& height, std::string mime_type )
{
	// Make a new media source with a random size which we'll either
	// directly or the media plugin will tell us what it wants later.
	// Use a random size so we can test support for weird media sizes.
	// (Almost everything else will get filled in later once the
	// plugin responds)
	// NB. Do we need to enforce that width is on 4 pixel boundary?
	width = ( ( rand() % 170 ) + 30 ) * 4;
	height = ( ( rand() % 170 ) + 30 ) * 4;

	// adjust this random size if it's a browser so we get
	// a more useful size for testing..
	if ( mime_type == "text/html" || mime_type == "example/example"  )
	{
		width = ( ( rand() % 100 ) + 100 ) * 4;
		height = ( width * ( ( rand() % 400 ) + 1000 ) ) / 1000;
	};
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::remMediaPanel( mediaPanel* panel )
{
	// always leave one panel
	if ( mMediaPanels.size() == 1 )
		return;

	// sanity check - don't think this can happen but see above for a case where it might...
	if ( ! panel )
		return;

	std::cout << "Removing media panel with index " << panel->mId << " - total panels = " << mMediaPanels.size() - 1 << std::endl;

	if(mSelectedPanel == panel)
		mSelectedPanel = NULL;

	delete panel;

	// remove from storage list
	for( int i = 0; i < (int)mMediaPanels.size(); ++i )
	{
		if ( mMediaPanels[ i ] == panel )
		{
			mMediaPanels.erase( mMediaPanels.begin() + i );
			break;
		};
	};

	// select the first panel
	selectPanel( mMediaPanels[ 0 ] );
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::updateStatusBar()
{
	if ( ! mSelectedPanel )
		return;

	// cache results - this is a very slow function
	static int cached_id = -1;
	static int cached_media_width = -1;
	static int cached_media_height = -1;
	static int cached_texture_width = -1;
	static int cached_texture_height = -1;
	static bool cached_supports_browser_media = true;
	static bool cached_supports_time_media = false;
	static int cached_movie_time = -1;
	static GLfloat cached_distance = -1.0f;

	static std::string cached_plugin_version = "";
	if (
		 cached_id == mSelectedPanel->mId &&
		 cached_media_width == mSelectedPanel->mMediaWidth &&
		 cached_media_height  == mSelectedPanel->mMediaHeight &&
		 cached_texture_width == mSelectedPanel->mTextureWidth &&
		 cached_texture_height == mSelectedPanel->mTextureHeight &&
		 cached_supports_browser_media == mSelectedPanel->mMediaSource->pluginSupportsMediaBrowser() &&
		 cached_supports_time_media == mSelectedPanel->mMediaSource->pluginSupportsMediaTime() &&
		 cached_plugin_version == mSelectedPanel->mMediaSource->getPluginVersion() &&
		 cached_movie_time == (int)mSelectedPanel->mMediaSource->getCurrentTime() &&
		 cached_distance == mDistanceCameraToSelectedGeometry
	   )
	{
		// nothing changed so don't spend time here
		return;
	};

	std::ostringstream stream( "" );

	stream.str( "" );
	stream.clear();

	stream << "Id: ";
	stream << std::setw( 2 ) << std::setfill( '0' );
	stream << mSelectedPanel->mId;
	stream << " | ";
	stream << "Media: ";
	stream << std::setw( 3 ) << std::setfill( '0' );
	stream << mSelectedPanel->mMediaWidth;
	stream << " x ";
	stream << std::setw( 3 ) << std::setfill( '0' );
	stream << mSelectedPanel->mMediaHeight;
	stream << " | ";
	stream << "Texture: ";
	stream << std::setw( 4 ) << std::setfill( '0' );
	stream << mSelectedPanel->mTextureWidth;
	stream << " x ";
	stream << std::setw( 4 ) << std::setfill( '0' );
	stream << mSelectedPanel->mTextureHeight;

	stream << " | ";
	stream << "Distance: ";
	stream << std::setw( 6 );
	stream << std::setprecision( 3 );
	stream << std::setprecision( 3 );
	stream << mDistanceCameraToSelectedGeometry;
	stream << " | ";

	if ( mSelectedPanel->mMediaSource->pluginSupportsMediaBrowser() )
		stream << "BROWSER";
	else
	if ( mSelectedPanel->mMediaSource->pluginSupportsMediaTime() )
		stream << "TIME   ";
	stream << " | ";
	stream << mSelectedPanel->mMediaSource->getPluginVersion();
	stream << " | ";
	if ( mSelectedPanel->mMediaSource->pluginSupportsMediaTime() )
	{
		stream << std::setw( 3 ) << std::setfill( '0' );
		stream << (int)mSelectedPanel->mMediaSource->getCurrentTime();
		stream << " / ";
		stream << std::setw( 3 ) << std::setfill( '0' );
		stream << (int)mSelectedPanel->mMediaSource->getDuration();
		stream << " @ ";
		stream << (int)mSelectedPanel->mMediaSource->getCurrentPlayRate();
		stream << " | ";
	};

	glutSetWindow( mBottomGLUIWindow->get_glut_window_id() );
	mStatusText->set_text( const_cast< char*>( stream.str().c_str() ) );
	glutSetWindow( mAppWindow );

	// caching
	cached_id = mSelectedPanel->mId;
	cached_media_width = mSelectedPanel->mMediaWidth;
	cached_media_height = mSelectedPanel->mMediaHeight;
	cached_texture_width = mSelectedPanel->mTextureWidth;
	cached_texture_height = mSelectedPanel->mTextureHeight;
	cached_supports_browser_media = mSelectedPanel->mMediaSource->pluginSupportsMediaBrowser();
	cached_supports_time_media = mSelectedPanel->mMediaSource->pluginSupportsMediaTime();
	cached_plugin_version = mSelectedPanel->mMediaSource->getPluginVersion();
	cached_movie_time = (int)mSelectedPanel->mMediaSource->getCurrentTime();
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::dumpPanelInfo()
{
	std::cout << std::endl << "===== Media Panels =====" << std::endl;
	for( int i = 0; i < (int)mMediaPanels.size(); ++i )
	{
		std::cout << std::setw( 2 ) << std::setfill( '0' );
		std::cout << i + 1 << "> ";
		std::cout << "Id: ";
		std::cout << std::setw( 2 ) << std::setfill( '0' );
		std::cout << mMediaPanels[ i ]->mId;
		std::cout << " | ";
		std::cout << "Media: ";
		std::cout << std::setw( 3 ) << std::setfill( '0' );
		std::cout << mMediaPanels[ i ]->mMediaWidth;
		std::cout << " x ";
		std::cout << std::setw( 3 ) << std::setfill( '0' );
		std::cout << mMediaPanels[ i ]->mMediaHeight;
		std::cout << " | ";
		std::cout << "Texture: ";
		std::cout << std::setw( 4 ) << std::setfill( '0' );
		std::cout << mMediaPanels[ i ]->mTextureWidth;
		std::cout << " x ";
		std::cout << std::setw( 4 ) << std::setfill( '0' );
		std::cout << mMediaPanels[ i ]->mTextureHeight;
		std::cout << " | ";
		if ( mMediaPanels[ i ] == mSelectedPanel )
			std::cout << "(selected)";

		std::cout << std::endl;
	};
	std::cout << "========================" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
//
void LLMediaPluginTest::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
	// Uncomment this to make things much, much quieter.
//	return;

	switch(event)
	{
		case MEDIA_EVENT_CONTENT_UPDATED:
			// too spammy -- don't log these
//			std::cerr <<  "Media event:  MEDIA_EVENT_CONTENT_UPDATED " << std::endl;
		break;

		case MEDIA_EVENT_TIME_DURATION_UPDATED:
			// too spammy -- don't log these
//			std::cerr <<  "Media event:  MEDIA_EVENT_TIME_DURATION_UPDATED, time is " << self->getCurrentTime() << " of " << self->getDuration() << std::endl;
		break;

		case MEDIA_EVENT_SIZE_CHANGED:
			std::cerr <<  "Media event:  MEDIA_EVENT_SIZE_CHANGED " << std::endl;
		break;

		case MEDIA_EVENT_CURSOR_CHANGED:
			std::cerr <<  "Media event:  MEDIA_EVENT_CURSOR_CHANGED, new cursor is " << self->getCursorName() << std::endl;
		break;

		case MEDIA_EVENT_NAVIGATE_BEGIN:
			std::cerr <<  "Media event:  MEDIA_EVENT_NAVIGATE_BEGIN " << std::endl;
		break;

		case MEDIA_EVENT_NAVIGATE_COMPLETE:
			std::cerr <<  "Media event:  MEDIA_EVENT_NAVIGATE_COMPLETE, result string is: " << self->getNavigateResultString() << std::endl;
		break;

		case MEDIA_EVENT_PROGRESS_UPDATED:
			std::cerr <<  "Media event:  MEDIA_EVENT_PROGRESS_UPDATED, loading at " << self->getProgressPercent() << "%" << std::endl;
		break;

		case MEDIA_EVENT_STATUS_TEXT_CHANGED:
			std::cerr <<  "Media event:  MEDIA_EVENT_STATUS_TEXT_CHANGED, new status text is: " << self->getStatusText() << std::endl;
		break;

		case MEDIA_EVENT_NAME_CHANGED:
			std::cerr <<  "Media event:  MEDIA_EVENT_NAME_CHANGED, new name is: " << self->getMediaName() << std::endl;
			glutSetWindowTitle( self->getMediaName().c_str() );
		break;

		case MEDIA_EVENT_LOCATION_CHANGED:
		{
			std::cerr <<  "Media event:  MEDIA_EVENT_LOCATION_CHANGED, new uri is: " << self->getLocation() << std::endl;
			mediaPanel* panel = findMediaPanel(self);
			if(panel != NULL)
			{
				panel->mStartUrl = self->getLocation();
				if(panel == mSelectedPanel)
				{
					mUrlEdit->set_text(const_cast<char*>(panel->mStartUrl.c_str()) );
				}
			}
		}
		break;

		case MEDIA_EVENT_CLICK_LINK_HREF:
			std::cerr <<  "Media event:  MEDIA_EVENT_CLICK_LINK_HREF, uri is " << self->getClickURL() << std::endl;
		break;

		case MEDIA_EVENT_CLICK_LINK_NOFOLLOW:
			std::cerr <<  "Media event:  MEDIA_EVENT_CLICK_LINK_NOFOLLOW, uri is " << self->getClickURL() << std::endl;
		break;

		case MEDIA_EVENT_PLUGIN_FAILED:
			std::cerr <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED" << std::endl;
		break;

		case MEDIA_EVENT_PLUGIN_FAILED_LAUNCH:
			std::cerr <<  "Media event:  MEDIA_EVENT_PLUGIN_FAILED_LAUNCH" << std::endl;
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
static void gluiCallbackWrapper( int control_id )
{
	if ( gApplication )
		gApplication->gluiCallback( control_id );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutReshape( int width, int height )
{
	if ( gApplication )
		gApplication->reshape( width, height );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutDisplay()
{
	if ( gApplication )
		gApplication->display();
};

////////////////////////////////////////////////////////////////////////////////
//
void glutIdle(int update_ms)
{
	GLUI_Master.set_glutTimerFunc( update_ms, glutIdle, update_ms);

	if ( gApplication )
		gApplication->idle();

};

////////////////////////////////////////////////////////////////////////////////
//
void glutKeyboard( unsigned char key, int x, int y )
{
	if ( gApplication )
		gApplication->keyboard( key );
};

////////////////////////////////////////////////////////////////////////////////
//
void glutMousePassive( int x, int y )
{
	if ( gApplication )
		gApplication->mousePassive( x, y );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseMove( int x , int y )
{
	if ( gApplication )
		gApplication->mouseMove( x, y );
}

////////////////////////////////////////////////////////////////////////////////
//
void glutMouseButton( int button, int state, int x, int y )
{
	if ( gApplication )
		gApplication->mouseButton( button, state, x, y );
}

////////////////////////////////////////////////////////////////////////////////
//
int main( int argc, char* argv[] )
{
#if LL_DARWIN
	// Set the current working directory to <application bundle>/Contents/Resources/
	CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	if(resources_url != NULL)
	{
		CFStringRef resources_string = CFURLCopyFileSystemPath(resources_url, kCFURLPOSIXPathStyle);
		CFRelease(resources_url);
		if(resources_string != NULL)
		{
			char buffer[PATH_MAX] = "";
			if(CFStringGetCString(resources_string, buffer, sizeof(buffer), kCFStringEncodingUTF8))
			{
				chdir(buffer);
			}
			CFRelease(resources_string);
		}
	}
#endif

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB );

	const int app_window_x = 80;
	const int app_window_y = 0;
	const int app_window_width = 960;
	const int app_window_height = 960;

	glutInitWindowPosition( app_window_x, app_window_y );
	glutInitWindowSize( app_window_width, app_window_height );

	int app_window_handle = glutCreateWindow( "LLMediaPluginTest" );

	glutDisplayFunc( glutDisplay );

	GLUI_Master.set_glutReshapeFunc( glutReshape );
	GLUI_Master.set_glutKeyboardFunc( glutKeyboard );
	GLUI_Master.set_glutMouseFunc( glutMouseButton );

	glutPassiveMotionFunc( glutMousePassive );
	glutMotionFunc( glutMouseMove );

	glutSetWindow( app_window_handle );

	gApplication = new LLMediaPluginTest( app_window_handle, app_window_width, app_window_height );

	// update at approximately 60hz
	int update_ms = 1000 / 60;

	GLUI_Master.set_glutTimerFunc( update_ms, glutIdle, update_ms);

	glutMainLoop();

	delete gApplication;
}
