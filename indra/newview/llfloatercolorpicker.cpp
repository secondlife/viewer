/** 
 * @file llfloatercolorpicker.cpp
 * @brief Generic system color picker
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include <sstream>
#include <iomanip>

#include "llfloatercolorpicker.h"

#include "llfontgl.h"
#include "llsys.h"
#include "llgl.h"
#include "v3dmath.h"
#include "lldir.h"
#include "llui.h"
#include "lllineeditor.h"
#include "v4coloru.h"
#include "llbutton.h"
#include "llviewercontrol.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"
#include "llviewerwindow.h"
#include "llgl.h"
#include "llmemory.h"
#include "llimage.h"
#include "llmousehandler.h"
#include "llimagegl.h"
#include "llglheaders.h"
#include "llcheckboxctrl.h"
#include "llworld.h"
#include "lltextbox.h"
#include "lluiconstants.h"
#include "llfocusmgr.h"
#include "lltoolmgr.h"
#include "lltoolpipette.h"
#include "lldraghandle.h"

const F32 CONTEXT_CONE_IN_ALPHA = 0.0f;
const F32 CONTEXT_CONE_OUT_ALPHA = 1.f;
const F32 CONTEXT_FADE_TIME = 0.08f;

//////////////////////////////////////////////////////////////////////////////
//
// Class LLFloaterColorPicker
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// default ctor
LLFloaterColorPicker::
LLFloaterColorPicker (LLColorSwatchCtrl* swatch, BOOL show_apply_immediate )
	: LLFloater ("Color Picker Floater"),
	  mComponents			( 3 ),
	  mMouseDownInLumRegion	( FALSE ),
	  mMouseDownInHueRegion	( FALSE ),
	  mMouseDownInSwatch	( FALSE ),
	  mRGBViewerImageLeft	( 140 ),
	  mRGBViewerImageTop	( 356 ),
	  mRGBViewerImageWidth	( 256 ),
	  mRGBViewerImageHeight ( 256 ),
	  mLumRegionLeft		( mRGBViewerImageLeft + mRGBViewerImageWidth + 16 ),
	  mLumRegionTop			( mRGBViewerImageTop ),
	  mLumRegionWidth		( 16 ),
	  mLumRegionHeight		( mRGBViewerImageHeight ),
	  mLumMarkerSize		( 6 ),
	  mSwatchRegionLeft		( 12 ),
	  mSwatchRegionTop		( 160 + 16 - 4 ),	// get help text baseline to line up with bottom of RGB viewer
	  mSwatchRegionWidth	( 110 ),
	  mSwatchRegionHeight	( 60 ),
	  mSwatchView			( NULL ),
	  numPaletteColumns		( 16 ),
	  numPaletteRows		( 2 ),
	  highlightEntry		( -1 ),
	  mPaletteRegionLeft	( mSwatchRegionLeft - 1 ),
	  mPaletteRegionTop		( 100 - 8 ),
	  mPaletteRegionWidth	( mLumRegionLeft + mLumRegionWidth - mSwatchRegionLeft + 2 ),
	  mPaletteRegionHeight	( 40 ),
	  mSwatch				( swatch ),
	  mActive				( TRUE ),
	  mCanApplyImmediately	( show_apply_immediate ),
	  mContextConeOpacity	( 0.f )
{
	// create user interface for this picker
	createUI ();

	if (!mCanApplyImmediately)
	{
		mApplyImmediateCheck->setEnabled(FALSE);
		mApplyImmediateCheck->set(FALSE);
	}
}

//////////////////////////////////////////////////////////////////////////////
// dtor
LLFloaterColorPicker::
~LLFloaterColorPicker()
{
	// destroy the UI we created
	destroyUI ();
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
createUI ()
{
	// build the majority of the gui using the factory builder
	gUICtrlFactory->buildFloater ( this, "floater_color_picker.xml" );
	setVisible ( FALSE );

	// create RGB type area (not really RGB but it's got R,G & B in it.,..

	LLPointer<LLImageRaw> raw = new LLImageRaw ( mRGBViewerImageWidth, mRGBViewerImageHeight, mComponents );
	U8* bits = raw->getData();
	S32 linesize = mRGBViewerImageWidth * mComponents;
	for ( S32 y = 0; y < mRGBViewerImageHeight; ++y )
	{
		for ( S32 x = 0; x < linesize; x += mComponents )
		{
			F32 rVal, gVal, bVal;

			hslToRgb ( (F32)x / (F32) ( linesize - 1 ),
					   (F32)y / (F32) ( mRGBViewerImageHeight - 1 ),
					   0.5f,
					   rVal,
					   gVal,
					   bVal );

			* ( bits + x + y * linesize + 0 ) = ( U8 )( rVal * 255.0f );
			* ( bits + x + y * linesize + 1 ) = ( U8 )( gVal * 255.0f );
			* ( bits + x + y * linesize + 2 ) = ( U8 )( bVal * 255.0f );
		}
	}
	mRGBImage = new LLImageGL ( (LLImageRaw*)raw, FALSE );
	mRGBImage->bind();
	mRGBImage->setClamp(TRUE, TRUE);
	
	// create palette
	for ( S32 each = 0; each < numPaletteColumns * numPaletteRows; ++each )
	{
		std::ostringstream codec;
		codec << "ColorPaletteEntry" << std::setfill ( '0' ) << std::setw ( 2 ) << each + 1;

		// argh!
		const std::string s ( codec.str () );
		mPalette.push_back ( new LLColor4 ( gSavedSettings.getColor4 ( s )  ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
showUI ()
{
	setVisible ( TRUE );
	setFocus ( TRUE );
	open();		/*Flawfinder: ignore*/

	// HACK: if system color picker is required - close the SL one we made and use default system dialog
	if ( gSavedSettings.getBOOL ( "UseDefaultColorPicker" ) )
	{
		LLColorSwatchCtrl* swatch = getSwatch ();

		setVisible ( FALSE );

		// code that will get switched in for default system color picker
		if ( swatch )
		{
			LLColor4 curCol = swatch->get ();
			send_agent_pause();
			gViewerWindow->getWindow ()->dialog_color_picker ( &curCol [ 0 ], &curCol [ 1 ], &curCol [ 2 ] );
			send_agent_resume();

			setOrigRgb ( curCol [ 0 ], curCol [ 1 ], curCol [ 2 ] );
			setCurRgb( curCol [ 0 ], curCol [ 1 ], curCol [ 2 ] );

			LLColorSwatchCtrl::onColorChanged ( swatch, LLColorSwatchCtrl::COLOR_CHANGE );
		}

		close();
	}
}

//////////////////////////////////////////////////////////////////////////////
// called after the dialog is rendered
BOOL
LLFloaterColorPicker::
postBuild()
{
	mCancelBtn = LLViewerUICtrlFactory::getButtonByName( this, "cancel_btn" );
    mCancelBtn->setClickedCallback ( onClickCancel );
    mCancelBtn->setCallbackUserData ( this );

	mSelectBtn = LLViewerUICtrlFactory::getButtonByName( this, "select_btn");
    mSelectBtn->setClickedCallback ( onClickSelect );
    mSelectBtn->setCallbackUserData ( this );
	mSelectBtn->setFocus ( TRUE );

	mPipetteBtn = LLViewerUICtrlFactory::getButtonByName ( this, "color_pipette" );

	mPipetteBtn->setImages("eye_button_inactive.tga", "eye_button_active.tga");

	mPipetteBtn->setClickedCallback( onClickPipette );
	mPipetteBtn->setCallbackUserData ( this );

	mApplyImmediateCheck = LLViewerUICtrlFactory::getCheckBoxByName( this, "apply_immediate");
	mApplyImmediateCheck->set(gSavedSettings.getBOOL("ApplyColorImmediately"));
	mApplyImmediateCheck->setCommitCallback(onImmediateCheck);
	mApplyImmediateCheck->setCallbackUserData(this);

	childSetCommitCallback("rspin", onTextCommit, (void*)this );
	childSetCommitCallback("gspin", onTextCommit, (void*)this );
	childSetCommitCallback("bspin", onTextCommit, (void*)this );
	childSetCommitCallback("hspin", onTextCommit, (void*)this );
	childSetCommitCallback("sspin", onTextCommit, (void*)this );
	childSetCommitCallback("lspin", onTextCommit, (void*)this );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
initUI ( F32 rValIn, F32 gValIn, F32 bValIn )
{
	// start catching lose-focus events from entry widgets
	enableTextCallbacks ( TRUE );

	// under some circumstances, we get rogue values that can be calmed by clamping...
	rValIn = llclamp ( rValIn, 0.0f, 1.0f );
	gValIn = llclamp ( gValIn, 0.0f, 1.0f );
	bValIn = llclamp ( bValIn, 0.0f, 1.0f );

	// store initial value in case cancel or revert is selected
	setOrigRgb ( rValIn, gValIn, bValIn );

	// starting point for current value to
	setCurRgb ( rValIn, gValIn, bValIn );

	// unpdate text entry fields
	updateTextEntry ();
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
destroyUI ()
{
	// shut down pipette tool if active
	stopUsingPipette();

	// delete palette we created
	std::vector < LLColor4* >::iterator iter = mPalette.begin ();
	while ( iter != mPalette.end () )
	{
		delete ( *iter );
		++iter;
	}

	if ( mSwatchView )
	{
		this->removeChild ( mSwatchView );
		delete mSwatchView;
		mSwatchView = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
rgbToHsl ( F32 rValIn, F32 gValIn, F32 bValIn, F32& hValOut, F32& sValOut, F32& lValOut )
{
	F32 var_R = ( rValIn );
	F32 var_G = ( gValIn );
	F32 var_B = ( bValIn );

	F32 var_Min = ( var_R < ( var_G < var_B ? var_G : var_B ) ? var_R : ( var_G < var_B ? var_G : var_B ) );
	F32 var_Max = ( var_R > ( var_G > var_B ? var_G : var_B ) ? var_R : ( var_G > var_B ? var_G : var_B ) );

	F32 del_Max = var_Max - var_Min;

	F32 L = ( var_Max + var_Min ) / 2.0f;
	F32 H = 0.0f;
	F32 S = 0.0f;

	if ( del_Max == 0.0f )
	{
	   H = 0.0f;
	   S = 0.0f;
	}
	else
	{
		if ( L < 0.5 )
			S = del_Max / ( var_Max + var_Min );
		else
			S = del_Max / ( 2.0f - var_Max - var_Min );

		F32 del_R = ( ( ( var_Max - var_R ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;
		F32 del_G = ( ( ( var_Max - var_G ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;
		F32 del_B = ( ( ( var_Max - var_B ) / 6.0f ) + ( del_Max / 2.0f ) ) / del_Max;

		if ( var_R >= var_Max )
			H = del_B - del_G;
		else
		if ( var_G >= var_Max )
			H = ( 1.0f / 3.0f ) + del_R - del_B;
		else
		if ( var_B >= var_Max )
			H = ( 2.0f / 3.0f ) + del_G - del_R;

		if ( H < 0.0f ) H += 1.0f;
		if ( H > 1.0f ) H -= 1.0f;
	}

	// scale to meet calculation requirements
	hValOut = H * 360.0f / 360.0f;
	sValOut = S * 100.0f / 100.0f;
	lValOut = L * 100.0f / 100.0f;
}

//////////////////////////////////////////////////////////////////////////////
//
F32
LLFloaterColorPicker::
hueToRgb ( F32 val1In, F32 val2In, F32 valHUeIn )
{
	if ( valHUeIn < 0.0f ) valHUeIn += 1.0f;
	if ( valHUeIn > 1.0f ) valHUeIn -= 1.0f;
	if ( ( 6.0f * valHUeIn ) < 1.0f ) return ( val1In + ( val2In - val1In ) * 6.0f * valHUeIn );
	if ( ( 2.0f * valHUeIn ) < 1.0f ) return ( val2In );
	if ( ( 3.0f * valHUeIn ) < 2.0f ) return ( val1In + ( val2In - val1In ) * ( ( 2.0f / 3.0f ) - valHUeIn ) * 6.0f );
	return ( val1In );
}

//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
hslToRgb ( F32 hValIn, F32 sValIn, F32 lValIn, F32& rValOut, F32& gValOut, F32& bValOut )
{
	if ( sValIn < 0.00001f )
	{
		rValOut = lValIn;
		gValOut = lValIn;
		bValOut = lValIn;
	}
	else
	{
		F32 interVal1;
		F32 interVal2;

		if ( lValIn < 0.5f )
			interVal2 = lValIn * ( 1.0f + sValIn );
		else
			interVal2 = ( lValIn + sValIn ) - ( sValIn * lValIn );

		interVal1 = 2.0f * lValIn - interVal2;

		rValOut = hueToRgb ( interVal1, interVal2, hValIn + ( 1.f / 3.f ) );
		gValOut = hueToRgb ( interVal1, interVal2, hValIn );
		bValOut = hueToRgb ( interVal1, interVal2, hValIn - ( 1.f / 3.f ) );
	}
}

//////////////////////////////////////////////////////////////////////////////
// mutator for original RGB value
void
LLFloaterColorPicker::
setOrigRgb ( F32 origRIn, F32 origGIn, F32 origBIn )
{
	origR = origRIn;
	origG = origGIn;
	origB = origBIn;
}

//////////////////////////////////////////////////////////////////////////////
// accessor for original RGB value
void
LLFloaterColorPicker::
getOrigRgb ( F32& origROut, F32& origGOut, F32& origBOut )
{
	origROut = origR;
	origGOut = origG;
	origBOut = origB;
}

//////////////////////////////////////////////////////////////////////////////
// mutator for current RGB value
void
LLFloaterColorPicker::
setCurRgb ( F32 curRIn, F32 curGIn, F32 curBIn )
{
	// save current RGB
	curR = curRIn;
	curG = curGIn;
	curB = curBIn;

	// update corresponding HSL values and
	rgbToHsl ( curR, curG, curB, curH, curS, curL );

	// color changed so update text fields (fixes SL-16968)
    // HACK: turn off the call back wilst we update the text or we recurse ourselves into oblivion
    // CP: this was required when I first wrote the code but this may not be necessary anymore - leaving it there just in case
    enableTextCallbacks( FALSE );
    updateTextEntry();
    enableTextCallbacks( TRUE );
}

//////////////////////////////////////////////////////////////////////////////
// accessor for current RGB value
void
LLFloaterColorPicker::
getCurRgb ( F32& curROut, F32& curGOut, F32& curBOut )
{
	curROut = curR;
	curGOut = curG;
	curBOut = curB;
}

//////////////////////////////////////////////////////////////////////////////
// mutator for current HSL value
void
LLFloaterColorPicker::
setCurHsl ( F32 curHIn, F32 curSIn, F32 curLIn )
{
	// save current HSL
	curH = curHIn;
	curS = curSIn;
	curL = curLIn;

	// update corresponding RGB values and
	hslToRgb ( curH, curS, curL, curR, curG, curB );
}

//////////////////////////////////////////////////////////////////////////////
// accessor for current HSL value
void
LLFloaterColorPicker::
getCurHsl ( F32& curHOut, F32& curSOut, F32& curLOut )
{
	curHOut = curH;
	curSOut = curS;
	curLOut = curL;
}

//////////////////////////////////////////////////////////////////////////////
// called when 'cancel' clicked
void
LLFloaterColorPicker::
onClickCancel ( void* data )
{
	if (data)
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;

		if ( self )
		{
			self->cancelSelection ();
			self->close();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// called when 'select' clicked
void
LLFloaterColorPicker::
onClickSelect ( void* data )
{
	if (data)
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;

		if ( self )
		{
			// apply to selection
			LLColorSwatchCtrl::onColorChanged ( self->getSwatch (), LLColorSwatchCtrl::COLOR_SELECT );
			self->close();
		}
	}
}

void LLFloaterColorPicker::onClickPipette( void* data )
{
	LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;

	if ( self && gToolMgr)
	{
		BOOL pipette_active = self->mPipetteBtn->getToggleState();
		pipette_active = !pipette_active;
		if (pipette_active)
		{
			gToolPipette->setSelectCallback(onColorSelect, self);
			gToolMgr->setTransientTool(gToolPipette);
		}
		else
		{
			gToolMgr->clearTransientTool();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
// called when 'text is committed' - i,e. focus moves from a text field
void
LLFloaterColorPicker::
onTextCommit ( LLUICtrl* ctrl, void* data )
{
	if ( data )
	{
		LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
		if ( self )
		{
			self->onTextEntryChanged ( ctrl );
		}
	}
}

void LLFloaterColorPicker::onImmediateCheck( LLUICtrl* ctrl, void* data)
{
	LLFloaterColorPicker* self = ( LLFloaterColorPicker* )data;
	if (self)
	{
		gSavedSettings.setBOOL("ApplyColorImmediately", self->mApplyImmediateCheck->get());

		if (self->mApplyImmediateCheck->get())
		{
			LLColorSwatchCtrl::onColorChanged ( self->getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
		}
	}
}

void LLFloaterColorPicker::onColorSelect( const LLTextureEntry& te, void *data )
{
	LLFloaterColorPicker* self = (LLFloaterColorPicker*)data;
	if (self)
	{
		self->setCurRgb(te.getColor().mV[VRED], te.getColor().mV[VGREEN], te.getColor().mV[VBLUE]);
		if (self->mApplyImmediateCheck->get())
		{
			LLColorSwatchCtrl::onColorChanged ( self->getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
		}
	}
}

void LLFloaterColorPicker::onMouseCaptureLost(LLMouseHandler *old_captor)
{
	LLFloaterColorPicker* self = (LLFloaterColorPicker*)old_captor;
	self->setMouseDownInHueRegion(FALSE);
	self->setMouseDownInLumRegion(FALSE);
}

//////////////////////////////////////////////////////////////////////////////
//
void LLFloaterColorPicker::draw()
{
	LLRect swatch_rect;
	mSwatch->localRectToOtherView(mSwatch->getLocalRect(), &swatch_rect, this);
	// draw context cone connecting color picker with color swatch in parent floater
	LLRect local_rect = getLocalRect();
	if (gFocusMgr.childHasKeyboardFocus(this) && mSwatch->isInVisibleChain() && mContextConeOpacity > 0.001f)
	{
		LLGLSNoTexture no_texture;
		LLGLEnable(GL_CULL_FACE);
		glBegin(GL_QUADS);
		{
			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			glVertex2i(swatch_rect.mLeft, swatch_rect.mTop);
			glVertex2i(swatch_rect.mRight, swatch_rect.mTop);
			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			glVertex2i(local_rect.mRight, local_rect.mTop);
			glVertex2i(local_rect.mLeft, local_rect.mTop);

			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			glVertex2i(local_rect.mLeft, local_rect.mTop);
			glVertex2i(local_rect.mLeft, local_rect.mBottom);
			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			glVertex2i(swatch_rect.mLeft, swatch_rect.mBottom);
			glVertex2i(swatch_rect.mLeft, swatch_rect.mTop);

			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			glVertex2i(local_rect.mRight, local_rect.mBottom);
			glVertex2i(local_rect.mRight, local_rect.mTop);
			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			glVertex2i(swatch_rect.mRight, swatch_rect.mTop);
			glVertex2i(swatch_rect.mRight, swatch_rect.mBottom);

			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_OUT_ALPHA * mContextConeOpacity);
			glVertex2i(local_rect.mLeft, local_rect.mBottom);
			glVertex2i(local_rect.mRight, local_rect.mBottom);
			glColor4f(0.f, 0.f, 0.f, CONTEXT_CONE_IN_ALPHA * mContextConeOpacity);
			glVertex2i(swatch_rect.mRight, swatch_rect.mBottom);
			glVertex2i(swatch_rect.mLeft, swatch_rect.mBottom);
		}
		glEnd();
	}

	if (gFocusMgr.childHasMouseCapture(mDragHandle))
	{
		mContextConeOpacity = lerp(mContextConeOpacity, gSavedSettings.getF32("PickerContextOpacity"), LLCriticalDamp::getInterpolant(CONTEXT_FADE_TIME));
	}
	else
	{
		mContextConeOpacity = lerp(mContextConeOpacity, 0.f, LLCriticalDamp::getInterpolant(CONTEXT_FADE_TIME));
	}

	mPipetteBtn->setEnabled(gToolMgr != NULL);
	mPipetteBtn->setToggleState(gToolMgr && gToolMgr->getCurrentTool() == gToolPipette);
	mApplyImmediateCheck->setEnabled(mActive && mCanApplyImmediately);
	mSelectBtn->setEnabled(mActive);

	// base floater stuff
	LLFloater::draw ();

	// draw image for RGB area (not really RGB but you'll see what I mean...
	gl_draw_image ( mRGBViewerImageLeft, mRGBViewerImageTop - mRGBViewerImageHeight, mRGBImage, LLColor4::white );

	// update 'cursor' into RGB Section
	S32 xPos = ( S32 ) ( ( F32 )mRGBViewerImageWidth * getCurH () ) - 8;
	S32 yPos = ( S32 ) ( ( F32 )mRGBViewerImageHeight * getCurS () ) - 8;
	gl_line_2d ( mRGBViewerImageLeft + xPos,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 8,
				 mRGBViewerImageLeft + xPos + 16,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 8,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ) );

	gl_line_2d ( mRGBViewerImageLeft + xPos + 8,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos,
				 mRGBViewerImageLeft + xPos + 8,
				 mRGBViewerImageTop - mRGBViewerImageHeight + yPos + 16,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ) );

	// create rgb area outline
	gl_rect_2d ( mRGBViewerImageLeft,
				 mRGBViewerImageTop - mRGBViewerImageHeight,
				 mRGBViewerImageLeft + mRGBViewerImageWidth,
				 mRGBViewerImageTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );

	// draw luminance slider
	for ( S32 y = 0; y < mLumRegionHeight; ++y )
	{
		F32 rValSlider, gValSlider, bValSlider;
		hslToRgb ( getCurH (), getCurS (), ( F32 )y / ( F32 )mLumRegionHeight, rValSlider, gValSlider, bValSlider );

		gl_rect_2d( mLumRegionLeft, 
			mLumRegionTop - mLumRegionHeight + y, 
				mLumRegionLeft + mLumRegionWidth, 
					mLumRegionTop - mLumRegionHeight + y - 1, 
						LLColor4 ( rValSlider, gValSlider, bValSlider, 1.0f ) );
	}


	// draw luninance marker
	S32 startX = mLumRegionLeft + mLumRegionWidth;
	S32 startY = mLumRegionTop - mLumRegionHeight + ( S32 ) ( mLumRegionHeight * getCurL () );
	gl_triangle_2d ( startX, startY,
			startX + mLumMarkerSize, startY - mLumMarkerSize,
				startX + mLumMarkerSize, startY + mLumMarkerSize,
					LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ), TRUE );

	// draw luminance slider outline
	gl_rect_2d ( mLumRegionLeft,
				 mLumRegionTop - mLumRegionHeight,
				 mLumRegionLeft + mLumRegionWidth,
				 mLumRegionTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );

	// draw selected color swatch
	gl_rect_2d ( mSwatchRegionLeft,
				 mSwatchRegionTop - mSwatchRegionHeight,
				 mSwatchRegionLeft + mSwatchRegionWidth,
				 mSwatchRegionTop,
				 LLColor4 ( getCurR (), getCurG (), getCurB (), 1.0f ),
				 TRUE );

	// draw selected color swatch outline
	gl_rect_2d ( mSwatchRegionLeft,
				 mSwatchRegionTop - mSwatchRegionHeight,
				 mSwatchRegionLeft + mSwatchRegionWidth,
				 mSwatchRegionTop,
				 LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ),
				 FALSE );

	// color palette code is a little more involved so break it out into its' own method
	drawPalette ();
}

//////////////////////////////////////////////////////////////////////////////
// find a complimentary color to the one passed in that can be used to highlight
LLColor4&
LLFloaterColorPicker::
getComplimentaryColor ( LLColor4& backgroundColor )
{
	// going to base calculation on luminance
	F32 hVal, sVal, lVal;
	rgbToHsl ( backgroundColor [ 0 ], backgroundColor [ 1 ], backgroundColor [ 2 ], hVal, sVal, lVal );

	// fairly simple heuristic for now...!
	if ( lVal < 0.5f )
	{
		return LLColor4::white;
	}

	return LLColor4::black;
}

//////////////////////////////////////////////////////////////////////////////
// draw color palette
void
LLFloaterColorPicker::
drawPalette ()
{
	S32 curEntry = 0;

	for ( S32 y = 0; y < numPaletteRows; ++y )
	{
		for ( S32 x = 0; x < numPaletteColumns; ++x )
		{
			// calculate position
			S32 x1 = mPaletteRegionLeft + ( mPaletteRegionWidth * x ) / numPaletteColumns;
			S32 y1 = mPaletteRegionTop - ( mPaletteRegionHeight * y ) / numPaletteRows;
			S32 x2 = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( x + 1 ) ) / numPaletteColumns );
			S32 y2 = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( y + 1 ) ) / numPaletteRows );

			// draw palette entry color
			if ( mPalette [ curEntry ] )
			{
				gl_rect_2d ( x1 + 2, y1 - 2, x2 - 2, y2 + 2, *mPalette [ curEntry++ ], TRUE );
				gl_rect_2d ( x1 + 1, y1 - 1, x2 - 1, y2 + 1, LLColor4 ( 0.0f, 0.0f, 0.0f, 1.0f ), FALSE );
			}
		}
	}

	// if there is something to highlight (mouse down in swatch & hovering over palette)
	if ( highlightEntry >= 0 )
	{
		// extract row/column from palette index
		S32 entryColumn = highlightEntry % numPaletteColumns;
		S32 entryRow = highlightEntry / numPaletteColumns;

		// calculate position of this entry
		S32 x1 = mPaletteRegionLeft + ( mPaletteRegionWidth * entryColumn ) / numPaletteColumns;
		S32 y1 = mPaletteRegionTop - ( mPaletteRegionHeight * entryRow ) / numPaletteRows;
		S32 x2 = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( entryColumn + 1 ) ) / numPaletteColumns );
		S32 y2 = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( entryRow + 1 ) ) / numPaletteRows );

		// center position of entry
		S32 xCenter = x1 + ( x2 - x1 ) / 2;
		S32 yCenter = y1 - ( y1 - y2 ) / 2;

		// find a color that works well as a highlight color
		LLColor4 hlColor ( getComplimentaryColor ( *mPalette [ highlightEntry ] ) );

		// mark a cross for entry that is being hovered
		gl_line_2d ( xCenter - 4, yCenter - 4, xCenter + 4, yCenter + 4, hlColor );
		gl_line_2d ( xCenter + 4, yCenter - 4, xCenter - 4, yCenter + 4, hlColor );
	}
}

//////////////////////////////////////////////////////////////////////////////
// update text entry values for RGB/HSL (can't be done in ::draw () since this overwrites input
void
LLFloaterColorPicker::
updateTextEntry ()
{
	// set values in spinners
	childSetValue("rspin", ( getCurR () * 255.0f ) );
	childSetValue("gspin", ( getCurG () * 255.0f ) );
	childSetValue("bspin", ( getCurB () * 255.0f ) );
	childSetValue("hspin", ( getCurH () * 360.0f ) );
	childSetValue("sspin", ( getCurS () * 100.0f ) );
	childSetValue("lspin", ( getCurL () * 100.0f ) );
}

//////////////////////////////////////////////////////////////////////////////
// turns on or off text entry commit call backs
void
LLFloaterColorPicker::
enableTextCallbacks ( BOOL stateIn )
{
	if ( stateIn )
	{
		childSetCommitCallback("rspin", onTextCommit, (void*)this );
		childSetCommitCallback("gspin", onTextCommit, (void*)this );
		childSetCommitCallback("bspin", onTextCommit, (void*)this );
		childSetCommitCallback("hspin", onTextCommit, (void*)this );
		childSetCommitCallback("sspin", onTextCommit, (void*)this );
		childSetCommitCallback("lspin", onTextCommit, (void*)this );
	}
	else
	{
		childSetCommitCallback("rspin", 0, (void*)this );
		childSetCommitCallback("gspin", 0, (void*)this );
		childSetCommitCallback("bspin", 0, (void*)this );
		childSetCommitCallback("hspin", 0, (void*)this );
		childSetCommitCallback("sspin", 0, (void*)this );
		childSetCommitCallback("lspin", 0, (void*)this );
	}
}


//////////////////////////////////////////////////////////////////////////////
//
void
LLFloaterColorPicker::
onTextEntryChanged ( LLUICtrl* ctrl )
{
	// value in RGB boxes changed
	LLString name = ctrl->getName();
	if ( ( name == "rspin" ) || ( name == "gspin" ) || ( name == "bspin" ) )
	{
		// get current RGB
		F32 rVal, gVal, bVal;
		getCurRgb ( rVal, gVal, bVal );

		// update component value with new value from text
		if ( name == "rspin" )
		{
			rVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}
		else
		if ( name == "gspin" )
		{
			gVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}
		else
		if ( name == "bspin" )
		{
			bVal = (F32)ctrl->getValue().asReal() / 255.0f;
		}

		// update current RGB (and implicitly HSL)
		setCurRgb ( rVal, gVal, bVal );

		// HACK: turn off the call back wilst we update the text or we recurse ourselves into oblivion
		enableTextCallbacks ( FALSE );
		updateTextEntry ();
		enableTextCallbacks ( TRUE );
	}
	else
	// value in HSL boxes changed
	if ( ( name == "hspin" ) || ( name == "sspin" ) || ( name == "lspin" ) )
	{
		// get current HSL
		F32 hVal, sVal, lVal;
		getCurHsl ( hVal, sVal, lVal );

		// update component value with new value from text
		if ( name == "hspin" )
			hVal = (F32)ctrl->getValue().asReal() / 360.0f;
		else
		if ( name == "sspin" )
			sVal = (F32)ctrl->getValue().asReal() / 100.0f;
		else
		if ( name == "lspin" )
			lVal = (F32)ctrl->getValue().asReal() / 100.0f;

		// update current HSL (and implicitly RGB)
		setCurHsl ( hVal, sVal, lVal );

		// HACK: turn off the call back wilst we update the text or we recurse ourselves into oblivion
		enableTextCallbacks ( FALSE );
		updateTextEntry ();
		enableTextCallbacks ( TRUE );
	}

	if (mApplyImmediateCheck->get())
	{
		LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
	}
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL
LLFloaterColorPicker::
updateRgbHslFromPoint ( S32 xPosIn, S32 yPosIn )
{
	if ( xPosIn >= mRGBViewerImageLeft &&
		 xPosIn <= mRGBViewerImageLeft + mRGBViewerImageWidth &&
		 yPosIn <= mRGBViewerImageTop &&
		 yPosIn >= mRGBViewerImageTop - mRGBViewerImageHeight )
	{
		// update HSL (and therefore RGB) based on new H & S and current L
		setCurHsl ( ( ( F32 )xPosIn - ( F32 )mRGBViewerImageLeft ) / ( F32 )mRGBViewerImageWidth,
					( ( F32 )yPosIn - ( ( F32 )mRGBViewerImageTop - ( F32 )mRGBViewerImageHeight ) ) / ( F32 )mRGBViewerImageHeight,
					getCurL () );

		// indicate a value changed
		return TRUE;
	}
	else
	if ( xPosIn >= mLumRegionLeft &&
		 xPosIn <= mLumRegionLeft + mLumRegionWidth &&
		 yPosIn <= mLumRegionTop &&
		 yPosIn >= mLumRegionTop - mLumRegionHeight )
	{

		// update HSL (and therefore RGB) based on current HS and new L
		 setCurHsl ( getCurH (),
					 getCurS (),
					( ( F32 )yPosIn - ( ( F32 )mRGBViewerImageTop - ( F32 )mRGBViewerImageHeight ) ) / ( F32 )mRGBViewerImageHeight );

		// indicate a value changed
		return TRUE;
	}

	return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL
LLFloaterColorPicker::
handleMouseDown ( S32 x, S32 y, MASK mask )
{
	// if this window is in the foreground
	if ( mForeground )
	{
		// make it the frontmost
		gFloaterView->bringToFront(this);

		// rect containing RGB area
		LLRect rgbAreaRect ( mRGBViewerImageLeft,
							 mRGBViewerImageTop,
							 mRGBViewerImageLeft + mRGBViewerImageWidth,
							 mRGBViewerImageTop - mRGBViewerImageHeight );

		if ( rgbAreaRect.pointInRect ( x, y ) )
		{
			gViewerWindow->setMouseCapture(this, onMouseCaptureLost);
			// mouse button down
			setMouseDownInHueRegion ( TRUE );

			// update all values based on initial click
			updateRgbHslFromPoint ( x, y );

			// required by base class
			return TRUE;
		}

		// rect containing RGB area
		LLRect lumAreaRect ( mLumRegionLeft,
							 mLumRegionTop,
							 mLumRegionLeft + mLumRegionWidth + mLumMarkerSize,
							 mLumRegionTop - mLumRegionHeight );

		if ( lumAreaRect.pointInRect ( x, y ) )
		{
			gViewerWindow->setMouseCapture(this, onMouseCaptureLost);
			// mouse button down
			setMouseDownInLumRegion ( TRUE );

			// required by base class
			return TRUE;
		}

		// rect containing swatch area
		LLRect swatchRect ( mSwatchRegionLeft,
							mSwatchRegionTop,
							mSwatchRegionLeft + mSwatchRegionWidth,
							mSwatchRegionTop - mSwatchRegionHeight );

		setMouseDownInSwatch( FALSE );
		if ( swatchRect.pointInRect ( x, y ) )
		{
			setMouseDownInSwatch( TRUE );

			// required - dont drag windows here.
			return TRUE;
		}

		// rect containing palette area
		LLRect paletteRect ( mPaletteRegionLeft,
							 mPaletteRegionTop,
							 mPaletteRegionLeft + mPaletteRegionWidth,
							 mPaletteRegionTop - mPaletteRegionHeight );

		if ( paletteRect.pointInRect ( x, y ) )
		{
			// release keyboard focus so we can change text values
			if (gFocusMgr.childHasKeyboardFocus(this))
			{
				mSelectBtn->setFocus(TRUE);
			}

			// calculate which palette index we selected
			S32 c = ( ( x - mPaletteRegionLeft ) * numPaletteColumns ) / mPaletteRegionWidth;
			S32 r = ( ( y - ( mPaletteRegionTop - mPaletteRegionHeight ) ) * numPaletteRows ) / mPaletteRegionHeight;

			U32 index = ( numPaletteRows - r - 1 ) * numPaletteColumns + c;

			if ( index <= mPalette.size () )
			{
				LLColor4 selected = *mPalette [ index ];

				setCurRgb ( selected [ 0 ], selected [ 1 ], selected [ 2 ] );

				if (mApplyImmediateCheck->get())
				{
					LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
				}

				// HACK: turn off the call back wilst we update the text or we recurse ourselves into oblivion
				enableTextCallbacks ( FALSE );
				updateTextEntry ();
				enableTextCallbacks ( TRUE );
			}

			return TRUE;
		}
	}
	// dispatch to base class for the rest of things
	return LLFloater::handleMouseDown ( x, y, mask );
}

//////////////////////////////////////////////////////////////////////////////
//
BOOL
LLFloaterColorPicker::
handleHover ( S32 x, S32 y, MASK mask )
{
	// if we're the front most window
	if ( isFrontmost () )
	{
		// mouse was pressed within region
		if ( getMouseDownInHueRegion() || getMouseDownInLumRegion())
		{
			S32 clamped_x, clamped_y;
			if (getMouseDownInHueRegion())
			{
				clamped_x = llclamp(x, mRGBViewerImageLeft, mRGBViewerImageLeft + mRGBViewerImageWidth);
				clamped_y = llclamp(y, mRGBViewerImageTop - mRGBViewerImageHeight, mRGBViewerImageTop);
			}
			else
			{
				clamped_x = llclamp(x, mLumRegionLeft, mLumRegionLeft + mLumRegionWidth);
				clamped_y = llclamp(y, mLumRegionTop - mLumRegionHeight, mLumRegionTop);
			}

			// update the stored RGB/HSL values using the mouse position - returns TRUE if RGB was updated
			if ( updateRgbHslFromPoint ( clamped_x, clamped_y ) )
			{
				// update text entry fields
				updateTextEntry ();

				// RN: apparently changing color when dragging generates too much traffic and results in sporadic updates
				//// commit changed color to swatch subject
				//// REVIEW: this gets sent each time a color changes - is this okay ?
				//if (mApplyImmediateCheck->get())
				//{
				//	LLColorSwatchCtrl::onColorChanged ( getSwatch () );
				//}
			}
		}

		highlightEntry = -1;

		if ( mMouseDownInSwatch )
		{
			getWindow()->setCursor ( UI_CURSOR_ARROWDRAG );

			// if cursor if over a palette entry
			LLRect paletteRect ( mPaletteRegionLeft,
								mPaletteRegionTop,
								mPaletteRegionLeft + mPaletteRegionWidth,
								mPaletteRegionTop - mPaletteRegionHeight );

			if ( paletteRect.pointInRect ( x, y ) )
			{
				// find row/column in palette
				S32 xOffset = ( ( x - mPaletteRegionLeft ) * numPaletteColumns ) / mPaletteRegionWidth;
				S32 yOffset = ( ( mPaletteRegionTop - y - 1 ) * numPaletteRows ) / mPaletteRegionHeight;

				// calculate the entry 0..n-1 to highlight and set variable to next draw() picks it up
				highlightEntry = xOffset + yOffset * numPaletteColumns;
			}

			return TRUE;
		}
	}

	// dispatch to base class for the rest of things
	return LLFloater::handleHover ( x, y, mask );
}

void LLFloaterColorPicker::onClose(bool app_quitting)
{
	//RN: this is consistent with texture picker in that closing the window leaves the current selection
	// to change this to "close to cancel", uncomment the following line
	//cancelSelection();
	LLFloater::onClose(app_quitting);
}

//////////////////////////////////////////////////////////////////////////////
// reverts state once mouse button is released
BOOL
LLFloaterColorPicker::
handleMouseUp ( S32 x, S32 y, MASK mask )
{
	getWindow()->setCursor ( UI_CURSOR_ARROW );

	if (getMouseDownInHueRegion() || getMouseDownInLumRegion())
	{
		if (mApplyImmediateCheck->get())
		{
			LLColorSwatchCtrl::onColorChanged ( getSwatch (), LLColorSwatchCtrl::COLOR_CHANGE );
		}
	}

	// rect containing palette area
	LLRect paletteRect ( mPaletteRegionLeft,
							mPaletteRegionTop,
							mPaletteRegionLeft + mPaletteRegionWidth,
							mPaletteRegionTop - mPaletteRegionHeight );

	if ( paletteRect.pointInRect ( x, y ) )
	{
		if ( mMouseDownInSwatch )
		{
			S32 curEntry = 0;
			for ( S32 row = 0; row < numPaletteRows; ++row )
			{
				for ( S32 column = 0; column < numPaletteColumns; ++column )
				{
					S32 left = mPaletteRegionLeft + ( mPaletteRegionWidth * column ) / numPaletteColumns;
					S32 top = mPaletteRegionTop - ( mPaletteRegionHeight * row ) / numPaletteRows;
					S32 right = ( mPaletteRegionLeft + ( mPaletteRegionWidth * ( column + 1 ) ) / numPaletteColumns );
					S32 bottom = ( mPaletteRegionTop - ( mPaletteRegionHeight * ( row + 1 ) ) / numPaletteRows );

					// rect is flipped vertically when testing here
					LLRect dropRect ( left, top, right, bottom );

					if ( dropRect.pointInRect ( x, y ) )
					{
						if ( mPalette [ curEntry ] )
						{
							delete mPalette [ curEntry ];

							mPalette [ curEntry ] = new LLColor4 ( getCurR (), getCurG (), getCurB (), 1.0f );

							// save off color
							std::ostringstream codec;
							codec << "ColorPaletteEntry" << std::setfill ( '0' ) << std::setw ( 2 ) << curEntry + 1;
							const std::string s ( codec.str () );
							gSavedSettings.setColor4( s, *mPalette [ curEntry ] );
						}
					}

					++curEntry;
				}
			}
		}
	}

	// mouse button not down anymore
	setMouseDownInHueRegion ( FALSE );
	setMouseDownInLumRegion ( FALSE );

	// mouse button not down in color swatch anymore
	mMouseDownInSwatch = false;

	if (gViewerWindow->hasMouseCapture(this))
	{
		gViewerWindow->setMouseCapture(NULL, NULL);
	}

	// dispatch to base class for the rest of things
	return LLFloater::handleMouseUp ( x, y, mask );
}

//////////////////////////////////////////////////////////////////////////////
// cancel current color selection, revert to original and close picker
void
LLFloaterColorPicker::
cancelSelection ()
{
	// restore the previous colour selection
	setCurRgb ( getOrigR (), getOrigG (), getOrigB () );

	// 	we're going away and when we do and the entry widgets lose focus, they do bad things so turn them off
	enableTextCallbacks ( FALSE );

	// update in world item with original color via current swatch
	LLColorSwatchCtrl::onColorChanged( getSwatch(), LLColorSwatchCtrl::COLOR_CANCEL );

	// hide picker dialog
	this->setVisible ( FALSE );
}

void LLFloaterColorPicker::setMouseDownInHueRegion ( BOOL mouse_down_in_region )
{
	mMouseDownInHueRegion = mouse_down_in_region;
	if (mouse_down_in_region)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			// get focus out of spinners so that they can update freely
			mSelectBtn->setFocus(TRUE);
		}
	}
}

void LLFloaterColorPicker::setMouseDownInLumRegion ( BOOL mouse_down_in_region )
{
	mMouseDownInLumRegion = mouse_down_in_region;
	if (mouse_down_in_region)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			// get focus out of spinners so that they can update freely
			mSelectBtn->setFocus(TRUE);
		}
	}
}

void LLFloaterColorPicker::setMouseDownInSwatch (BOOL mouse_down_in_swatch)
{
	mMouseDownInSwatch = mouse_down_in_swatch;
	if (mouse_down_in_swatch)
	{
		if (gFocusMgr.childHasKeyboardFocus(this))
		{
			// get focus out of spinners so that they can update freely
			mSelectBtn->setFocus(TRUE);
		}
	}
}

void LLFloaterColorPicker::setActive(BOOL active) 
{ 
	// shut down pipette tool if active
	if (!active && mPipetteBtn->getToggleState())
	{
		stopUsingPipette();
	}
	mActive = active; 
}

void LLFloaterColorPicker::stopUsingPipette()
{
	if (gToolMgr && gToolMgr->getCurrentTool() == gToolPipette)
	{
		gToolMgr->clearTransientTool();
	}
}
