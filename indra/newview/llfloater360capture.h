/**
 * @file llfloater360capture.h
 * @author Callum Prentice (callum@lindenlab.com)
 * @brief Floater for the 360 capture feature
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

#ifndef LL_FLOATER_360CAPTURE_H
#define LL_FLOATER_360CAPTURE_H

#include "llfloater.h"
#include "llmediactrl.h"
#include "llcharacter.h"

class LLImageRaw;
class LLTextBox;
class LLRadioGroup;

class LLFloater360Capture:
    public LLFloater,
    public LLViewerMediaObserver
{
        friend class LLFloaterReg;

    private:
        LLFloater360Capture(const LLSD& key);

        ~LLFloater360Capture();
        bool postBuild() override;
        void onOpen(const LLSD& key) override;
        void handleMediaEvent(LLPluginClassMedia* self, EMediaEvent event) override;

        const std::string getHTMLBaseFolder();
        void capture360Images();

        const std::string makeFullPathToJS(const std::string filename);
        void writeDataURLHeader(const std::string filename);
        void writeDataURLFooter(const std::string filename);
        bool writeDataURL(const std::string filename, const std::string prefix, U8* data, unsigned int data_len);
        void encodeAndSave(LLPointer<LLImageRaw> raw_image, const std::string filename, const std::string prefix);

        std::vector<LLAnimPauseRequest> mAvatarPauseHandles;
        void freezeWorld(bool enable);

        void mockSnapShot(LLImageRaw* raw);

        void suspendForAFrame();

        const std::string generate_proposed_filename();

        void setSourceImageSize();

        LLMediaCtrl* mWebBrowser;
        const std::string mDefaultHTML = "default.html";
        const std::string mEqrGenHTML = "eqr_gen.html";

        LLUICtrl* mCaptureBtn;
        void onCapture360ImagesBtn();

        void onSaveLocalBtn();
        LLUICtrl* mSaveLocalBtn;

        LLRadioGroup* mQualityRadioGroup;
        void onChooseQualityRadioGroup();
        const std::string getSelectedQualityTooltip();

        int mSourceImageSize;
        float mInitialHeadingDeg;
        int mOutputImageWidth;
        int mOutputImageHeight;
        std::string mImageSaveDir;

        LLPointer<LLImageRaw> mRawImages[6];

        std::string mStartILMode;
};

#endif  // LL_FLOATER_360CAPTURE_H
