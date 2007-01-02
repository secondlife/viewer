/** 
 * @file llmanip.h
 * @brief LLManip class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_MANIP_H
#define LL_MANIP_H

#include "lltool.h"
//#include "v3math.h"

class LLView;
class LLTextBox;
class LLViewerObject;
class LLToolComposite;
class LLVector3;

const S32 MIN_DIVISION_PIXEL_WIDTH = 9;

class LLManip : public LLTool
{
public:
	typedef enum e_manip_part
	{
		LL_NO_PART = 0,

		// Translation
		LL_X_ARROW,
		LL_Y_ARROW,
		LL_Z_ARROW,

		LL_YZ_PLANE,
		LL_XZ_PLANE,
		LL_XY_PLANE,

		// Scale
		LL_CORNER_NNN,
		LL_CORNER_NNP,		
		LL_CORNER_NPN,
		LL_CORNER_NPP,
		LL_CORNER_PNN,
		LL_CORNER_PNP,
		LL_CORNER_PPN,
		LL_CORNER_PPP,

		// Faces
		LL_FACE_POSZ,
		LL_FACE_POSX,
		LL_FACE_POSY,
		LL_FACE_NEGX,
		LL_FACE_NEGY,
		LL_FACE_NEGZ,

		// Edges
		LL_EDGE_NEGX_NEGY,
		LL_EDGE_NEGX_POSY,
		LL_EDGE_POSX_NEGY,
		LL_EDGE_POSX_POSY,
		
		LL_EDGE_NEGY_NEGZ,
		LL_EDGE_NEGY_POSZ,
		LL_EDGE_POSY_NEGZ,
		LL_EDGE_POSY_POSZ,

		LL_EDGE_NEGZ_NEGX,
		LL_EDGE_NEGZ_POSX,
		LL_EDGE_POSZ_NEGX,
		LL_EDGE_POSZ_POSX,

		// Rotation Manip
		LL_ROT_GENERAL,
		LL_ROT_X,
		LL_ROT_Y,
		LL_ROT_Z,
		LL_ROT_ROLL
	} EManipPart;

	// For use in loops and range checking.
	typedef enum e_select_part_ranges
	{
		LL_ARROW_MIN = LL_X_ARROW,
		LL_ARROW_MAX = LL_Z_ARROW,

		LL_CORNER_MIN = LL_CORNER_NNN,
		LL_CORNER_MAX = LL_CORNER_PPP,
		
		LL_FACE_MIN	  = LL_FACE_POSZ,
		LL_FACE_MAX	  = LL_FACE_NEGZ,
		
		LL_EDGE_MIN   = LL_EDGE_NEGX_NEGY,
		LL_EDGE_MAX   = LL_EDGE_POSZ_POSX
	} EManipPartRanges;
public:
	static void rebuild(LLViewerObject* vobj);
	
	LLManip( const LLString& name, LLToolComposite* composite );

	virtual BOOL		handleMouseDownOnPart(S32 x, S32 y, MASK mask) = 0;
	void				renderGuidelines(BOOL draw_x = TRUE, BOOL draw_y = TRUE, BOOL draw_z = TRUE);
	static void			renderXYZ(const LLVector3 &vec);

    virtual BOOL		handleMouseDown(S32 x, S32 y, MASK mask) = 0;
    /*virtual*/ BOOL	handleMouseUp(S32 x, S32 y, MASK mask);
    /*virtual*/ BOOL	handleHover(S32 x, S32 y, MASK mask);
	virtual EManipPart	getHighlightedPart() { return LL_NO_PART; }
	virtual void		highlightManipulators(S32 x, S32 y) {};
protected:
	LLVector3			getSavedPivotPoint() const;
	LLVector3			getPivotPoint() const;
	void				getManipNormal(LLViewerObject* object, EManipPart manip, LLVector3 &normal);
	BOOL				getManipAxis(LLViewerObject* object, EManipPart manip, LLVector3 &axis);
	F32					getSubdivisionLevel(const LLVector3 &reference_point, const LLVector3 &translate_axis, F32 grid_scale, S32 min_pixel_spacing = MIN_DIVISION_PIXEL_WIDTH);
	void				renderTickValue(const LLVector3& pos, F32 value, const char* suffix, const LLColor4 &color);
	void				renderTickText(const LLVector3& pos, const char* suffix, const LLColor4 &color);
	void				updateGridSettings();
	BOOL				getMousePointOnPlaneGlobal(LLVector3d& point, S32 x, S32 y, LLVector3d origin, LLVector3 normal);
	BOOL				getMousePointOnPlaneAgent(LLVector3& point, S32 x, S32 y, LLVector3 origin, LLVector3 normal);
	BOOL				nearestPointOnLineFromMouse( S32 x, S32 y, const LLVector3& b1, const LLVector3& b2, F32 &a_param, F32 &b_param ) const;
	LLColor4			setupSnapGuideRenderPass(S32 pass);
protected:
	LLFrameTimer		mHelpTextTimer;
	BOOL				mInSnapRegime;

	static F32			sHelpTextVisibleTime;
	static F32			sHelpTextFadeTime;
	static S32			sNumTimesHelpTextShown;
	static S32			sMaxTimesShowHelpText;
	static F32			sGridMaxSubdivisionLevel;
	static F32			sGridMinSubdivisionLevel;
	static LLVector2	sTickLabelSpacing;
};


#endif
