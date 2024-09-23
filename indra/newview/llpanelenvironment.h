/**
 * @file llpanelenvironment.h
 * @brief LLPanelExperiences class definition
 *
 * $LicenseInfo:firstyear=2013&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2013, Linden Research, Inc.
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

#ifndef LL_LLPANELENVIRONMENT_H
#define LL_LLPANELENVIRONMENT_H

#include "llaccordionctrltab.h"
#include "llradiogroup.h"
#include "llcheckboxctrl.h"
#include "llsliderctrl.h"
#include "llsettingsdaycycle.h"
#include "llenvironment.h"
#include "llparcel.h"
#include "llsettingspicker.h"
#include "llfloatereditextdaycycle.h"
#include "llestateinfomodel.h"

class LLViewerRegion;
class LLIconCtrl;
class LLSettingsDropTarget;

class LLPanelEnvironmentInfo : public LLPanel
{
    friend class LLSettingsDropTarget;
public:
                                LLPanelEnvironmentInfo();
    virtual                     ~LLPanelEnvironmentInfo();

    virtual bool                postBuild() override;
    virtual void                onOpen(const LLSD& key) override;

    virtual bool                isDirty() const override            { return getIsDirty(); }
    virtual void                onVisibilityChange(bool new_visibility) override;

    virtual void                refresh() override;

    virtual bool                isRegion() const = 0;
    virtual LLParcel *          getParcel() = 0;
    virtual bool                canEdit() = 0;
    virtual S32                 getParcelId() = 0;

protected:
    LOG_CLASS(LLPanelEnvironmentInfo);

    static constexpr U32 ALTITUDE_SLIDER_COUNT = 3;
    static constexpr U32 ALTITUDE_MARKERS_COUNT = 3;
    static constexpr U32 ALTITUDE_PREFIXERS_COUNT = 5;

    static const std::string    BTN_SELECTINV;
    static const std::string    BTN_EDIT;
    static const std::string    BTN_USEDEFAULT;
    static const std::string    BTN_RST_ALTITUDES;
    static const std::string    SLD_DAYLENGTH;
    static const std::string    SLD_DAYOFFSET;
    static const std::string    SLD_ALTITUDES;
    static const std::string    ICN_GROUND;
    static const std::string    ICN_WATER;
    static const std::string    CHK_ALLOWOVERRIDE;
    static const std::string    BTN_APPLY;
    static const std::string    BTN_CANCEL;
    static const std::string    LBL_TIMEOFDAY;
    static const std::string    PNL_SETTINGS;
    static const std::string    PNL_ENVIRONMENT_ALTITUDES;
    static const std::string    PNL_BUTTONS;
    static const std::string    PNL_DISABLED;
    static const std::string    PNL_REGION_MSG;
    static const std::string    TXT_DISABLED;
    static const std::string    SDT_DROP_TARGET;

    static const std::string    STR_LABEL_USEDEFAULT;
    static const std::string    STR_LABEL_USEREGION;
    static const std::string    STR_ALTITUDE_DESCRIPTION;
    static const std::string    STR_NO_PARCEL;
    static const std::string    STR_CROSS_REGION;
    static const std::string    STR_LEGACY;
    static const std::string    STR_DISALLOWED;
    static const std::string    STR_TOO_SMALL;

    static const S32            MINIMUM_PARCEL_SIZE;

    static const U32            DIRTY_FLAG_DAYCYCLE;
    static const U32            DIRTY_FLAG_DAYLENGTH;
    static const U32            DIRTY_FLAG_DAYOFFSET;
    static const U32            DIRTY_FLAG_ALTITUDES;

    static const U32            DIRTY_FLAG_MASK;

    bool                        setControlsEnabled(bool enabled);
    void                        setDirtyFlag(U32 flag);
    void                        clearDirtyFlag(U32 flag);
    bool                        getIsDirty() const                  { return (mDirtyFlag != 0); }
    bool                        getIsDirtyFlag(U32 flag) const      { return ((mDirtyFlag & flag) != 0); }
    U32                         getDirtyFlag() const                { return mDirtyFlag; }
    void                        updateAltLabel(U32 alt_index, U32 sky_index, F32 alt_value);
    void                        readjustAltLabels();

    void                        onSldDayLengthChanged(F32 value);
    void                        onSldDayOffsetChanged(F32 value);
    void                        onAltSliderCallback(LLUICtrl *cntrl, const LLSD &data);
    void                        onAltSliderMouseUp();

    void                        onBtnEdit();
    void                        onBtnSelect();
    void                        onBtnDefault();
    void                        onBtnRstAltitudes();

    void                        udpateApparentTimeOfDay();

    void                        onPickerCommitted(LLUUID item_id, std::string source);
    void                        onPickerCommitted(LLUUID item_id, S32 track_num = LLEnvironment::NO_TRACK);
    void                        onEditCommitted(LLSettingsDay::ptr_t newday);
    void                        onDayLenOffsetMouseUp();
    void                        commitDayLenOffsetChanges(bool need_callback);

    void                        onPickerAssetDownloaded(LLSettingsBase::ptr_t settings);
    void                        onEnvironmentReceived(S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo);
    static void                 onEnvironmentReceived(LLHandle<LLPanel> that_h, S32 parcel_id, LLEnvironment::EnvironmentInfo::ptr_t envifo);

    virtual bool                isLargeEnough() = 0;
    virtual void                refreshFromSource() = 0;

    std::string                 getNameForTrackIndex(U32 index);

    LLFloaterSettingsPicker *   getSettingsPicker(bool create = true);
    LLFloaterEditExtDayCycle *  getEditFloater(bool create = true);
    void                        updateEditFloater(const LLEnvironment::EnvironmentInfo::ptr_t &nextenv, bool enable);

    void                        setCrossRegion(bool val) { mCrossRegion = val; }
    void                        setNoSelection(bool val) { mNoSelection = val; }
    void                        setNoEnvironmentSupport(bool val) { mNoEnvironment = val; }

    LLEnvironment::EnvironmentInfo::ptr_t   mCurrentEnvironment;

    void                        onEnvironmentChanged(LLEnvironment::EnvSelection_t env, S32 version);

    class AltitudeData
    {
    public:
        AltitudeData() :
            mTrackIndex(0), mLabelIndex(0), mAltitude(0)
        {}
        AltitudeData(U32 track_index, U32 label_index, F32 altitude) :
            mTrackIndex(track_index), mLabelIndex(label_index), mAltitude(altitude)
        {}

        U32 mTrackIndex;
        U32 mLabelIndex;
        F32 mAltitude;
    };
    typedef std::map<std::string, AltitudeData>      altitudes_data_t;
    altitudes_data_t                mAltitudes;
    S32                             mCurEnvVersion; // used to filter duplicate callbacks/refreshes

    LLUICtrl* mPanelEnvAltitudes = nullptr;
    LLUICtrl* mPanelEnvConfig = nullptr;
    LLUICtrl* mPanelEnvButtons = nullptr;
    LLUICtrl* mPanelEnvDisabled = nullptr;
    LLUICtrl* mPanelEnvRegionMsg = nullptr;

    LLButton* mBtnSelectInv = nullptr;
    LLButton* mBtnEdit = nullptr;
    LLButton* mBtnUseDefault = nullptr;
    LLButton* mBtnResetAltitudes = nullptr;

    LLMultiSliderCtrl* mMultiSliderAltitudes = nullptr;

    LLSliderCtrl* mSliderDayLength = nullptr;
    LLSliderCtrl* mSliderDayOffset = nullptr;

    LLTextBox* mEnvironmentDisabledText = nullptr;
    LLTextBox* mLabelApparentTime = nullptr;

    LLCheckBoxCtrl* mCheckAllowOverride = nullptr;

    LLIconCtrl* mIconGround = nullptr;
    LLIconCtrl* mIconWater = nullptr;

    std::array<LLUICtrl*, ALTITUDE_MARKERS_COUNT> mAltitudeMarkers;
    std::array<LLSettingsDropTarget*, ALTITUDE_PREFIXERS_COUNT> mAltitudeDropTarget;

    std::array<LLTextBox*, ALTITUDE_PREFIXERS_COUNT> mAltitudeLabels;
    std::array<LLLineEditor*, ALTITUDE_PREFIXERS_COUNT> mAltitudeEditor;
    std::array<LLView*, ALTITUDE_PREFIXERS_COUNT> mAltitudePanels;

protected:
    typedef boost::signals2::connection connection_t;

    void                            refreshFromEstate();
    bool                            mAllowOverride;

private:
    static void                     onIdlePlay(void *);

    connection_t                    mCommitConnection;
    connection_t                    mChangeMonitor;
    connection_t                    mUpdateConnection;

    LLHandle<LLFloater>             mSettingsFloater;
    LLHandle<LLFloater>             mEditFloater;
    S32                             mDirtyFlag;
    S32                             mEditorLastParcelId;
    LLUUID                          mEditorLastRegionId;
    bool                            mCrossRegion;
    bool                            mNoSelection;
    bool                            mNoEnvironment;

};

class LLSettingsDropTarget : public LLView
{
public:
    struct Params : public LLInitParam::Block<Params, LLView::Params>
    {
        Params()
        {
            changeDefault(mouse_opaque, false);
            changeDefault(follows.flags, FOLLOWS_ALL);
        }
    };
    LLSettingsDropTarget(const Params&);
    ~LLSettingsDropTarget() {};

    virtual bool handleDragAndDrop(S32 x, S32 y, MASK mask, bool drop,
        EDragAndDropType cargo_type,
        void* cargo_data,
        EAcceptance* accept,
        std::string& tooltip_msg);
    void setPanel(LLPanelEnvironmentInfo* panel, std::string track) { mEnvironmentInfoPanel = panel;  mTrack = track; };
    void setDndEnabled(bool dnd_enabled) { mDndEnabled = dnd_enabled; };

protected:
    LLPanelEnvironmentInfo* mEnvironmentInfoPanel;
    std::string mTrack;
    bool                    mDndEnabled;
};
#endif // LL_LLPANELENVIRONMENT_H
