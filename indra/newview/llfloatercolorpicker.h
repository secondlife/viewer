/** 
 * @file llfloatercolorpicker.h
 * @brief Generic system color picker
 *
 * $LicenseInfo:firstyear=2004&license=viewergpl$
 * 
 * Copyright (c) 2004-2007, Linden Research, Inc.
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

#ifndef LL_LLFLOATERCOLORPICKER_H
#define LL_LLFLOATERCOLORPICKER_H

#include <vector>

#include "llfloater.h"
#include "llmemory.h"
#include "llcolorswatch.h"
#include "llspinctrl.h"
#include "lltextureentry.h"

class LLButton;
class LLLineEditor;
class LLCheckBoxCtrl;

//////////////////////////////////////////////////////////////////////////////
// floater class
class LLFloaterColorPicker 
	: public LLFloater
{
	public:
		LLFloaterColorPicker (LLColorSwatchCtrl* swatch, BOOL show_apply_immediate = FALSE);
		virtual ~LLFloaterColorPicker ();

		// overrides
		virtual BOOL postBuild ();
		virtual void draw ();
		virtual BOOL handleMouseDown ( S32 x, S32 y, MASK mask );
		virtual BOOL handleMouseUp ( S32 x, S32 y, MASK mask );
		virtual BOOL handleHover ( S32 x, S32 y, MASK mask );
		virtual void onMouseCaptureLost();
		virtual void onClose(bool app_quitting);

		// implicit methods
		void createUI ();
		void initUI ( F32 rValIn, F32 gValIn, F32 bValIn );
		void showUI ();
		void destroyUI ();
		void cancelSelection ();
		LLColorSwatchCtrl* getSwatch () { return mSwatch; };

		// mutator / accessor for original RGB value
		void setOrigRgb ( F32 origRIn, F32 origGIn, F32 origBIn );
		void getOrigRgb ( F32& origROut, F32& origGOut, F32& origBOut );
		F32 getOrigR () { return origR; };
		F32 getOrigG () { return origG; };
		F32 getOrigB () { return origB; };

		// mutator / accessors for currernt RGB value
		void setCurRgb ( F32 curRIn, F32 curGIn, F32 curBIn );
		void getCurRgb ( F32& curROut, F32& curGOut, F32& curBOut );
		F32	 getCurR () { return curR; };
		F32	 getCurG () { return curG; };
		F32	 getCurB () { return curB; };

		// mutator / accessors for currernt HSL value
		void setCurHsl ( F32 curHIn, F32 curSIn, F32 curLIn );
		void getCurHsl ( F32& curHOut, F32& curSOut, F32& curLOut );
		F32	 getCurH () { return curH; };
		F32	 getCurS () { return  curS; };
		F32	 getCurL () { return curL; };

		// updates current RGB/HSL values based on point in picker
		BOOL updateRgbHslFromPoint ( S32 xPosIn, S32 yPosIn );

		// updates text entry fields with current RGB/HSL
		void updateTextEntry ();

		void stopUsingPipette();

		// mutator / accessor for mouse button pressed in region
		void setMouseDownInHueRegion ( BOOL mouse_down_in_region );
		BOOL getMouseDownInHueRegion () { return mMouseDownInHueRegion; };

		void setMouseDownInLumRegion ( BOOL mouse_down_in_region );
		BOOL getMouseDownInLumRegion () { return mMouseDownInLumRegion; };

		void setMouseDownInSwatch (BOOL mouse_down_in_swatch);
		BOOL getMouseDownInSwatch () { return mMouseDownInSwatch; }

		// called when text entries (RGB/HSL etc.) are changed by user
		void onTextEntryChanged ( LLUICtrl* ctrl );

		// convert RGB to HSL and vice-versa
		void hslToRgb ( F32 hValIn, F32 sValIn, F32 lValIn, F32& rValOut, F32& gValOut, F32& bValOut );
		F32	 hueToRgb ( F32 val1In, F32 val2In, F32 valHUeIn );

		void setActive(BOOL active);

	protected:
		// callbacks
		static void onClickCancel ( void* data );
		static void onClickSelect ( void* data );
		static void onClickPipette ( void* data );
		static void onTextCommit ( LLUICtrl* ctrl, void* data );
		static void onImmediateCheck ( LLUICtrl* ctrl, void* data );
		static void onColorSelect( const LLTextureEntry& te, void *data );
	private:
		// turns on or off text entry commit call backs
		void enableTextCallbacks ( BOOL stateIn );

		// draws color selection palette
		void drawPalette ();

		// find a complimentary color to the one passed in that can be used to highlight 
		const LLColor4& getComplimentaryColor ( const LLColor4& backgroundColor );

		// original RGB values
		F32 origR, origG, origB;

		// current RGB/HSL values
		F32 curR, curG, curB;
		F32 curH, curS, curL;

		const S32 mComponents;

		BOOL mMouseDownInLumRegion;
		BOOL mMouseDownInHueRegion;
		BOOL mMouseDownInSwatch;

		const S32 mRGBViewerImageLeft;
		const S32 mRGBViewerImageTop;
		const S32 mRGBViewerImageWidth;
		const S32 mRGBViewerImageHeight;

		const S32 mLumRegionLeft;
		const S32 mLumRegionTop;
		const S32 mLumRegionWidth;
		const S32 mLumRegionHeight;
		const S32 mLumMarkerSize;

		// Preview of the current color.
		const S32 mSwatchRegionLeft;
		const S32 mSwatchRegionTop;
		const S32 mSwatchRegionWidth;
		const S32 mSwatchRegionHeight;

		LLView* mSwatchView;

		const S32 numPaletteColumns;
		const S32 numPaletteRows;
        std::vector < LLColor4* > mPalette;
		S32 highlightEntry;
		const S32 mPaletteRegionLeft;
		const S32 mPaletteRegionTop;
		const S32 mPaletteRegionWidth;
		const S32 mPaletteRegionHeight;

		// image used to compose color grid
		LLPointer<LLImageGL> mRGBImage;

		// current swatch in use
		LLColorSwatchCtrl* mSwatch;

		// are we actively tied to some output?
		BOOL	mActive;

		// enable/disable immediate updates
		LLCheckBoxCtrl* mApplyImmediateCheck;
		BOOL mCanApplyImmediately;

		LLButton* mSelectBtn;
		LLButton* mCancelBtn;

		LLButton* mPipetteBtn;

		F32		  mContextConeOpacity;
};

#endif // LL_LLFLOATERCOLORPICKER_H
