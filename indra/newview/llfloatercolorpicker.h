/** 
 * @file llfloatercolorpicker.h
 * @brief Generic system color picker
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
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

#ifndef LL_LLFLOATERCOLORPICKER_H
#define LL_LLFLOATERCOLORPICKER_H

#include <vector>

#include "llfloater.h"
#include "llpointer.h"
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
		virtual F32  getSwatchTransparency();

		// implicit methods
		void createUI ();
		void initUI ( F32 rValIn, F32 gValIn, F32 bValIn );
		void showUI ();
		void destroyUI ();
		void cancelSelection ();
		LLColorSwatchCtrl* getSwatch () { return mSwatch; };
		void setSwatch( LLColorSwatchCtrl* swatch) { mSwatch = swatch; }

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
			   void onClickPipette ( );
		static void onTextCommit ( LLUICtrl* ctrl, void* data );
		static void onImmediateCheck ( LLUICtrl* ctrl, void* data );
			   void onColorSelect( const LLTextureEntry& te );
	private:
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
		LLPointer<LLViewerTexture> mRGBImage;

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
