/** 
 * @file llfloaterbvhpreview.cpp
 * @brief LLFloaterBvhPreview class implementation
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

#include "llviewerprecompiledheaders.h"

#include "llfloaterbvhpreview.h"

#include "llbvhloader.h"
#include "lldatapacker.h"
#include "lldir.h"
#include "lleconomy.h"
#include "llnotificationsutil.h"
#include "llvfile.h"
#include "llapr.h"
#include "llstring.h"

#include "llagent.h"
#include "llanimationstates.h"
#include "llbbox.h"
#include "llbutton.h"
#include "llcheckboxctrl.h"
#include "llcombobox.h"
#include "lldrawable.h"
#include "lldrawpoolavatar.h"
#include "llrender.h"
#include "llface.h"
#include "llfocusmgr.h"
#include "llkeyframemotion.h"
#include "lllineeditor.h"
#include "llfloaterperms.h"
#include "llsliderctrl.h"
#include "llspinctrl.h"
#include "lltextbox.h"
#include "lltoolmgr.h"
#include "llui.h"
#include "llviewercamera.h"
#include "llviewerobjectlist.h"
#include "llviewerwindow.h"
#include "llviewermenufile.h"	// upload_new_resource()
#include "llvoavatar.h"
#include "pipeline.h"
#include "lluictrlfactory.h"
#include "lltrans.h"

const S32 PREVIEW_BORDER_WIDTH = 2;
const S32 PREVIEW_RESIZE_HANDLE_SIZE = S32(RESIZE_HANDLE_WIDTH * OO_SQRT2) + PREVIEW_BORDER_WIDTH;
const S32 PREVIEW_HPAD = PREVIEW_RESIZE_HANDLE_SIZE;
const S32 PREF_BUTTON_HEIGHT = 16;
const S32 PREVIEW_TEXTURE_HEIGHT = 300;

const F32 PREVIEW_CAMERA_DISTANCE = 4.f;

const F32 MIN_CAMERA_ZOOM = 0.5f;
const F32 MAX_CAMERA_ZOOM = 10.f;

const F32 BASE_ANIM_TIME_OFFSET = 5.f;

std::string STATUS[] =
{
	"E_ST_OK",
	"E_ST_EOF",
	"E_ST_NO_CONSTRAINT",
	"E_ST_NO_FILE",
	"E_ST_NO_HIER",
	"E_ST_NO_JOINT",
	"E_ST_NO_NAME",
	"E_ST_NO_OFFSET",
	"E_ST_NO_CHANNELS",
	"E_ST_NO_ROTATION",
	"E_ST_NO_AXIS",
	"E_ST_NO_MOTION",
	"E_ST_NO_FRAMES",
	"E_ST_NO_FRAME_TIME",
	"E_ST_NO_POS",
	"E_ST_NO_ROT",
	"E_ST_NO_XLT_FILE",
	"E_ST_NO_XLT_HEADER",
	"E_ST_NO_XLT_NAME",
	"E_ST_NO_XLT_IGNORE",
	"E_ST_NO_XLT_RELATIVE",
	"E_ST_NO_XLT_OUTNAME",
	"E_ST_NO_XLT_MATRIX",
	"E_ST_NO_XLT_MERGECHILD",
	"E_ST_NO_XLT_MERGEPARENT",
	"E_ST_NO_XLT_PRIORITY",
	"E_ST_NO_XLT_LOOP",
	"E_ST_NO_XLT_EASEIN",
	"E_ST_NO_XLT_EASEOUT",
	"E_ST_NO_XLT_HAND",
	"E_ST_NO_XLT_EMOTE",
"E_ST_BAD_ROOT"
};

//-----------------------------------------------------------------------------
// LLFloaterBvhPreview()
//-----------------------------------------------------------------------------
LLFloaterBvhPreview::LLFloaterBvhPreview(const std::string& filename) : 
	LLFloaterNameDesc(filename)
{
	mLastMouseX = 0;
	mLastMouseY = 0;

	mIDList["Standing"] = ANIM_AGENT_STAND;
	mIDList["Walking"] = ANIM_AGENT_FEMALE_WALK;
	mIDList["Sitting"] = ANIM_AGENT_SIT_FEMALE;
	mIDList["Flying"] = ANIM_AGENT_HOVER;

	mIDList["[None]"] = LLUUID::null;
	mIDList["Aaaaah"] = ANIM_AGENT_EXPRESS_OPEN_MOUTH;
	mIDList["Afraid"] = ANIM_AGENT_EXPRESS_AFRAID;
	mIDList["Angry"] = ANIM_AGENT_EXPRESS_ANGER;
	mIDList["Big Smile"] = ANIM_AGENT_EXPRESS_TOOTHSMILE;
	mIDList["Bored"] = ANIM_AGENT_EXPRESS_BORED;
	mIDList["Cry"] = ANIM_AGENT_EXPRESS_CRY;
	mIDList["Disdain"] = ANIM_AGENT_EXPRESS_DISDAIN;
	mIDList["Embarrassed"] = ANIM_AGENT_EXPRESS_EMBARRASSED;
	mIDList["Frown"] = ANIM_AGENT_EXPRESS_FROWN;
	mIDList["Kiss"] = ANIM_AGENT_EXPRESS_KISS;
	mIDList["Laugh"] = ANIM_AGENT_EXPRESS_LAUGH;
	mIDList["Plllppt"] = ANIM_AGENT_EXPRESS_TONGUE_OUT;
	mIDList["Repulsed"] = ANIM_AGENT_EXPRESS_REPULSED;
	mIDList["Sad"] = ANIM_AGENT_EXPRESS_SAD;
	mIDList["Shrug"] = ANIM_AGENT_EXPRESS_SHRUG;
	mIDList["Smile"] = ANIM_AGENT_EXPRESS_SMILE;
	mIDList["Surprise"] = ANIM_AGENT_EXPRESS_SURPRISE;
	mIDList["Wink"] = ANIM_AGENT_EXPRESS_WINK;
	mIDList["Worry"] = ANIM_AGENT_EXPRESS_WORRY;
}

//-----------------------------------------------------------------------------
// setAnimCallbacks()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::setAnimCallbacks()
{
	getChild<LLUICtrl>("playback_slider")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onSliderMove, this));
	
	getChild<LLUICtrl>("preview_base_anim")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitBaseAnim, this));
	getChild<LLUICtrl>("preview_base_anim")->setValue("Standing");

	getChild<LLUICtrl>("priority")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitPriority, this));
	getChild<LLUICtrl>("loop_check")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitLoop, this));
	getChild<LLUICtrl>("loop_in_point")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitLoopIn, this));
	getChild<LLUICtrl>("loop_in_point")->setValidateBeforeCommit( boost::bind(&LLFloaterBvhPreview::validateLoopIn, this, _1));
	getChild<LLUICtrl>("loop_out_point")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitLoopOut, this));
	getChild<LLUICtrl>("loop_out_point")->setValidateBeforeCommit( boost::bind(&LLFloaterBvhPreview::validateLoopOut, this, _1));

	getChild<LLUICtrl>("hand_pose_combo")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitHandPose, this));
	
	getChild<LLUICtrl>("emote_combo")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitEmote, this));
	getChild<LLUICtrl>("emote_combo")->setValue("[None]");

	getChild<LLUICtrl>("ease_in_time")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitEaseIn, this));
	getChild<LLUICtrl>("ease_in_time")->setValidateBeforeCommit( boost::bind(&LLFloaterBvhPreview::validateEaseIn, this, _1));
	getChild<LLUICtrl>("ease_out_time")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitEaseOut, this));
	getChild<LLUICtrl>("ease_out_time")->setValidateBeforeCommit( boost::bind(&LLFloaterBvhPreview::validateEaseOut, this, _1));
}

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------
BOOL LLFloaterBvhPreview::postBuild()
{
	LLKeyframeMotion* motionp = NULL;
	LLBVHLoader* loaderp = NULL;

	if (!LLFloaterNameDesc::postBuild())
	{
		return FALSE;
	}

	getChild<LLUICtrl>("name_form")->setCommitCallback(boost::bind(&LLFloaterBvhPreview::onCommitName, this));

	childSetAction("ok_btn", onBtnOK, this);
	setDefaultBtn();

	mPreviewRect.set(PREVIEW_HPAD, 
		PREVIEW_TEXTURE_HEIGHT,
		getRect().getWidth() - PREVIEW_HPAD, 
		PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
	mPreviewImageRect.set(0.f, 1.f, 1.f, 0.f);

	mPlayButton = getChild<LLButton>( "play_btn");
	mPlayButton->setClickedCallback(boost::bind(&LLFloaterBvhPreview::onBtnPlay, this));
	mPlayButton->setVisible(true);

	mPauseButton = getChild<LLButton>( "pause_btn");
	mPauseButton->setClickedCallback(boost::bind(&LLFloaterBvhPreview::onBtnPause, this));
	mPauseButton->setVisible(false);
	
	mStopButton = getChild<LLButton>( "stop_btn");
	mStopButton->setClickedCallback(boost::bind(&LLFloaterBvhPreview::onBtnStop, this));

	getChildView("bad_animation_text")->setVisible(FALSE);

	std::string exten = gDirUtilp->getExtension(mFilename);
	if (exten == "bvh")
	{
		// loading a bvh file

		// now load bvh file
		S32 file_size;
		
		LLAPRFile infile ;
		infile.open(mFilenameAndPath, LL_APR_RB, NULL, &file_size);
		
		if (!infile.getFileHandle())
		{
			llwarns << "Can't open BVH file:" << mFilename << llendl;	
		}
		else
		{
			char*	file_buffer;

			file_buffer = new char[file_size + 1];

			if (file_size == infile.read(file_buffer, file_size))
			{
				file_buffer[file_size] = '\0';
				llinfos << "Loading BVH file " << mFilename << llendl;
				ELoadStatus load_status = E_ST_OK;
				S32 line_number = 0; 
				loaderp = new LLBVHLoader(file_buffer, load_status, line_number);
				std::string status = getString(STATUS[load_status]);
				
				if(load_status == E_ST_NO_XLT_FILE)
				{
					llwarns << "NOTE: No translation table found." << llendl;
				}
				else
				{
					llwarns << "ERROR: [line: " << line_number << "] " << status << llendl;
				}
			}

			infile.close() ;
			delete[] file_buffer;
		}
	}

	if (loaderp && loaderp->isInitialized() && loaderp->getDuration() <= MAX_ANIM_DURATION)
	{
		// generate unique id for this motion
		mTransactionID.generate();
		mMotionID = mTransactionID.makeAssetID(gAgent.getSecureSessionID());

		mAnimPreview = new LLPreviewAnimation(256, 256);

		// motion will be returned, but it will be in a load-pending state, as this is a new motion
		// this motion will not request an asset transfer until next update, so we have a chance to 
		// load the keyframe data locally
		motionp = (LLKeyframeMotion*)mAnimPreview->getDummyAvatar()->createMotion(mMotionID);

		// create data buffer for keyframe initialization
		S32 buffer_size = loaderp->getOutputSize();
		U8* buffer = new U8[buffer_size];

		LLDataPackerBinaryBuffer dp(buffer, buffer_size);

		// pass animation data through memory buffer
		loaderp->serialize(dp);
		dp.reset();
		BOOL success = motionp && motionp->deserialize(dp);

		delete []buffer;

		if (success)
		{
			setAnimCallbacks() ;
			
			const LLBBoxLocal &pelvis_bbox = motionp->getPelvisBBox();

			LLVector3 temp = pelvis_bbox.getCenter();
			// only consider XY?
			//temp.mV[VZ] = 0.f;
			F32 pelvis_offset = temp.magVec();

			temp = pelvis_bbox.getExtent();
			//temp.mV[VZ] = 0.f;
			F32 pelvis_max_displacement = pelvis_offset + (temp.magVec() * 0.5f) + 1.f;
			
			F32 camera_zoom = LLViewerCamera::getInstance()->getDefaultFOV() / (2.f * atan(pelvis_max_displacement / PREVIEW_CAMERA_DISTANCE));
		
			mAnimPreview->setZoom(camera_zoom);

			motionp->setName(getChild<LLUICtrl>("name_form")->getValue().asString());
			mAnimPreview->getDummyAvatar()->startMotion(mMotionID);
			
			getChild<LLSlider>("playback_slider")->setMinValue(0.0);
			getChild<LLSlider>("playback_slider")->setMaxValue(1.0);

			getChild<LLUICtrl>("loop_check")->setValue(LLSD(motionp->getLoop()));
			getChild<LLUICtrl>("loop_in_point")->setValue(LLSD(motionp->getLoopIn() / motionp->getDuration() * 100.f));
			getChild<LLUICtrl>("loop_out_point")->setValue(LLSD(motionp->getLoopOut() / motionp->getDuration() * 100.f));
			getChild<LLUICtrl>("priority")->setValue(LLSD((F32)motionp->getPriority()));
			getChild<LLUICtrl>("hand_pose_combo")->setValue(LLHandMotion::getHandPoseName(motionp->getHandPose()));
			getChild<LLUICtrl>("ease_in_time")->setValue(LLSD(motionp->getEaseInDuration()));
			getChild<LLUICtrl>("ease_out_time")->setValue(LLSD(motionp->getEaseOutDuration()));
			setEnabled(TRUE);
			std::string seconds_string;
			seconds_string = llformat(" - %.2f seconds", motionp->getDuration());

			setTitle(mFilename + std::string(seconds_string));
		}
		else
		{
			mAnimPreview = NULL;
			mMotionID.setNull();
			getChild<LLUICtrl>("bad_animation_text")->setValue(getString("failed_to_initialize"));
		}
	}
	else
	{
		if ( loaderp )
		{
			if (loaderp->getDuration() > MAX_ANIM_DURATION)
			{
				LLUIString out_str = getString("anim_too_long");
				out_str.setArg("[LENGTH]", llformat("%.1f", loaderp->getDuration()));
				out_str.setArg("[MAX_LENGTH]", llformat("%.1f", MAX_ANIM_DURATION));
				getChild<LLUICtrl>("bad_animation_text")->setValue(out_str.getString());
			}
			else
			{
				LLUIString out_str = getString("failed_file_read");
				out_str.setArg("[STATUS]", getString(STATUS[loaderp->getStatus()])); 
				getChild<LLUICtrl>("bad_animation_text")->setValue(out_str.getString());
			}
		}

		//setEnabled(FALSE);
		mMotionID.setNull();
		mAnimPreview = NULL;
	}

	refresh();

	delete loaderp;

	return TRUE;
}

//-----------------------------------------------------------------------------
// LLFloaterBvhPreview()
//-----------------------------------------------------------------------------
LLFloaterBvhPreview::~LLFloaterBvhPreview()
{
	mAnimPreview = NULL;

	setEnabled(FALSE);
}

//-----------------------------------------------------------------------------
// draw()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::draw()
{
	LLFloater::draw();
	LLRect r = getRect();

	refresh();

	if (mMotionID.notNull() && mAnimPreview)
	{
		gGL.color3f(1.f, 1.f, 1.f);

		gGL.getTexUnit(0)->bind(mAnimPreview);

		gGL.begin( LLRender::QUADS );
		{
			gGL.texCoord2f(0.f, 1.f);
			gGL.vertex2i(PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
			gGL.texCoord2f(0.f, 0.f);
			gGL.vertex2i(PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			gGL.texCoord2f(1.f, 0.f);
			gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_HPAD + PREF_BUTTON_HEIGHT + PREVIEW_HPAD);
			gGL.texCoord2f(1.f, 1.f);
			gGL.vertex2i(r.getWidth() - PREVIEW_HPAD, PREVIEW_TEXTURE_HEIGHT);
		}
		gGL.end();

		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);

		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		if (!avatarp->areAnimationsPaused())
		{
			mAnimPreview->requestUpdate();
		}
	}
}

//-----------------------------------------------------------------------------
// resetMotion()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::resetMotion()
{
	if (!mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	BOOL paused = avatarp->areAnimationsPaused();

	LLKeyframeMotion* motionp = dynamic_cast<LLKeyframeMotion*>(avatarp->findMotion(mMotionID));
	if( motionp )
	{
		// Set emotion
		std::string emote = getChild<LLUICtrl>("emote_combo")->getValue().asString();
		motionp->setEmote(mIDList[emote]);
	}

	LLUUID base_id = mIDList[getChild<LLUICtrl>("preview_base_anim")->getValue().asString()];
	avatarp->deactivateAllMotions();
	avatarp->startMotion(mMotionID, 0.0f);
	avatarp->startMotion(base_id, BASE_ANIM_TIME_OFFSET);
	getChild<LLUICtrl>("playback_slider")->setValue(0.0f);

	// Set pose
	std::string handpose = getChild<LLUICtrl>("hand_pose_combo")->getValue().asString();
	avatarp->startMotion( ANIM_AGENT_HAND_MOTION, 0.0f );

	if( motionp )
	{
		motionp->setHandPose(LLHandMotion::getHandPose(handpose));
	}
	
	if (paused)
	{
		mPauseRequest = avatarp->requestPause();
	}
	else
	{
		mPauseRequest = NULL;	
	}
}

//-----------------------------------------------------------------------------
// handleMouseDown()
//-----------------------------------------------------------------------------
BOOL LLFloaterBvhPreview::handleMouseDown(S32 x, S32 y, MASK mask)
{
	if (mPreviewRect.pointInRect(x, y))
	{
		bringToFront( x, y );
		gFocusMgr.setMouseCapture(this);
		gViewerWindow->hideCursor();
		mLastMouseX = x;
		mLastMouseY = y;
		return TRUE;
	}

	return LLFloater::handleMouseDown(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleMouseUp()
//-----------------------------------------------------------------------------
BOOL LLFloaterBvhPreview::handleMouseUp(S32 x, S32 y, MASK mask)
{
	gFocusMgr.setMouseCapture(FALSE);
	gViewerWindow->showCursor();
	return LLFloater::handleMouseUp(x, y, mask);
}

//-----------------------------------------------------------------------------
// handleHover()
//-----------------------------------------------------------------------------
BOOL LLFloaterBvhPreview::handleHover(S32 x, S32 y, MASK mask)
{
	MASK local_mask = mask & ~MASK_ALT;

	if (mAnimPreview && hasMouseCapture())
	{
		if (local_mask == MASK_PAN)
		{
			// pan here
			mAnimPreview->pan((F32)(x - mLastMouseX) * -0.005f, (F32)(y - mLastMouseY) * -0.005f);
		}
		else if (local_mask == MASK_ORBIT)
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 pitch_radians = (F32)(y - mLastMouseY) * 0.02f;
			
			mAnimPreview->rotate(yaw_radians, pitch_radians);
		}
		else 
		{
			F32 yaw_radians = (F32)(x - mLastMouseX) * -0.01f;
			F32 zoom_amt = (F32)(y - mLastMouseY) * 0.02f;
			
			mAnimPreview->rotate(yaw_radians, 0.f);
			mAnimPreview->zoom(zoom_amt);
		}

		mAnimPreview->requestUpdate();

		LLUI::setMousePositionLocal(this, mLastMouseX, mLastMouseY);
	}

	if (!mPreviewRect.pointInRect(x, y) || !mAnimPreview)
	{
		return LLFloater::handleHover(x, y, mask);
	}
	else if (local_mask == MASK_ORBIT)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLCAMERA);
	}
	else if (local_mask == MASK_PAN)
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLPAN);
	}
	else
	{
		gViewerWindow->setCursor(UI_CURSOR_TOOLZOOMIN);
	}

	return TRUE;
}

//-----------------------------------------------------------------------------
// handleScrollWheel()
//-----------------------------------------------------------------------------
BOOL LLFloaterBvhPreview::handleScrollWheel(S32 x, S32 y, S32 clicks)
{
	if (!mAnimPreview)
		return false;

	mAnimPreview->zoom((F32)clicks * -0.2f);
	mAnimPreview->requestUpdate();

	return TRUE;
}

//-----------------------------------------------------------------------------
// onMouseCaptureLost()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onMouseCaptureLost()
{
	gViewerWindow->showCursor();
}

//-----------------------------------------------------------------------------
// onBtnPlay()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onBtnPlay()
{
	if (!getEnabled())
		return;

	if (mMotionID.notNull() && mAnimPreview)
	{
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		
		if (!avatarp->isMotionActive(mMotionID))
		{
			resetMotion();
			mPauseRequest = NULL;
		}
		else if (avatarp->areAnimationsPaused())
		{			
			mPauseRequest = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// onBtnPause()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onBtnPause()
{
	if (!getEnabled())
		return;
	
	if (mMotionID.notNull() && mAnimPreview)
	{
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();

		if (avatarp->isMotionActive(mMotionID))
		{
			if (!avatarp->areAnimationsPaused())
			{
				mPauseRequest = avatarp->requestPause();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// onBtnStop()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onBtnStop()
{
	if (!getEnabled())
		return;

	if (mMotionID.notNull() && mAnimPreview)
	{
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		resetMotion();
		mPauseRequest = avatarp->requestPause();
	}
}

//-----------------------------------------------------------------------------
// onSliderMove()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onSliderMove()
{
	if (!getEnabled())
		return;

	if (mAnimPreview)
	{
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		F32 slider_value = (F32)getChild<LLUICtrl>("playback_slider")->getValue().asReal();
		LLUUID base_id = mIDList[getChild<LLUICtrl>("preview_base_anim")->getValue().asString()];
		LLMotion* motionp = avatarp->findMotion(mMotionID);
		F32 duration = motionp->getDuration();// + motionp->getEaseOutDuration();
		F32 delta_time = duration * slider_value;
		avatarp->deactivateAllMotions();
		avatarp->startMotion(base_id, delta_time + BASE_ANIM_TIME_OFFSET);
		avatarp->startMotion(mMotionID, delta_time);
		mPauseRequest = avatarp->requestPause();
		refresh();
	}

}

//-----------------------------------------------------------------------------
// onCommitBaseAnim()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitBaseAnim()
{
	if (!getEnabled())
		return;

	if (mAnimPreview)
	{
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();

		BOOL paused = avatarp->areAnimationsPaused();

		// stop all other possible base motions
		avatarp->stopMotion(mIDList["Standing"], TRUE);
		avatarp->stopMotion(mIDList["Walking"], TRUE);
		avatarp->stopMotion(mIDList["Sitting"], TRUE);
		avatarp->stopMotion(mIDList["Flying"], TRUE);

		resetMotion();

		if (!paused)
		{
			mPauseRequest = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// onCommitLoop()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitLoop()
{
	if (!getEnabled() || !mAnimPreview)
		return;
	
	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	if (motionp)
	{
		motionp->setLoop(getChild<LLUICtrl>("loop_check")->getValue().asBoolean());
		motionp->setLoopIn((F32)getChild<LLUICtrl>("loop_in_point")->getValue().asReal() * 0.01f * motionp->getDuration());
		motionp->setLoopOut((F32)getChild<LLUICtrl>("loop_out_point")->getValue().asReal() * 0.01f * motionp->getDuration());
	}
}

//-----------------------------------------------------------------------------
// onCommitLoopIn()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitLoopIn()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	if (motionp)
	{
		motionp->setLoopIn((F32)getChild<LLUICtrl>("loop_in_point")->getValue().asReal() / 100.f);
		resetMotion();
		getChild<LLUICtrl>("loop_check")->setValue(LLSD(TRUE));
		onCommitLoop();
	}
}

//-----------------------------------------------------------------------------
// onCommitLoopOut()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitLoopOut()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	if (motionp)
	{
		motionp->setLoopOut((F32)getChild<LLUICtrl>("loop_out_point")->getValue().asReal() * 0.01f * motionp->getDuration());
		resetMotion();
		getChild<LLUICtrl>("loop_check")->setValue(LLSD(TRUE));
		onCommitLoop();
	}
}

//-----------------------------------------------------------------------------
// onCommitName()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitName()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	if (motionp)
	{
		motionp->setName(getChild<LLUICtrl>("name_form")->getValue().asString());
	}

	doCommit();
}

//-----------------------------------------------------------------------------
// onCommitHandPose()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitHandPose()
{
	if (!getEnabled())
		return;

	resetMotion(); // sets hand pose
}

//-----------------------------------------------------------------------------
// onCommitEmote()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitEmote()
{
	if (!getEnabled())
		return;

	resetMotion(); // ssts emote
}

//-----------------------------------------------------------------------------
// onCommitPriority()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitPriority()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	motionp->setPriority(llfloor((F32)getChild<LLUICtrl>("priority")->getValue().asReal()));
}

//-----------------------------------------------------------------------------
// onCommitEaseIn()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitEaseIn()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	motionp->setEaseIn((F32)getChild<LLUICtrl>("ease_in_time")->getValue().asReal());
	resetMotion();
}

//-----------------------------------------------------------------------------
// onCommitEaseOut()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onCommitEaseOut()
{
	if (!getEnabled() || !mAnimPreview)
		return;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);

	motionp->setEaseOut((F32)getChild<LLUICtrl>("ease_out_time")->getValue().asReal());
	resetMotion();
}

//-----------------------------------------------------------------------------
// validateEaseIn()
//-----------------------------------------------------------------------------
bool LLFloaterBvhPreview::validateEaseIn(const LLSD& data)
{
	if (!getEnabled() || !mAnimPreview)
		return false;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_in = llclamp((F32)getChild<LLUICtrl>("ease_in_time")->getValue().asReal(), 0.f, motionp->getDuration() - motionp->getEaseOutDuration());
		getChild<LLUICtrl>("ease_in_time")->setValue(LLSD(new_ease_in));
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// validateEaseOut()
//-----------------------------------------------------------------------------
bool LLFloaterBvhPreview::validateEaseOut(const LLSD& data)
{
	if (!getEnabled() || !mAnimPreview)
		return false;

	LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
	LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
	
	if (!motionp->getLoop())
	{
		F32 new_ease_out = llclamp((F32)getChild<LLUICtrl>("ease_out_time")->getValue().asReal(), 0.f, motionp->getDuration() - motionp->getEaseInDuration());
		getChild<LLUICtrl>("ease_out_time")->setValue(LLSD(new_ease_out));
	}

	return true;
}

//-----------------------------------------------------------------------------
// validateLoopIn()
//-----------------------------------------------------------------------------
bool LLFloaterBvhPreview::validateLoopIn(const LLSD& data)
{
	if (!getEnabled())
		return false;

	F32 loop_in_value = (F32)getChild<LLUICtrl>("loop_in_point")->getValue().asReal();
	F32 loop_out_value = (F32)getChild<LLUICtrl>("loop_out_point")->getValue().asReal();

	if (loop_in_value < 0.f)
	{
		loop_in_value = 0.f;
	}
	else if (loop_in_value > 100.f)
	{
		loop_in_value = 100.f;
	}
	else if (loop_in_value > loop_out_value)
	{
		loop_in_value = loop_out_value;
	}

	getChild<LLUICtrl>("loop_in_point")->setValue(LLSD(loop_in_value));
	return true;
}

//-----------------------------------------------------------------------------
// validateLoopOut()
//-----------------------------------------------------------------------------
bool LLFloaterBvhPreview::validateLoopOut(const LLSD& data)
{
	if (!getEnabled())
		return false;

	F32 loop_out_value = (F32)getChild<LLUICtrl>("loop_out_point")->getValue().asReal();
	F32 loop_in_value = (F32)getChild<LLUICtrl>("loop_in_point")->getValue().asReal();

	if (loop_out_value < 0.f)
	{
		loop_out_value = 0.f;
	}
	else if (loop_out_value > 100.f)
	{
		loop_out_value = 100.f;
	}
	else if (loop_out_value < loop_in_value)
	{
		loop_out_value = loop_in_value;
	}

	getChild<LLUICtrl>("loop_out_point")->setValue(LLSD(loop_out_value));
	return true;
}


//-----------------------------------------------------------------------------
// refresh()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::refresh()
{
	// Are we showing the play button (default) or the pause button?
	bool show_play = true;
	if (!mAnimPreview)
	{
		getChildView("bad_animation_text")->setVisible(TRUE);
		// play button visible but disabled
		mPlayButton->setEnabled(FALSE);
		mStopButton->setEnabled(FALSE);
		getChildView("ok_btn")->setEnabled(FALSE);
	}
	else
	{
		getChildView("bad_animation_text")->setVisible(FALSE);
		// re-enabled in case previous animation was bad
		mPlayButton->setEnabled(TRUE);
		mStopButton->setEnabled(TRUE);
		LLVOAvatar* avatarp = mAnimPreview->getDummyAvatar();
		if (avatarp->isMotionActive(mMotionID))
		{
			mStopButton->setEnabled(TRUE);
			LLKeyframeMotion* motionp = (LLKeyframeMotion*)avatarp->findMotion(mMotionID);
			if (!avatarp->areAnimationsPaused())
			{
				// animation is playing
				if (motionp)
				{
					F32 fraction_complete = motionp->getLastUpdateTime() / motionp->getDuration();
					getChild<LLUICtrl>("playback_slider")->setValue(fraction_complete);
				}
				show_play = false;
			}
		}
		else
		{
			// Motion just finished playing
			mPauseRequest = avatarp->requestPause();
		}
		getChildView("ok_btn")->setEnabled(TRUE);
		mAnimPreview->requestUpdate();
	}
	mPlayButton->setVisible(show_play);
	mPauseButton->setVisible(!show_play);
}

//-----------------------------------------------------------------------------
// onBtnOK()
//-----------------------------------------------------------------------------
void LLFloaterBvhPreview::onBtnOK(void* userdata)
{
	LLFloaterBvhPreview* floaterp = (LLFloaterBvhPreview*)userdata;
	if (!floaterp->getEnabled()) return;

	if (floaterp->mAnimPreview)
	{
		LLKeyframeMotion* motionp = (LLKeyframeMotion*)floaterp->mAnimPreview->getDummyAvatar()->findMotion(floaterp->mMotionID);

		S32 file_size = motionp->getFileSize();
		U8* buffer = new U8[file_size];

		LLDataPackerBinaryBuffer dp(buffer, file_size);
		if (motionp->serialize(dp))
		{
			LLVFile file(gVFS, motionp->getID(), LLAssetType::AT_ANIMATION, LLVFile::APPEND);

			S32 size = dp.getCurrentSize();
			file.setMaxSize(size);
			if (file.write((U8*)buffer, size))
			{
				std::string name = floaterp->getChild<LLUICtrl>("name_form")->getValue().asString();
				std::string desc = floaterp->getChild<LLUICtrl>("description_form")->getValue().asString();
				LLAssetStorage::LLStoreAssetCallback callback = NULL;
				S32 expected_upload_cost = LLGlobalEconomy::Singleton::getInstance()->getPriceUpload();
				void *userdata = NULL;
				upload_new_resource(floaterp->mTransactionID, // tid
						    LLAssetType::AT_ANIMATION,
						    name,
						    desc,
						    0,
						    LLFolderType::FT_NONE,
						    LLInventoryType::IT_ANIMATION,
						    LLFloaterPerms::getNextOwnerPerms(), LLFloaterPerms::getGroupPerms(), LLFloaterPerms::getEveryonePerms(),
						    name,
						    callback, expected_upload_cost, userdata);

			}
			else
			{
				llwarns << "Failure writing animation data." << llendl;
				LLNotificationsUtil::add("WriteAnimationFail");
			}
		}

		delete [] buffer;
		// clear out cache for motion data
		floaterp->mAnimPreview->getDummyAvatar()->removeMotion(floaterp->mMotionID);
		LLKeyframeDataCache::removeKeyframeData(floaterp->mMotionID);
	}

	floaterp->closeFloater(false);
}

//-----------------------------------------------------------------------------
// LLPreviewAnimation
//-----------------------------------------------------------------------------
LLPreviewAnimation::LLPreviewAnimation(S32 width, S32 height) : LLViewerDynamicTexture(width, height, 3, ORDER_MIDDLE, FALSE)
{
	mNeedsUpdate = TRUE;
	mCameraDistance = PREVIEW_CAMERA_DISTANCE;
	mCameraYaw = 0.f;
	mCameraPitch = 0.f;
	mCameraZoom = 1.f;

	mDummyAvatar = (LLVOAvatar*)gObjectList.createObjectViewer(LL_PCODE_LEGACY_AVATAR, gAgent.getRegion());
	mDummyAvatar->createDrawable(&gPipeline);
	mDummyAvatar->mIsDummy = TRUE;
	mDummyAvatar->mSpecialRenderMode = 1;
	mDummyAvatar->setPositionAgent(LLVector3::zero);
	mDummyAvatar->slamPosition();
	mDummyAvatar->updateJointLODs();
	mDummyAvatar->updateGeometry(mDummyAvatar->mDrawable);
	mDummyAvatar->startMotion(ANIM_AGENT_STAND, BASE_ANIM_TIME_OFFSET);
	mDummyAvatar->hideSkirt();

	// stop extraneous animations
	mDummyAvatar->stopMotion( ANIM_AGENT_HEAD_ROT, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_EYE, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_BODY_NOISE, TRUE );
	mDummyAvatar->stopMotion( ANIM_AGENT_BREATHE_ROT, TRUE );
}

//-----------------------------------------------------------------------------
// LLPreviewAnimation()
//-----------------------------------------------------------------------------
LLPreviewAnimation::~LLPreviewAnimation()
{
	mDummyAvatar->markDead();
}

//virtual
S8 LLPreviewAnimation::getType() const
{
	return LLViewerDynamicTexture::LL_PREVIEW_ANIMATION ;
}

//-----------------------------------------------------------------------------
// update()
//-----------------------------------------------------------------------------
BOOL	LLPreviewAnimation::render()
{
	mNeedsUpdate = FALSE;
	LLVOAvatar* avatarp = mDummyAvatar;
	
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadIdentity();
	gGL.ortho(0.0f, mFullWidth, 0.0f, mFullHeight, -1.0f, 1.0f);

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadIdentity();

	if (LLGLSLShader::sNoFixedFunction)
	{
		gUIProgram.bind();
	}

	LLGLSUIDefault def;
	gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	gGL.color4f(0.15f, 0.2f, 0.3f, 1.f);

	gl_rect_2d_simple( mFullWidth, mFullHeight );

	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();

	gGL.flush();

	LLVector3 target_pos = avatarp->mRoot->getWorldPosition();

	LLQuaternion camera_rot = LLQuaternion(mCameraPitch, LLVector3::y_axis) * 
		LLQuaternion(mCameraYaw, LLVector3::z_axis);

	LLQuaternion av_rot = avatarp->mRoot->getWorldRotation() * camera_rot;
	LLViewerCamera::getInstance()->setOriginAndLookAt(
		target_pos + ((LLVector3(mCameraDistance, 0.f, 0.f) + mCameraOffset) * av_rot),		// camera
		LLVector3::z_axis,																	// up
		target_pos + (mCameraOffset  * av_rot) );											// point of interest

	LLViewerCamera::getInstance()->setView(LLViewerCamera::getInstance()->getDefaultFOV() / mCameraZoom);
	LLViewerCamera::getInstance()->setPerspective(FALSE, mOrigin.mX, mOrigin.mY, mFullWidth, mFullHeight, FALSE);

	mCameraRelPos = LLViewerCamera::getInstance()->getOrigin() - avatarp->mHeadp->getWorldPosition();

	//avatarp->setAnimationData("LookAtPoint", (void *)&mCameraRelPos);

	//SJB: Animation is updated in LLVOAvatar::updateCharacter
	
	if (avatarp->mDrawable.notNull())
	{
		avatarp->updateLOD();
		
		LLVertexBuffer::unbind();
		LLGLDepthTest gls_depth(GL_TRUE);

		LLFace* face = avatarp->mDrawable->getFace(0);
		if (face)
		{
			LLDrawPoolAvatar *avatarPoolp = (LLDrawPoolAvatar *)face->getPool();
			avatarp->dirtyMesh();
			avatarPoolp->renderAvatars(avatarp);  // renders only one avatar
		}
	}

	gGL.color4f(1,1,1,1);
	return TRUE;
}

//-----------------------------------------------------------------------------
// requestUpdate()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::requestUpdate()
{ 
	mNeedsUpdate = TRUE; 
}

//-----------------------------------------------------------------------------
// rotate()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::rotate(F32 yaw_radians, F32 pitch_radians)
{
	mCameraYaw = mCameraYaw + yaw_radians;

	mCameraPitch = llclamp(mCameraPitch + pitch_radians, F_PI_BY_TWO * -0.8f, F_PI_BY_TWO * 0.8f);
}

//-----------------------------------------------------------------------------
// zoom()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::zoom(F32 zoom_delta)
{
	setZoom(mCameraZoom + zoom_delta);
}

//-----------------------------------------------------------------------------
// setZoom()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::setZoom(F32 zoom_amt)
{
	mCameraZoom	= llclamp(zoom_amt, MIN_CAMERA_ZOOM, MAX_CAMERA_ZOOM);
}

//-----------------------------------------------------------------------------
// pan()
//-----------------------------------------------------------------------------
void LLPreviewAnimation::pan(F32 right, F32 up)
{
	mCameraOffset.mV[VY] = llclamp(mCameraOffset.mV[VY] + right * mCameraDistance / mCameraZoom, -1.f, 1.f);
	mCameraOffset.mV[VZ] = llclamp(mCameraOffset.mV[VZ] + up * mCameraDistance / mCameraZoom, -1.f, 1.f);
}



