/**
 * @file llfloater360capture.cpp
 * @author Callum Prentice (callum@lindenlab.com)
 * @brief Floater code for the 360 Capture feature
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2011, Linden Research, Inc.
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

#include "llfloater360capture.h"

#include "llagent.h"
#include "llagentui.h"
#include "llbase64.h"
#include "llcallbacklist.h"
#include "llenvironment.h"
#include "llimagejpeg.h"
#include "llmediactrl.h"
#include "llradiogroup.h"
#include "llslurl.h"
#include "lltextbox.h"
#include "lltrans.h"
#include "lluictrlfactory.h"
#include "llversioninfo.h"
#include "llviewercamera.h"
#include "llviewercontrol.h"
#include "llviewerpartsim.h"
#include "llviewerregion.h"
#include "llviewerwindow.h"
#include "pipeline.h"

#include <iterator>

LLFloater360Capture::LLFloater360Capture(const LLSD& key)
    :   LLFloater(key)
{
    // The handle to embedded browser that we use to
    // render the WebGL preview as we as host the
    // Cube Map to Equirectangular image code
    mWebBrowser = nullptr;

    // Ask the simulator to send us everything (and not just
    // what it thinks the connected Viewer can see) until
    // such time as we ask it not to (the dtor). If we crash or
    // otherwise, exit before this is turned off, the Simulator
    // will take care of cleaning up for us.
    mStartILMode = gAgent.getInterestListMode();

    // send everything to us for as long as this floater is open
    gAgent.changeInterestListMode(IL_MODE_360);
}

LLFloater360Capture::~LLFloater360Capture()
{
    if (mWebBrowser)
    {
        mWebBrowser->navigateStop();
        mWebBrowser->clearCache();
        mWebBrowser->unloadMediaSource();
    }

    // Restore interest list mode to the state when started
    // Normally LLFloater360Capture tells the Simulator send everything
    // and now reverts to the regular "keyhole" frustum of interest
    // list updates.
    if (!LLApp::isExiting() &&
        gSavedSettings.getBOOL("360CaptureUseInterestListCap") &&
        mStartILMode != gAgent.getInterestListMode())
    {
        gAgent.changeInterestListMode(mStartILMode);
    }
}

bool LLFloater360Capture::postBuild()
{
    mCaptureBtn = getChild<LLUICtrl>("capture_button");
    mCaptureBtn->setCommitCallback(boost::bind(&LLFloater360Capture::onCapture360ImagesBtn, this));

    mSaveLocalBtn = getChild<LLUICtrl>("save_local_button");
    mSaveLocalBtn->setCommitCallback(boost::bind(&LLFloater360Capture::onSaveLocalBtn, this));
    mSaveLocalBtn->setEnabled(false);

    mWebBrowser = getChild<LLMediaCtrl>("360capture_contents");
    mWebBrowser->addObserver(this);
    mWebBrowser->setAllowFileDownload(true);

    // There is a group of radio buttons that define the quality
    // by each having a 'value' that is returns equal to the pixel
    // size (width == height)
    mQualityRadioGroup = getChild<LLRadioGroup>("360_quality_selection");
    mQualityRadioGroup->setCommitCallback(boost::bind(&LLFloater360Capture::onChooseQualityRadioGroup, this));

    // UX/UI called for preview mode (always the first index/option)
    // by default each time vs restoring the last value
    mQualityRadioGroup->setSelectedIndex(0);

    return true;
}

void LLFloater360Capture::onOpen(const LLSD& key)
{
    // Construct a URL pointing to the first page to load. Although
    // we do not use this page for anything (after some significant
    // design changes), we retain the code to load the start page
    // in case that changes again one day. It also makes sure the
    // embedded browser is active and ready to go for when the real
    // page with the 360 preview is navigated to.
    std::string url = STRINGIZE(
                          "file:///" <<
                          getHTMLBaseFolder() <<
                          mDefaultHTML
                      );
    mWebBrowser->navigateTo(url);

    // initial pass at determining what size (width == height since
    // the cube map images are square) we should capture at.
    setSourceImageSize();

    // the size of the output equirectangular image. The height of an EQR image
    // is always 1/2 of the width so we should not store it but rather,
    // calculate it from the width directly
    mOutputImageWidth = gSavedSettings.getU32("360CaptureOutputImageWidth");
    mOutputImageHeight = mOutputImageWidth / 2;

    // enable resizing and enable for width and for height
    enableResizeCtrls(true, true, true);

    // initial heading that consumers of the equirectangular image
    // (such as Facebook or Flickr) use to position initial view -
    // we set during capture - stored as degrees (0..359)
    mInitialHeadingDeg = 0.0;

    // save directory in which to store the images (must obliviously be
    // writable by the viewer). Also create it for users who haven't
    // used the 360 feature before.
    mImageSaveDir = gDirUtilp->getLindenUserDir() + gDirUtilp->getDirDelimiter() + "eqrimg";
    LLFile::mkdir(mImageSaveDir);

    // We do an initial capture when the floater is opened, albeit at a 'preview'
    // quality level (really low resolution, but really fast)
    onCapture360ImagesBtn();
}

// called when the user choose a quality level using
// the buttons in the radio group
void LLFloater360Capture::onChooseQualityRadioGroup()
{
    // set the size of the captured cube map images based
    // on the quality level chosen
    setSourceImageSize();
}


// There is is a setting (360CaptureSourceImageSize) that holds the size
// (width == height since it's a square) of each of the 6 source snapshots.
// However there are some known (and I dare say, some more unknown conditions
// where the specified size is not possible and this function tries to figure it
// out and change that setting to the optimal value for the current conditions.
void LLFloater360Capture::setSourceImageSize()
{
    mSourceImageSize = mQualityRadioGroup->getSelectedValue().asInteger();

    // If deferred rendering is off, we need to shrink the window we capture
    // until it's smaller than the Viewer window dimensions.
    if (!LLPipeline::sRenderDeferred)
    {
        LLRect window_rect = gViewerWindow->getWindowRectRaw();
        S32 window_width = window_rect.getWidth();
        S32 window_height = window_rect.getHeight();

        // It's not possible (as I had hoped) to always render to an off screen
        // buffer regardless of deferred rendering status so the next best
        // option is to render to a buffer that is the size of the users app
        // window.  Note, this was changed - before it chose the smallest
        // power of 2 less than the window size - but since that meant a
        // 1023 height app window would result in a 512 pixel capture, Maxim
        // tried this and it does indeed appear to work.  Mayb need to revisit
        // after the project viewer pass if people on low end graphics systems
        // after having issues.
        if (mSourceImageSize > window_width || mSourceImageSize > window_height)
        {
            mSourceImageSize = llmin(window_width, window_height, mSourceImageSize);
            LL_INFOS("360Capture") << "Deferred rendering is forcing a smaller capture size: " << mSourceImageSize << LL_ENDL;
        }

        // there has to be an easier way than this to get the value
        // from the radio group item at index 0. Why doesn't
        // LLRadioGroup::getSelectedValue(int index) exist?
        int index = mQualityRadioGroup->getSelectedIndex();
        mQualityRadioGroup->setSelectedIndex(0);
        int min_size = mQualityRadioGroup->getSelectedValue().asInteger();
        mQualityRadioGroup->setSelectedIndex(index);

        // If the maximum size we can support falls below a threshold then
        // we should display a message in the log so we can try to debug
        // why this is happening
        if (mSourceImageSize < min_size)
        {
            LL_INFOS("360Capture") << "Small snapshot size due to deferred rendering and small app window" << LL_ENDL;
        }
    }
}

// This function shouldn't exist! We use the tooltip text from
// the corresponding XUI file (floater_360capture.xml) as the
// overlay text for the final web page to inform the user
// about the quality level in play.  There ought to be a
// UI function like LLView* getSelectedItemView() or similar
// but as far as I can tell, there isn't so we have to resort
// to finding it ourselves with this icky code..
const std::string LLFloater360Capture::getSelectedQualityTooltip()
{
    // safey (or bravery?)
    if (mQualityRadioGroup != nullptr)
    {
        // for all the child widgets for the radio group
        // (just the radio buttons themselves I think)
        for (child_list_const_reverse_iter_t iter = mQualityRadioGroup->getChildList()->rbegin();
                iter != mQualityRadioGroup->getChildList()->rend();
                ++iter)
        {
            // if we match the selected index (which we can get easily)
            // with our position in the list of children
            if (mQualityRadioGroup->getSelectedIndex() ==
                    std::distance(mQualityRadioGroup->getChildList()->rend(), iter) - 1)
            {
                // return the plain old tooltip text
                return (*iter)->getToolTip();
            }
        }
    }

    // if it's not found or not available, return an empty string
    return std::string();
}

// Some of the 'magic' happens via a web page in an HTML directory
// and this code provides a single point of reference for its' location
const std::string LLFloater360Capture::getHTMLBaseFolder()
{
    std::string folder_name = gDirUtilp->getSkinDir();
    folder_name += gDirUtilp->getDirDelimiter();
    folder_name += "html";
    folder_name += gDirUtilp->getDirDelimiter();
    folder_name += "common";
    folder_name += gDirUtilp->getDirDelimiter();
    folder_name += "equirectangular";
    folder_name += gDirUtilp->getDirDelimiter();

    return folder_name;
}

// triggered when the 'capture' button in the UI is pressed
void LLFloater360Capture::onCapture360ImagesBtn()
{
    capture360Images();
}

// Gets the full path name for a given JavaScript file in the HTML folder. We
// use this ultimately as a parameter to the main web app so it knows where to find
// the JavaScript array containing the 6 cube map images, stored as data URLs
const std::string LLFloater360Capture::makeFullPathToJS(const std::string filename)
{
    std::string full_js_path = mImageSaveDir;
    full_js_path += gDirUtilp->getDirDelimiter();
    full_js_path += filename;

    return full_js_path;
}

// Write the header/prequel portion of the JavaScript array of data urls
// that we use to store the cube map images in (so the web code can load
// them without tweaking browser security - we'd have to do this if they
// we stored as plain old images) This deliberately overwrites the old
// one, if it exists
void LLFloater360Capture::writeDataURLHeader(const std::string filename)
{
    llofstream file_handle(filename.c_str());
    if (file_handle.is_open())
    {
        file_handle << "// cube map images for Second Life Viewer panorama 360 images" << std::endl;
        file_handle.close();
    }
}

// Write the footer/sequel portion of the JavaScript image code. When this is
// called, the current file on disk will contain the header and the 6 data
// URLs, each with a well specified name.  This final piece of JavaScript code
// creates an array from those data URLs that the main application can
// reference and read.
void LLFloater360Capture::writeDataURLFooter(const std::string filename)
{
    llofstream file_handle(filename.c_str(), std::ios_base::app);
    if (file_handle.is_open())
    {
        file_handle << "var cubemap_img_js = [" << std::endl;
        file_handle << "    img_posx, img_negx," << std::endl;
        file_handle << "    img_posy, img_negy," << std::endl;
        file_handle << "    img_posz, img_negz," << std::endl;
        file_handle << "];" << std::endl;

        file_handle.close();
    }
}

// Given a filename, a chunk of data (representing an image file) and the size
// of the buffer, we create a BASE64 encoded string and use it to build a JavaScript
// data URL that represents the image in a web browser environment
bool LLFloater360Capture::writeDataURL(const std::string filename, const std::string prefix, U8* data, unsigned int data_len)
{
    LL_INFOS("360Capture") << "Writing data URL for " << prefix << " to " << filename << LL_ENDL;

    const std::string data_url = LLBase64::encode(data, data_len);

    llofstream file_handle(filename.c_str(), std::ios_base::app);
    if (file_handle.is_open())
    {
        file_handle << "var img_";
        file_handle << prefix;
        file_handle << " = '";
        file_handle << "data:image/jpeg; base64,";
        file_handle << data_url;
        file_handle << "'";
        file_handle << std::endl;
        file_handle.close();

        return true;
    }

    return false;
}

// Encode the image from each of the 6 snapshots and save it out to
// the JavaScript array of data URLs
void LLFloater360Capture::encodeAndSave(LLPointer<LLImageRaw> raw_image, const std::string filename, const std::string prefix)
{
    // the default quality for the JPEG encoding is set quite high
    // but this still seems to be a reasonable compromise for
    // quality/size and is still much smaller than equivalent PNGs
    int jpeg_encode_quality = gSavedSettings.getU32("360CaptureJPEGEncodeQuality");
    LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG(jpeg_encode_quality);

    LLImageDataSharedLock lock(raw_image);

    // Actually encode the JPEG image. This is where a lot of time
    // is spent now that the snapshot capture process has been
    // optimized.  The encode_time parameter doesn't appear to be
    // used anymore.
    const int encode_time = 0;
    bool resultjpeg = jpeg_image->encode(raw_image, encode_time);

    if (resultjpeg)
    {
        // save individual cube map images as real JPEG files
        // for debugging or curiosity) based on debug settings
        if (gSavedSettings.getBOOL("360CaptureDebugSaveImage"))
        {
            const std::string jpeg_filename = STRINGIZE(
                                                  gDirUtilp->getLindenUserDir() <<
                                                  gDirUtilp->getDirDelimiter() <<
                                                  "eqrimg" <<
                                                  gDirUtilp->getDirDelimiter() <<
                                                  prefix <<
                                                  "." <<
                                                  jpeg_image->getExtension()
                                              );

            LL_INFOS("360Capture") << "Saving debug JPEG image as " << jpeg_filename << LL_ENDL;
            jpeg_image->save(jpeg_filename);
        }

        // actually write the JPEG image to disk as a data URL
        writeDataURL(filename, prefix, jpeg_image->getData(), jpeg_image->getDataSize());
    }
}

// Defer back to the main loop for a single rendered frame to give
// the renderer a chance to update the UI if it is needed
void LLFloater360Capture::suspendForAFrame()
{
    const U32 frame_count_delta = 1;
    U32 curr_frame_count = LLFrameTimer::getFrameCount();
    while (LLFrameTimer::getFrameCount() <= curr_frame_count + frame_count_delta)
    {
        llcoro::suspend();
    }
}

// A debug version of the snapshot code that simply fills the
// buffer with a pattern that can be used to investigate
// issues with encoding and saving off each RAW image.
// Probably not needed anymore but saving here just in case.
void LLFloater360Capture::mockSnapShot(LLImageRaw* raw)
{
    LLImageDataLock lock(raw);

    unsigned int width = raw->getWidth();
    unsigned int height = raw->getHeight();
    unsigned int depth = raw->getComponents();
    unsigned char* pixels = raw->getData();

    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++)
        {
            unsigned long offset = y * width * depth + x * depth;
            unsigned char red = x * 256 / width;
            unsigned char green = y * 256 / height;
            unsigned char blue = ((x + y) / 2) * 256 / (width + height) / 2;
            pixels[offset + 0] = red;
            pixels[offset + 1] = green;
            pixels[offset + 2] = blue;
        }
    }
}

// The main code that actually captures all 6 images and then saves them out to
// disk before navigating the embedded web browser to the page with the WebGL
// application that consumes them and creates an EQR image. This code runs as a
// coroutine so it can be suspended at certain points.
void LLFloater360Capture::capture360Images()
{
    // recheck the size of the cube map source images in case it changed
    // since it was set when we opened the floater
    setSourceImageSize();

    // disable buttons while we are capturing
    mCaptureBtn->setEnabled(false);
    mSaveLocalBtn->setEnabled(false);

    bool render_attached_lights = LLPipeline::sRenderAttachedLights;
    // determine whether or not to include avatar in the scene as we capture the 360 panorama
    if (gSavedSettings.getBOOL("360CaptureHideAvatars"))
    {
        // Turn off the avatar if UI tells us to hide it.
        // Note: the original call to gAvatar.hide(false) did *not* hide
        // attachments and so for most residents, there would be some debris
        // left behind in the snapshot.
        // Note: this toggles so if it set to on, this will turn it off and
        // the subsequent call to the same thing after capture is finished
        // will turn it back on again.  Similarly, for the case where it
        // was set to off - I think this is what we need
        LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
        LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_PARTICLES);
        LLPipeline::sRenderAttachedLights = false;
    }

    // these are the 6 directions we will point the camera - essentially,
    // North, South, East, West, Up, Down
    LLVector3 look_dirs[6] = { LLVector3(1, 0, 0), LLVector3(0, 1, 0), LLVector3(0, 0, 1), LLVector3(-1, 0, 0), LLVector3(0, -1, 0), LLVector3(0, 0, -1) };
    LLVector3 look_upvecs[6] = { LLVector3(0, 0, 1), LLVector3(0, 0, 1), LLVector3(0, -1, 0), LLVector3(0, 0, 1), LLVector3(0, 0, 1), LLVector3(0, 1, 0) };

    // save current view/camera settings so we can restore them afterwards
    S32 old_occlusion = LLPipeline::sUseOcclusion;

    // set new parameters specific to the 360 requirements
    LLPipeline::sUseOcclusion = 0;
    LLViewerCamera* camera = LLViewerCamera::getInstance();
    F32 old_fov = camera->getView();
    F32 old_aspect = camera->getAspect();
    F32 old_yaw = camera->getYaw();

    // stop the motion of as much of the world moving as much as we can
    freezeWorld(true);

    // Save the direction (in degrees) the camera is looking when we
    // take the shot since that is what we write to image metadata
    // 'GPano:InitialViewHeadingDegrees' field.
    // We need to convert from the angle getYaw() gives us into something
    // the XMP data field wants (N=0, E=90, S=180, W= 270 etc.)
    mInitialHeadingDeg  = (float)((360 + 90 - (int)(camera->getYaw() * RAD_TO_DEG)) % 360);
    LL_INFOS("360Capture") << "Recording a heading of " << (int)(mInitialHeadingDeg)
        << " Image size: " << (S32)mSourceImageSize << LL_ENDL;

    // camera constants for the square, cube map capture image
    camera->setAspect(1.0); // must set aspect ratio first to avoid undesirable clamping of vertical FoV
    camera->setView(F_PI_BY_TWO);
    camera->yaw(0.0);

    // record how many times we changed camera to try to understand the "all shots are the same issue"
    unsigned int camera_changed_times = 0;

    // the name of the JavaScript file written out that contains the 6 cube map images
    // stored as a JavaScript array of data URLs.  If you change this filename, you must
    // also change the corresponding entry in the HTML file that uses it -
    // (newview/skins/default/html/common/equirectangular/display_eqr.html)
    const std::string cumemap_js_filename("cubemap_img.js");

    // construct the full path to this file - typically stored in the users'
    // Second Life settings / username / eqrimg folder.
    const std::string cubemap_js_full_path = makeFullPathToJS(cumemap_js_filename);

    // Write the JavaScript file header (the top of the file before the
    // declarations of the actual data URLs array).  In practice, all this writes
    // is a comment - it's main purpose is to reset the file from the last time
    // it was written
    writeDataURLHeader(cubemap_js_full_path);

    // the names of the prefixes we assign as the name to each data URL and are then
    // consumed by the WebGL application. Nominally, they stand for positive and
    // negative in the X/Y/Z directions.
    static const std::string prefixes[6] =
    {
        "posx", "posz", "posy",
        "negx", "negz", "negy",
    };

    // number of times to render the scene (display(..) inside
    // the simple snapshot function in llViewerWindow.
    // Note: rendering even just 1 more time (for a total of 2)
    // has a dramatic effect on the scene contents and *much*
    // less of it is missing. More investigation required
    // but for the moment, this helps with missing content
    // because of interest list issues.
    int num_render_passes = gSavedSettings.getU32("360CaptureNumRenderPasses");

    // time the encode process for later optimization
    auto encode_time_total = 0.0;

    // for each of the 6 directions we shoot...
    for (int i = 0; i < 6; i++)
    {
        LLAppViewer::instance()->pauseMainloopTimeout();
        LLViewerStats::instance().getRecording().stop();

        // these buffers are where the raw, captured pixels are stored and
        // the first time we use them, we have to make a new one
        if (mRawImages[i] == nullptr)
        {
            mRawImages[i] = new LLImageRaw(mSourceImageSize, mSourceImageSize, 3);
        }
        else
            // subsequent capture with floater open so we resize the buffer from
            // the previous run
        {
            // LLImageRaw deletes the old one via operator= but just to be
            // sure, we delete its' large data member first...
            mRawImages[i]->deleteData();
            mRawImages[i] = new LLImageRaw(mSourceImageSize, mSourceImageSize, 3);
        }

        // set up camera to look in each direction
        camera->lookDir(look_dirs[i], look_upvecs[i]);

        // record if camera changed to try to understand the "all shots are the same issue"
        if (camera->isChanged())
        {
            ++camera_changed_times;
        }

        // call the (very) simplified snapshot code that simply deals
        // with a single image, no sub-images etc. but is very fast
        gViewerWindow->simpleSnapshot(mRawImages[i],
                                      mSourceImageSize, mSourceImageSize, num_render_passes);

        // encode each image and write to disk while saving how long it took to do so
        auto t_start = std::chrono::high_resolution_clock::now();
        encodeAndSave(mRawImages[i], cubemap_js_full_path, prefixes[i]);
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_start);
        encode_time_total += duration.count();

        LLViewerStats::instance().getRecording().resume();
        LLAppViewer::instance()->resumeMainloopTimeout();

        // update main loop timeout state
        LLAppViewer::instance()->pingMainloopTimeout("LLFloater360Capture::capture360Images");
    }

    // display time to encode all 6 images.  It tends to be a fairly linear
    // time for each so we don't need to worry about displaying the time
    // for each - this gives us plenty to use for optimizing
    LL_INFOS("360Capture") << "Time to encode and save 6 images was " <<
                           encode_time_total << " seconds" << LL_ENDL;

    // Write the JavaScript file footer (the bottom of the file after the
    // declarations of the actual data URLs array). The footer comprises of
    // a JavaScript array declaration that references the 6 data URLs generated
    // previously and is what is referred to in the display HTML file
    // (newview/skins/default/html/common/equirectangular/display_eqr.html)
    writeDataURLFooter(cubemap_js_full_path);

    // unfreeze the world now we have our shots
    freezeWorld(false);

    // restore original view/camera/avatar settings settings
    camera->setAspect(old_aspect);
    camera->setView(old_fov);
    camera->yaw(old_yaw);
    LLPipeline::sUseOcclusion = old_occlusion;

    // if we toggled off the avatar because the Hide check box was ticked,
    // we should toggle it back to where it was before we started the capture
    if (gSavedSettings.getBOOL("360CaptureHideAvatars"))
    {
        LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_AVATAR);
        LLPipeline::toggleRenderTypeControl(LLPipeline::RENDER_TYPE_PARTICLES);
        LLPipeline::sRenderAttachedLights = render_attached_lights;
    }

    // record that we missed some shots in the log for later debugging
    // note: we use 5 and not 6 because the first shot isn't regarded
    // as a change - only the subsequent 5 are
    if (camera_changed_times < 5)
    {
        LL_WARNS("360Capture") << "360 image capture expected 5 or more images, only captured " << camera_changed_times << " images." << LL_ENDL;
    }

    // now we have the 6 shots saved in a well specified location,
    // we can load the web content that uses them
    std::string url = "file:///" + getHTMLBaseFolder() + mEqrGenHTML;
    mWebBrowser->navigateTo(url);

    // page is loaded and ready so we can turn on the buttons again
    mCaptureBtn->setEnabled(true);
    mSaveLocalBtn->setEnabled(true);

}

// once the request is made to navigate to the web page containing the code
// to process the 6 images into an EQR one, we have to wait for it to finish
// loaded - we get a "navigate complete" event when that happens that we can act on
void LLFloater360Capture::handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event)
{
    switch (event)
    {
        // not used right now but retaining because this event might
        // be useful for a feature I am hoping to add
        case MEDIA_EVENT_LOCATION_CHANGED:
            break;

        // navigation in the browser completed
        case MEDIA_EVENT_NAVIGATE_COMPLETE:
        {
            // Confirm that the navigation event does indeed apply to the
            // page we are looking for. At the moment, this is the only
            // one we care about so the test is superfluous but that might change.
            std::string navigate_url = self->getNavigateURI();
            if (navigate_url.find(mEqrGenHTML) != std::string::npos)
            {
                // this string is being passed across to the web so replace all the windows backslash
                // characters with forward slashes or (I think) the backslashes are treated as escapes
                std::replace(mImageSaveDir.begin(), mImageSaveDir.end(), '\\', '/');

                // we store the camera FOV (field of view) in a saved setting since this feels
                // like something it would be interesting to change and experiment with
                int camera_fov = gSavedSettings.getU32("360CaptureCameraFOV");

                // compose the overlay for the final web page that tells the user
                // what level of quality the capture was taken with
                std::string overlay_label = "'" + getSelectedQualityTooltip() + "'";

                // so now our page is loaded and images are in place - call
                // the JavaScript init script with some parameters to initialize
                // the WebGL based preview
                const std::string cmd = STRINGIZE(
                                            "init("
                                            << mOutputImageWidth
                                            << ", "
                                            << mOutputImageHeight
                                            << ", "
                                            << "'"
                                            << mImageSaveDir
                                            << "'"
                                            << ", "
                                            << camera_fov
                                            << ", "
                                            << LLViewerCamera::getInstance()->getYaw()
                                            << ", "
                                            << overlay_label
                                            << ")"
                                        );

                // execute the command on the page
                mWebBrowser->getMediaPlugin()->executeJavaScript(cmd);
            }
        }
        break;

        default:
            break;
    }
}

// called when the user wants to save the cube maps off to the final EQR image
void LLFloater360Capture::onSaveLocalBtn()
{
    // region name and URL
    std::string region_name; // no sensible default
    std::string region_url("http://secondlife.com");
    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        // region names can (and do) contain characters that would make passing
        // them into a JavaScript function problematic - single quotes for example
        // so we must escape/encode both
        region_name = region->getName();

        // escaping/encoding is a minefield - let's just remove any offending characters from the region name
        region_name.erase(std::remove(region_name.begin(), region_name.end(), '\''), region_name.end());
        region_name.erase(std::remove(region_name.begin(), region_name.end(), '\"'), region_name.end());

        // fortunately there is already an escaping function built into the SLURL generation code
        LLSLURL slurl;
        bool is_escaped = true;
        LLAgentUI::buildSLURL(slurl, is_escaped);
        region_url = slurl.getSLURLString();
    }

    // build suggested filename (the one that appears as the default
    // in the Save dialog box)
    const std::string suggested_filename = generate_proposed_filename();

    // This string (the name of the product plus a truncated version number (no build))
    // is used in the XMP block as the name of the generating and stitching software.
    // We save the version number here and not in the more generic 'software' item
    // because that might help us determine something about the image in the future.
    const std::string client_version = STRINGIZE(
                                           LLVersionInfo::instance().getChannel() <<
                                           " " <<
                                           LLVersionInfo::instance().getShortVersion()
                                       );

    // save the time the image was created. I don't know if this should be
    // UTC/ZULU or the users' local time. It probably doesn't matter.
    std::time_t result = std::time(nullptr);
    std::string ctime_str = std::ctime(&result);
    std::string time_str = ctime_str.substr(0, ctime_str.length() - 1);

    // build the JavaScript data structure that is used to pass all the
    // variables into the JavaScript function on the web page loaded into
    // the embedded browser component of the floater.
    const std::string xmp_details = STRINGIZE(
                                        "{ " <<
                                        "pano_version: '" << "2.2.1" << "', " <<
                                        "software: '" << LLVersionInfo::instance().getChannel() << "', " <<
                                        "capture_software: '" << client_version << "', " <<
                                        "stitching_software: '" << client_version << "', " <<
                                        "width: " << mOutputImageWidth << ", " <<
                                        "height: " << mOutputImageHeight << ", " <<
                                        "heading: " << mInitialHeadingDeg << ", " <<
                                        "actual_source_image_size: " << mQualityRadioGroup->getSelectedValue().asInteger() << ", " <<
                                        "scaled_source_image_size: " << mSourceImageSize << ", " <<
                                        "first_photo_date: '" << time_str << "', " <<
                                        "last_photo_date: '" << time_str << "', " <<
                                        "region_name: '" << region_name << "', " <<
                                        "region_url: '" << region_url << "', " <<
                                        " }"
                                    );

    // build the JavaScript command to send to the web browser
    const std::string cmd = "saveAsEqrImage(\"" + suggested_filename + "\", " + xmp_details + ")";

    // send it to the browser instance, triggering the equirectangular capture
    // process and complimentary offer to save the image
    mWebBrowser->getMediaPlugin()->executeJavaScript(cmd);
}

// We capture all 6 images sequentially and if parts of the world are moving
// E.G. clouds, water, objects - then we may get seams or discontinuities
// when the images are combined to form the EQR image.  This code tries to
// stop everything so we can shoot for seamless shots.  There is probably more
// we can do here - e.g. waves in the water probably won't line up.
void LLFloater360Capture::freezeWorld(bool enable)
{
    static bool clouds_scroll_paused = false;
    if (enable)
    {
        // record the cloud scroll current value so we can restore it
        clouds_scroll_paused = LLEnvironment::instance().isCloudScrollPaused();

        // stop the clouds moving
        LLEnvironment::instance().pauseCloudScroll();

        // freeze all avatars
        for (LLCharacter* character : LLCharacter::sInstances)
        {
            mAvatarPauseHandles.push_back(character->requestPause());
        }

        // freeze everything else
        gSavedSettings.setBOOL("FreezeTime", true);

        // disable particle system
        LLViewerPartSim::getInstance()->enable(false);
    }
    else // turning off freeze world mode, either temporarily or not.
    {
        // restart the clouds moving if they were not paused before
        // we starting using the 360 capture floater
        if (!clouds_scroll_paused)
        {
            LLEnvironment::instance().resumeCloudScroll();
        }

        // thaw all avatars
        mAvatarPauseHandles.clear();

        // thaw everything else
        gSavedSettings.setBOOL("FreezeTime", false);

        //enable particle system
        LLViewerPartSim::getInstance()->enable(true);
    }
}

// Build the default filename that appears in the Save dialog box. We try
// to encode some metadata about too (region name, EQR dimensions, capture
// time) but the user if free to replace this with anything else before
// the images is saved.
const std::string LLFloater360Capture::generate_proposed_filename()
{
    std::ostringstream filename("");

    // base name
    filename << "sl360_";

    LLViewerRegion* region = gAgent.getRegion();
    if (region)
    {
        // this looks complex but it's straightforward - removes all non-alpha chars from a string
        // which in this case is the SL region name - we use it as a proposed filename but the user is free to change
        std::string region_name = region->getName();
        std::replace_if(region_name.begin(), region_name.end(),
                        [](char c){ return ! std::isalnum(c); },
                        '_');
        if (! region_name.empty())
        {
            filename << region_name;
            filename << "_";
        }
    }

    // add in resolution to make it easier to tell what you captured later
    filename << mOutputImageWidth;
    filename << "x";
    filename << mOutputImageHeight;
    filename << "_";

    // Add in the size of the source image (width == height since it was square)
    // Might be useful later for quality comparisons
    filename << mSourceImageSize;
    filename << "_";

    // add in the current HH-MM-SS (with leading 0's) so users can easily save many shots in same folder
    std::time_t cur_epoch = std::time(nullptr);
    std::tm* tm_time = std::localtime(&cur_epoch);
    filename << std::setfill('0') << std::setw(4) << (tm_time->tm_year + 1900);
    filename << std::setfill('0') << std::setw(2) << (tm_time->tm_mon + 1);
    filename << std::setfill('0') << std::setw(2) << tm_time->tm_mday;
    filename << "_";
    filename << std::setfill('0') << std::setw(2) << tm_time->tm_hour;
    filename << std::setfill('0') << std::setw(2) << tm_time->tm_min;
    filename << std::setfill('0') << std::setw(2) << tm_time->tm_sec;

    // the unusual way we save the output image (originates in the
    // embedded browser and not the C++ code) means that the system
    // appends ".jpeg" to the file automatically on macOS at least,
    // so we only need to do it ourselves for windows.
#if LL_WINDOWS
    filename << ".jpg";
#endif

    return filename.str();
}
