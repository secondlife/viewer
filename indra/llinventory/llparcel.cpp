/** 
 * @file llparcel.cpp
 * @brief A land parcel.
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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
#include <iostream>

#include "llparcel.h"
#include "llstreamtools.h"

#include "llmath.h"
#include "llsd.h"
#include "llsdutil.h"
#include "lltransactiontypes.h"
#include "lltransactionflags.h"
#include "llsdutil.h"
#include "message.h"
#include "u64.h"

static const F32 SOME_BIG_NUMBER = 1000.0f;
static const F32 SOME_BIG_NEG_NUMBER = -1000.0f;
static const std::string PARCEL_OWNERSHIP_STATUS_STRING[LLParcel::OS_COUNT+1] =
{
    "leased",
    "lease_pending",
    "abandoned",
    "none"
};

// NOTE: Adding parcel categories also requires updating:
// * newview/app_settings/floater_directory.xml category combobox
// * Web site "create event" tools
static const std::string PARCEL_CATEGORY_STRING[LLParcel::C_COUNT] =
{
    "none",
    "linden",
    "adult",
    "arts",
    "store", // "business" legacy name
    "educational",
    "game",	 // "gaming" legacy name
    "gather", // "hangout" legacy name
    "newcomer",
    "park",
    "home",	 // "residential" legacy name
    "shopping",
    "stage",
    "other",
};
static const std::string PARCEL_CATEGORY_UI_STRING[LLParcel::C_COUNT + 1] =
{
    "None",
    "Linden Location",
    "Adult",
    "Arts & Culture",
    "Business",
    "Educational",
    "Gaming",
    "Hangout",
    "Newcomer Friendly",
    "Parks & Nature",
    "Residential",
    "Shopping",
    "Stage",
    "Other",
    "Any",	 // valid string for parcel searches
};

static const std::string PARCEL_ACTION_STRING[LLParcel::A_COUNT + 1] =
{
    "create",
    "release",
    "absorb",
    "absorbed",
    "divide",
    "division",
    "acquire",
    "relinquish",
    "confirm",
    "unknown"
};

// Timeouts for parcels
// default is 21 days * 24h/d * 60m/h * 60s/m *1000000 usec/s = 1814400000000
const U64 DEFAULT_USEC_CONVERSION_TIMEOUT = U64L(1814400000000);
// ***** TESTING is 10 minutes
//const U64 DEFAULT_USEC_CONVERSION_TIMEOUT = U64L(600000000);

// group is 60 days * 24h/d * 60m/h * 60s/m *1000000 usec/s = 5184000000000
const U64 GROUP_USEC_CONVERSION_TIMEOUT = U64L(5184000000000);
// ***** TESTING is 10 minutes
//const U64 GROUP_USEC_CONVERSION_TIMEOUT = U64L(600000000);

// default sale timeout is 2 days -> 172800000000
const U64 DEFAULT_USEC_SALE_TIMEOUT = U64L(172800000000);
// ***** TESTING is 10 minutes
//const U64 DEFAULT_USEC_SALE_TIMEOUT = U64L(600000000);

// more grace period extensions.
const U64 SEVEN_DAYS_IN_USEC = U64L(604800000000);

// if more than 100,000s before sale revert, and no extra extension
// has been given, go ahead and extend it more. That's about 1.2 days.
const S32 EXTEND_GRACE_IF_MORE_THAN_SEC = 100000;


const std::string& ownership_status_to_string(LLParcel::EOwnershipStatus status);
LLParcel::EOwnershipStatus ownership_string_to_status(const std::string& s);
//const char* revert_action_to_string(LLParcel::ESaleTimerExpireAction action);
//LLParcel::ESaleTimerExpireAction revert_string_to_action(const char* s);
const std::string& category_to_string(LLParcel::ECategory category);
const std::string& category_to_ui_string(LLParcel::ECategory category);
LLParcel::ECategory category_string_to_category(const std::string& s);
LLParcel::ECategory category_ui_string_to_category(const std::string& s);

LLParcel::LLParcel()
{
    init(LLUUID::null, TRUE, FALSE, FALSE, 0, 0, 0, 0, 0, 1.f, 0);
}


LLParcel::LLParcel(const LLUUID &owner_id,
                   BOOL modify, BOOL terraform, BOOL damage,
                   time_t claim_date, S32 claim_price_per_meter,
                   S32 rent_price_per_meter, S32 area, S32 sim_object_limit, F32 parcel_object_bonus,
                   BOOL is_group_owned)
{
    init( owner_id, modify, terraform, damage, claim_date,
          claim_price_per_meter, rent_price_per_meter, area, sim_object_limit, parcel_object_bonus,
          is_group_owned);
}


// virtual
LLParcel::~LLParcel()
{
    // user list cleaned up by LLDynamicArray destructor.
}

void LLParcel::init(const LLUUID &owner_id,
                    BOOL modify, BOOL terraform, BOOL damage,
                    time_t claim_date, S32 claim_price_per_meter,
                    S32 rent_price_per_meter, S32 area, S32 sim_object_limit, F32 parcel_object_bonus,
                    BOOL is_group_owned)
{
	mID.setNull();
	mOwnerID			= owner_id;
	mGroupOwned			= is_group_owned;
	mClaimDate			= claim_date;
	mClaimPricePerMeter	= claim_price_per_meter;
	mRentPricePerMeter	= rent_price_per_meter;
	mArea				= area;
	mDiscountRate		= 1.0f;
	mDrawDistance		= 512.f;

	mUserLookAt.setVec(0.0f, 0.f, 0.f);
	// Default to using the parcel's landing point, if any.
	mLandingType = L_LANDING_POINT;

	// *FIX: if owner_id != null, should be owned or sale pending,
	// investigate init callers.
	mStatus = OS_NONE;
	mCategory = C_NONE;
	mAuthBuyerID.setNull();
	//mBuyerID.setNull();
	//mJoinNeighbors = 0x0;
	mSaleTimerExpires.setTimerExpirySec(0);
	mSaleTimerExpires.stop();
	mGraceExtension = 0;
	//mExpireAction = STEA_REVERT;
	mRecordTransaction = FALSE;

	mAuctionID = 0;
	mInEscrow = false;

	mParcelFlags = PF_DEFAULT;
	setParcelFlag(PF_CREATE_OBJECTS,  modify);
	setParcelFlag(PF_ALLOW_TERRAFORM, terraform);
	setParcelFlag(PF_ALLOW_DAMAGE,    damage);

	mSalePrice			= 10000;
	setName(LLStringUtil::null);
	setDesc(LLStringUtil::null);
	setMusicURL(LLStringUtil::null);
	setMediaURL(LLStringUtil::null);
	setMediaDesc(LLStringUtil::null);
	setMediaType(LLStringUtil::null);
	mMediaID.setNull();
	mMediaAutoScale = 0;
	mMediaLoop = TRUE;
	mObscureMedia = 1;
	mObscureMusic = 1;
	mMediaWidth = 0;
	mMediaHeight = 0;

	mGroupID.setNull();

	mPassPrice = PARCEL_PASS_PRICE_DEFAULT;
	mPassHours = PARCEL_PASS_HOURS_DEFAULT;

	mAABBMin.setVec(SOME_BIG_NUMBER, SOME_BIG_NUMBER, SOME_BIG_NUMBER);
	mAABBMax.setVec(SOME_BIG_NEG_NUMBER, SOME_BIG_NEG_NUMBER, SOME_BIG_NEG_NUMBER);

	mLocalID = 0;

	//mSimWidePrimCorrection = 0;
	setMaxPrimCapacity((S32)(sim_object_limit * area / (F32)(REGION_WIDTH_METERS * REGION_WIDTH_METERS)));
	setSimWideMaxPrimCapacity(0);
	setSimWidePrimCount(0);
	setOwnerPrimCount(0);
	setGroupPrimCount(0);
	setOtherPrimCount(0);
	setSelectedPrimCount(0);
	setTempPrimCount(0);
	setCleanOtherTime(0);
    setRegionPushOverride(FALSE);
    setRegionDenyAnonymousOverride(FALSE);
    setRegionDenyAgeUnverifiedOverride(FALSE);
	setParcelPrimBonus(parcel_object_bonus);

	setPreviousOwnerID(LLUUID::null);
	setPreviouslyGroupOwned(FALSE);
}

void LLParcel::overrideOwner(const LLUUID& owner_id, BOOL is_group_owned)
{
    // Override with system permission (LLUUID::null)
    // Overridden parcels have no group
    mOwnerID = owner_id;
    mGroupOwned = is_group_owned;
    if(mGroupOwned)
    {
        mGroupID = mOwnerID;
    }
    else
    {
        mGroupID.setNull();
    }
    mInEscrow = false;
}

void LLParcel::overrideParcelFlags(U32 flags)
{
    mParcelFlags = flags;
}
void LLParcel::setName(const std::string& name)
{
    // The escaping here must match the escaping in the database
    // abstraction layer.
    mName = name;
    LLStringFn::replace_nonprintable(mName, LL_UNKNOWN_CHAR);
}

void LLParcel::setDesc(const std::string& desc)
{
    // The escaping here must match the escaping in the database
    // abstraction layer.
    mDesc = desc;
    mDesc = rawstr_to_utf8(mDesc);
}

void LLParcel::setMusicURL(const std::string& url)
{
    mMusicURL = url;
    // The escaping here must match the escaping in the database
    // abstraction layer.
    // This should really filter the url in some way. Other than
    // simply requiring non-printable.
    LLStringFn::replace_nonprintable(mMusicURL, LL_UNKNOWN_CHAR);
}

void LLParcel::setMediaURL(const std::string& url)
{
    mMediaURL = url;
    // The escaping here must match the escaping in the database
    // abstraction layer if it's ever added.
    // This should really filter the url in some way. Other than
    // simply requiring non-printable.
    LLStringFn::replace_nonprintable(mMediaURL, LL_UNKNOWN_CHAR);
}

void LLParcel::setMediaDesc(const std::string& desc)
{
	// The escaping here must match the escaping in the database
	// abstraction layer.
	mMediaDesc = desc;
	mMediaDesc = rawstr_to_utf8(mMediaDesc);
}
void LLParcel::setMediaType(const std::string& type)
{
	// The escaping here must match the escaping in the database
	// abstraction layer.
	mMediaType = type;
	mMediaType = rawstr_to_utf8(mMediaType);

	// This code attempts to preserve legacy movie functioning
	if(mMediaType.empty() && ! mMediaURL.empty())
	{
		setMediaType(std::string("video/vnd.secondlife.qt.legacy"));
	}
}
void LLParcel::setMediaWidth(S32 width)
{
	mMediaWidth = width;
}
void LLParcel::setMediaHeight(S32 height)
{
	mMediaHeight = height;
}
// virtual
void LLParcel::setLocalID(S32 local_id)
{
    mLocalID = local_id;
}

void LLParcel::setAllParcelFlags(U32 flags)
{
    mParcelFlags = flags;
}

void LLParcel::setParcelFlag(U32 flag, BOOL b)
{
    if (b)
    {
        mParcelFlags |= flag;
    }
    else
    {
        mParcelFlags &= ~flag;
    }
}


BOOL LLParcel::allowModifyBy(const LLUUID &agent_id, const LLUUID &group_id) const
{
    if (agent_id == LLUUID::null)
    {
        // system always can enter
        return TRUE;
    }
    else if (isPublic())
    {
        return TRUE;
    }
    else if (agent_id == mOwnerID)
    {
        // owner can always perform operations
        return TRUE;
    }
    else if (mParcelFlags & PF_CREATE_OBJECTS)
    {
        return TRUE;
    }
    else if ((mParcelFlags & PF_CREATE_GROUP_OBJECTS)
             && group_id.notNull() )
    {
        return (getGroupID() == group_id);
    }
    
    return FALSE;
}

BOOL LLParcel::allowTerraformBy(const LLUUID &agent_id) const
{
    if (agent_id == LLUUID::null)
    {
        // system always can enter
        return TRUE;
    }
    else if(OS_LEASED == mStatus)
    {
        if(agent_id == mOwnerID)
        {
            // owner can modify leased land
            return TRUE;
        }
        else
        {
            // otherwise check other people
            return mParcelFlags & PF_ALLOW_TERRAFORM;
        }
    }
    else
    {
        return FALSE;
    }
}


bool LLParcel::isAgentBlockedFromParcel(LLParcel* parcelp,
                                        const LLUUID& agent_id,
                                        const std::vector<LLUUID>& group_ids,
                                        const BOOL is_agent_identified,
                                        const BOOL is_agent_transacted,
                                        const BOOL is_agent_ageverified)
{
    S32 current_group_access = parcelp->blockAccess(agent_id, LLUUID::null, is_agent_identified, is_agent_transacted, is_agent_ageverified);
    S32 count;
    bool is_allowed = (current_group_access == BA_ALLOWED) ? true: false;
    LLUUID group_id;
    
    count = group_ids.size();
    for (int i = 0; i < count && !is_allowed; i++)
    {
        group_id = group_ids[i];
        current_group_access = parcelp->blockAccess(agent_id, group_id, is_agent_identified, is_agent_transacted, is_agent_ageverified);
        
        if (current_group_access == BA_ALLOWED) is_allowed = true;
    }
    
    return !is_allowed;
}

BOOL LLParcel::isAgentBanned(const LLUUID& agent_id) const
{
	// Test ban list
	if (mBanList.find(agent_id) != mBanList.end())
	{
		return TRUE;
	}
    
    return FALSE;
}

S32 LLParcel::blockAccess(const LLUUID& agent_id, const LLUUID& group_id,
                          const BOOL is_agent_identified,
                          const BOOL is_agent_transacted,
                          const BOOL is_agent_ageverified) const
{
    // Test ban list
    if (isAgentBanned(agent_id))
    {
        return BA_BANNED;
    }
    
    // Always allow owner on (unless he banned himself, useful for
    // testing). We will also allow estate owners/managers in if they 
    // are not explicitly banned.
    if (agent_id == mOwnerID)
    {
        return BA_ALLOWED;
    }
    
    // Special case when using pass list where group access is being restricted but not 
    // using access list.	 In this case group members are allowed only if they buy a pass.
    // We return BA_NOT_IN_LIST if not in list
    BOOL passWithGroup = getParcelFlag(PF_USE_PASS_LIST) && !getParcelFlag(PF_USE_ACCESS_LIST) 
    && getParcelFlag(PF_USE_ACCESS_GROUP) && !mGroupID.isNull() && group_id == mGroupID;
    
    
    // Test group list
    if (getParcelFlag(PF_USE_ACCESS_GROUP)
        && !mGroupID.isNull()
        && group_id == mGroupID
        && !passWithGroup)
    {
        return BA_ALLOWED;
    }
    
    // Test access list
    if (getParcelFlag(PF_USE_ACCESS_LIST) || passWithGroup )
    {
        if (mAccessList.find(agent_id) != mAccessList.end())
        {
            return BA_ALLOWED;
        }
        
        return BA_NOT_ON_LIST; 
    }
    
    // If we're not doing any other limitations, all users
    // can enter, unless
    if (		 !getParcelFlag(PF_USE_ACCESS_GROUP)
                 && !getParcelFlag(PF_USE_ACCESS_LIST))
    { 
        //If the land is group owned, and you are in the group, bypass these checks
        if(getIsGroupOwned() && group_id == mGroupID)
        {
            return BA_ALLOWED;
        }
        
        // Test for "payment" access levels
        // Anonymous - No Payment Info on File
        if(getParcelFlag(PF_DENY_ANONYMOUS) && !is_agent_identified && !is_agent_transacted)
        {
            return BA_NO_ACCESS_LEVEL;
        }
        // AgeUnverified - Not Age Verified
        if(getParcelFlag(PF_DENY_AGEUNVERIFIED) && !is_agent_ageverified)
        {
			return BA_NOT_AGE_VERIFIED;
        }
    
        return BA_ALLOWED;
    }
    
    return BA_NOT_IN_GROUP;
    
}


void LLParcel::setArea(S32 area, S32 sim_object_limit)
{
    mArea = area;
    setMaxPrimCapacity((S32)(sim_object_limit * area / (F32)(REGION_WIDTH_METERS * REGION_WIDTH_METERS)));
}

void LLParcel::setDiscountRate(F32 rate)
{
    // this is to make sure that the rate is at least sane - this is
    // not intended to enforce economy rules. It only enfoces that the
    // rate is a scaler between 0 and 1.
    mDiscountRate = llclampf(rate);
}


//-----------------------------------------------------------
// File input and output
//-----------------------------------------------------------


// WARNING: Area will be wrong until you calculate it.
BOOL LLParcel::importStream(std::istream& input_stream)
{
	U32 setting;
	S32 secs_until_revert = 0;

	skip_to_end_of_next_keyword("{", input_stream);
	if (!input_stream.good()) 
	{
		llwarns << "LLParcel::importStream() - bad input_stream" << llendl;
		return FALSE;
	}

	while (input_stream.good())
	{
		skip_comments_and_emptyspace(input_stream);
		std::string line, keyword, value;
		get_line(line, input_stream, MAX_STRING);
		get_keyword_and_value(keyword, value, line);

		if ("}" == keyword)
		{
			break;
		}
		else if ("parcel_id" == keyword)
		{
			mID.set(value);
		}
		else if ("status" == keyword)
		{
			mStatus = ownership_string_to_status(value);
		}
		else if ("category" == keyword)
		{
			mCategory = category_string_to_category(value);
		}
		else if ("local_id" == keyword)
		{
			LLStringUtil::convertToS32(value, mLocalID);
		}
		else if ("name" == keyword)
		{
			setName( value );
		}
		else if ("desc" == keyword)
		{
			setDesc( value );
		}
		else if ("music_url" == keyword)
		{
			setMusicURL( value );
		}
		else if ("media_url" == keyword)
		{
			setMediaURL( value );
		}
		else if ("media_desc" == keyword)
		{
			setMediaDesc( value );
		}
		else if ("media_type" == keyword)
		{
			setMediaType( value );
		}
		else if ("media_width" == keyword)
		{
			S32 width;
			LLStringUtil::convertToS32(value, width);
			setMediaWidth( width );
		}
		else if ("media_height" == keyword)
		{
			S32 height;
			LLStringUtil::convertToS32(value, height);
			setMediaHeight( height );
		}
		else if ("media_id" == keyword)
		{
			mMediaID.set( value );
		}
		else if ("media_auto_scale" == keyword)
		{
			LLStringUtil::convertToU8(value, mMediaAutoScale);
		}
		else if ("media_loop" == keyword)
		{
			LLStringUtil::convertToU8(value, mMediaLoop);
		}
		else if ("obscure_media" == keyword)
		{
			LLStringUtil::convertToU8(value, mObscureMedia);
		}		
		else if ("obscure_music" == keyword)
		{
			LLStringUtil::convertToU8(value, mObscureMusic);
		}		
		else if ("owner_id" == keyword)
		{
			mOwnerID.set( value );
		}
		else if ("group_owned" == keyword)
		{
			LLStringUtil::convertToBOOL(value, mGroupOwned);
		}
		else if ("clean_other_time" == keyword)
		{
			S32 time;
			LLStringUtil::convertToS32(value, time);
			setCleanOtherTime(time);
		}
		else if ("auth_buyer_id" == keyword)
		{
			mAuthBuyerID.set(value);
		}
		else if ("snapshot_id" == keyword)
		{
			mSnapshotID.set(value);
		}
		else if ("user_location" == keyword)
		{
			sscanf(value.c_str(), "%f %f %f",
				&mUserLocation.mV[VX],
				&mUserLocation.mV[VY],
				&mUserLocation.mV[VZ]);
		}
		else if ("user_look_at" == keyword)
		{
			sscanf(value.c_str(), "%f %f %f",
				&mUserLookAt.mV[VX],
				&mUserLookAt.mV[VY],
				&mUserLookAt.mV[VZ]);
		}
		else if ("landing_type" == keyword)
		{
			S32 landing_type = 0;
			LLStringUtil::convertToS32(value, landing_type);
			mLandingType = (ELandingType) landing_type;
		}
		else if ("join_neighbors" == keyword)
		{
			llinfos << "found deprecated keyword join_neighbors" << llendl;
		}
		else if ("revert_sale" == keyword)
		{
			LLStringUtil::convertToS32(value, secs_until_revert);
			if (secs_until_revert > 0)
			{
				mSaleTimerExpires.start();
				mSaleTimerExpires.setTimerExpirySec((F32)secs_until_revert);
			}
		}
		else if("extended_grace" == keyword)
		{
			LLStringUtil::convertToS32(value, mGraceExtension);
		}
		else if ("user_list_type" == keyword)
		{
			// deprecated
		}
		else if("auction_id" == keyword)
		{
			LLStringUtil::convertToU32(value, mAuctionID);
		}
		else if ("allow_modify" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_CREATE_OBJECTS, setting);
		}
		else if ("allow_group_modify" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_CREATE_GROUP_OBJECTS, setting);
		}
		else if ("allow_all_object_entry" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_ALL_OBJECT_ENTRY, setting);
		}
		else if ("allow_group_object_entry" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_GROUP_OBJECT_ENTRY, setting);
		}
		else if ("allow_deed_to_group" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_DEED_TO_GROUP, setting);
		}
		else if("contribute_with_deed" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_CONTRIBUTE_WITH_DEED, setting);
		}
		else if ("allow_terraform" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_TERRAFORM, setting);
		}
		else if ("allow_damage" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_DAMAGE, setting);
		}
		else if ("allow_fly" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_FLY, setting);
		}
		else if ("allow_landmark" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_LANDMARK, setting);
		}
		else if ("sound_local" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_SOUND_LOCAL, setting);
		}
		else if ("allow_group_scripts" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_GROUP_SCRIPTS, setting);
		}
		else if ("allow_voice_chat" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_VOICE_CHAT, setting);
		}
		else if ("use_estate_voice_chan" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, setting);
		}
		else if ("allow_scripts" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_OTHER_SCRIPTS, setting);
		}
		else if ("for_sale" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_FOR_SALE, setting);
		}
		else if ("sell_w_objects" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_SELL_PARCEL_OBJECTS, setting);
		}
		else if ("use_pass_list" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_USE_PASS_LIST, setting);
		}
		else if ("show_directory" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_SHOW_DIRECTORY, setting);
		}
		else if ("allow_publish" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_ALLOW_PUBLISH, setting);
		}
		else if ("mature_publish" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_MATURE_PUBLISH, setting);
		}
		else if ("claim_date" == keyword)
		{
			// BUG: This will fail when time rolls over in 2038.
			S32 time;
			LLStringUtil::convertToS32(value, time);
			mClaimDate = time;
		}
		else if ("claim_price" == keyword)
		{
			LLStringUtil::convertToS32(value, mClaimPricePerMeter);
		}
		else if ("rent_price" == keyword)
		{
			LLStringUtil::convertToS32(value, mRentPricePerMeter);
		}
		else if ("discount_rate" == keyword)
		{
			LLStringUtil::convertToF32(value, mDiscountRate);
		}
		else if ("draw_distance" == keyword)
		{
			LLStringUtil::convertToF32(value, mDrawDistance);
		}
		else if ("sale_price" == keyword)
		{
			LLStringUtil::convertToS32(value, mSalePrice);
		}
		else if ("pass_price" == keyword)
		{
			LLStringUtil::convertToS32(value, mPassPrice);
		}
		else if ("pass_hours" == keyword)
		{
			LLStringUtil::convertToF32(value, mPassHours);
		}
		else if ("box" == keyword)
		{
			// deprecated
		}
		else if ("aabb_min" == keyword)
		{
			sscanf(value.c_str(), "%f %f %f", 
				&mAABBMin.mV[VX], &mAABBMin.mV[VY], &mAABBMin.mV[VZ]);
		}
		else if ("use_access_group" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_USE_ACCESS_GROUP, setting);
		}
		else if ("use_access_list" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_USE_ACCESS_LIST, setting);
		}
		else if ("use_ban_list" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_USE_BAN_LIST, setting);
		}
		else if ("group_name" == keyword)
		{
			llinfos << "found deprecated keyword group_name" << llendl;
		}
		else if ("group_id" == keyword)
		{
			mGroupID.set( value );
		}
		// TODO: DEPRECATED FLAG
		// Flag removed from simstate files in 1.11.1
		// Remove at some point where we have guarenteed this flag
		// no longer exists anywhere in simstate files.
		else if ("require_identified" == keyword)
		{
			// LLStringUtil::convertToU32(value, setting);
			// setParcelFlag(PF_DENY_ANONYMOUS, setting);
		}
		// TODO: DEPRECATED FLAG
		// Flag removed from simstate files in 1.11.1
		// Remove at some point where we have guarenteed this flag
		// no longer exists anywhere in simstate files.
		else if ("require_transacted" == keyword)
		{
			// LLStringUtil::convertToU32(value, setting);
			// setParcelFlag(PF_DENY_ANONYMOUS, setting);
			// setParcelFlag(PF_DENY_IDENTIFIED, setting);
		}
		else if ("restrict_pushobject" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_RESTRICT_PUSHOBJECT, setting);
		}
		else if ("deny_anonymous" == keyword)
		{
			LLStringUtil::convertToU32(value, setting);
			setParcelFlag(PF_DENY_ANONYMOUS, setting);
		}
		else if ("deny_identified" == keyword)
		{
// 			LLStringUtil::convertToU32(value, setting);
// 			setParcelFlag(PF_DENY_IDENTIFIED, setting);
		}
		else if ("deny_transacted" == keyword)
		{
// 			LLStringUtil::convertToU32(value, setting);
// 			setParcelFlag(PF_DENY_TRANSACTED, setting);
		}
        else if ("deny_age_unverified" == keyword)
        {
            LLStringUtil::convertToU32(value, setting);
            setParcelFlag(PF_DENY_AGEUNVERIFIED, setting);
        }
        else if ("access_list" == keyword)
        {
            S32 entry_count = 0;
            LLStringUtil::convertToS32(value, entry_count);
            for (S32 i = 0; i < entry_count; i++)
            {
                LLAccessEntry entry;
                if (importAccessEntry(input_stream, &entry))
                {
                    mAccessList[entry.mID] = entry;
                }
            }
        }
        else if ("ban_list" == keyword)
        {
            S32 entry_count = 0;
            LLStringUtil::convertToS32(value, entry_count);
            for (S32 i = 0; i < entry_count; i++)
            {
                LLAccessEntry entry;
                if (importAccessEntry(input_stream, &entry))
                {
                    mBanList[entry.mID] = entry;
                }
            }
        }
        else if ("renter_list" == keyword)
        {
            /*
             S32 entry_count = 0;
             LLStringUtil::convertToS32(value, entry_count);
             for (S32 i = 0; i < entry_count; i++)
             {
                 LLAccessEntry entry;
                 if (importAccessEntry(input_stream, &entry))
                 {
                     mRenterList.put(entry);
                 }
             }*/
        }
        else if ("pass_list" == keyword)
        {
            // legacy - put into access list
            S32 entry_count = 0;
            LLStringUtil::convertToS32(value, entry_count);
            for (S32 i = 0; i < entry_count; i++)
            {
                LLAccessEntry entry;
                if (importAccessEntry(input_stream, &entry))
                {
                    mAccessList[entry.mID] = entry;
                }
            }
        }
        
        else
        {
            llwarns << "Unknown keyword in parcel section: <" 
            << keyword << ">" << llendl;
        }
    }
    
    // this code block detects if we have loaded a 1.1 simstate file,
    // and follows the conversion rules specified in
    // design_docs/land/pay_for_parcel.txt.
    F32 time_to_expire = 0.0f;
    if(mID.isNull())
    {
        mID.generate();
        mStatus = OS_LEASE_PENDING;
        //mBuyerID = mOwnerID;
        if(getIsGroupOwned())
        {
            time_to_expire += GROUP_USEC_CONVERSION_TIMEOUT / SEC_TO_MICROSEC;
        }
        else
        {
            time_to_expire += DEFAULT_USEC_CONVERSION_TIMEOUT / SEC_TO_MICROSEC;
        }
        //mExpireAction = STEA_PUBLIC;
        mRecordTransaction = TRUE;
    }
    
    // this code block deals with giving an extension to pending
    // parcels to the midday of 2004-01-19 if they were originally set
    // for some time on 2004-01-12.
    if((0 == mGraceExtension)
       && (EXTEND_GRACE_IF_MORE_THAN_SEC < secs_until_revert))
    {
        const S32 NEW_CONVERSION_DATE = 1074538800; // 2004-01-19T11:00:00
        time_t now = time(NULL); // now in epoch
        secs_until_revert = (S32)(NEW_CONVERSION_DATE - now);
        time_to_expire = (F32)secs_until_revert;
        mGraceExtension = 1;
    }
    
    // This code block adds yet another week to the deadline. :(
    if(1 == mGraceExtension)
    {
        time_to_expire += SEVEN_DAYS_IN_USEC / SEC_TO_MICROSEC;
        mGraceExtension = 2;
    }
    
    if (time_to_expire > 0)
    {
        mSaleTimerExpires.setTimerExpirySec(time_to_expire);
        mSaleTimerExpires.start();
    }
    
    // successful import
    return TRUE;
}


BOOL LLParcel::importAccessEntry(std::istream& input_stream, LLAccessEntry* entry)
{
    skip_to_end_of_next_keyword("{", input_stream);
    while (input_stream.good())
    {
        skip_comments_and_emptyspace(input_stream);
        std::string line, keyword, value;
        get_line(line, input_stream, MAX_STRING);
        get_keyword_and_value(keyword, value, line);
        
        if ("}" == keyword)
        {
            break;
        }
        else if ("id" == keyword)
        {
            entry->mID.set( value );
        }
        else if ("name" == keyword)
        {
            // deprecated
        }
        else if ("time" == keyword)
        {
            S32 when;
            LLStringUtil::convertToS32(value, when);
            entry->mTime = when;
        }
        else if ("flags" == keyword)
        {
            U32 setting;
            LLStringUtil::convertToU32(value, setting);
            entry->mFlags = setting;
        }
        else
        {
            llwarns << "Unknown keyword in parcel access entry section: <" 
            << keyword << ">" << llendl;
        }
    }
    return input_stream.good();
}

BOOL LLParcel::exportStream(std::ostream& output_stream)
{
	S32 setting;
	std::string id_string;

	std::ios::fmtflags old_flags = output_stream.flags();
	output_stream.setf(std::ios::showpoint);
	output_stream << "\t{\n";

	mID.toString(id_string);
	output_stream << "\t\t parcel_id        " << id_string << "\n";
	output_stream << "\t\t status           " << ownership_status_to_string(mStatus) << "\n";
	output_stream << "\t\t category         " << category_to_string(mCategory) << "\n";

	output_stream << "\t\t local_id         " << mLocalID  << "\n";

	const char* name = (mName.empty() ? "" : mName.c_str() );
	output_stream << "\t\t name             " << name << "\n";

	const char* desc = (mDesc.empty() ? "" : mDesc.c_str() );
	output_stream << "\t\t desc             " << desc << "\n";

	const char* music_url = (mMusicURL.empty() ? "" : mMusicURL.c_str() );
	output_stream << "\t\t music_url        " << music_url << "\n";

	const char* media_url = (mMediaURL.empty() ? "" : mMediaURL.c_str() );
	output_stream << "\t\t media_url        " << media_url << "\n";

	const char* media_type = (mMediaType.empty() ? "" : mMediaType.c_str() );
	output_stream << "\t\t media_type             " << media_type << "\n";

	const char* media_desc = (mMediaDesc.empty() ? "" : mMediaDesc.c_str() );
	output_stream << "\t\t media_desc             " << media_desc << "\n";

	output_stream << "\t\t media_auto_scale " << (mMediaAutoScale ? 1 : 0)  << "\n";
	output_stream << "\t\t media_loop " << (mMediaLoop ? 1 : 0)  << "\n";
	output_stream << "\t\t obscure_media " << (mObscureMedia ? 1 : 0)  << "\n";
	output_stream << "\t\t obscure_music " << (mObscureMusic ? 1 : 0)  << "\n";

	mMediaID.toString(id_string);
	output_stream << "\t\t media_id         " << id_string  << "\n";

	output_stream << "\t\t media_width     " << mMediaWidth  << "\n";
	output_stream << "\t\t media_height     " << mMediaHeight  << "\n";

	mOwnerID.toString(id_string);
	output_stream << "\t\t owner_id         " << id_string  << "\n";
	output_stream << "\t\t group_owned	   " << (mGroupOwned ? 1 : 0)  << "\n";
	output_stream << "\t\t clean_other_time " << getCleanOtherTime() << "\n";

	if(!mAuthBuyerID.isNull())
	{
		mAuthBuyerID.toString(id_string);
		output_stream << "\t\t auth_buyer_id    " << id_string << "\n";
	}
	if (!mSnapshotID.isNull())
	{
		mSnapshotID.toString(id_string);
		output_stream << "\t\t snapshot_id      " << id_string << "\n";
	}
	if (!mUserLocation.isExactlyZero())
	{
		output_stream << "\t\t user_location " 
			<< (F64)mUserLocation.mV[VX]
			<< " " << (F64)mUserLocation.mV[VY]
			<< " " << (F64)mUserLocation.mV[VZ] << "\n";
		output_stream << "\t\t user_look_at " 
			<< (F64)mUserLookAt.mV[VX]
			<< " " << (F64)mUserLookAt.mV[VY]
			<< " " << (F64)mUserLookAt.mV[VZ] << "\n";
	}
	output_stream << "\t\t landing_type " << mLandingType << "\n";
	//if(mJoinNeighbors)
	//{
	//	output_stream << "\t\t join_neighbors " << mJoinNeighbors << "\n";
	//}
	if(mSaleTimerExpires.getStarted())
	{
		S32 dt_sec = (S32) mSaleTimerExpires.getRemainingTimeF32()+60; // Add a minute to prevent race conditions
		output_stream << "\t\t revert_sale      " << dt_sec << "\n";
		//output_stream << "\t\t revert_action    " << revert_action_to_string(mExpireAction) << "\n";
		output_stream << "\t\t extended_grace   " << mGraceExtension << "\n";
	}

	if(0 != mAuctionID)
	{
		output_stream << "\t\t auction_id       " << mAuctionID << "\n";
	}

	output_stream << "\t\t allow_modify     " << getAllowModify()  << "\n";
	output_stream << "\t\t allow_group_modify     " << getAllowGroupModify()  << "\n";
	output_stream << "\t\t allow_all_object_entry     " << getAllowAllObjectEntry()  << "\n";
	output_stream << "\t\t allow_group_object_entry     " << getAllowGroupObjectEntry()  << "\n";
	output_stream << "\t\t allow_terraform  " << getAllowTerraform()  << "\n";
	output_stream << "\t\t allow_deed_to_group " << getAllowDeedToGroup()  << "\n";
	output_stream << "\t\t contribute_with_deed " << getContributeWithDeed() << "\n";
	output_stream << "\t\t allow_damage     " << getAllowDamage()  << "\n";
	output_stream << "\t\t claim_date       " << (S32)mClaimDate  << "\n";
	output_stream << "\t\t claim_price      " << mClaimPricePerMeter  << "\n";
	output_stream << "\t\t rent_price       " << mRentPricePerMeter  << "\n";
	output_stream << "\t\t discount_rate    " << mDiscountRate  << "\n";
	output_stream << "\t\t allow_fly        " << (getAllowFly()      ? 1 : 0)  << "\n";
	output_stream << "\t\t allow_landmark   " << (getAllowLandmark() ? 1 : 0)  << "\n";
	output_stream << "\t\t sound_local	   " << (getSoundLocal() ? 1 : 0)  << "\n";
	output_stream << "\t\t allow_scripts    " << (getAllowOtherScripts()  ? 1 : 0)  << "\n";
	output_stream << "\t\t allow_group_scripts    " << (getAllowGroupScripts()  ? 1 : 0)  << "\n";
	output_stream << "\t\t use_estate_voice_chan		 " << (getParcelFlagUseEstateVoiceChannel() ? 1 : 0) << "\n";

	output_stream << "\t\t allow_voice_chat    " << (getParcelFlagAllowVoice() ? 1 : 0) << "\n";
	output_stream << "\t\t use_estate_voice_chan   " << (getParcelFlagUseEstateVoiceChannel() ? 1 : 0) << "\n";
	output_stream << "\t\t for_sale         " << (getForSale()       ? 1 : 0)  << "\n";
	output_stream << "\t\t sell_w_objects   " << (getSellWithObjects()	? 1 : 0)  << "\n";
	output_stream << "\t\t draw_distance    " << mDrawDistance  << "\n";
	output_stream << "\t\t sale_price       " << mSalePrice  << "\n";

	setting = (getParcelFlag(PF_USE_ACCESS_GROUP) ? 1 : 0);
	output_stream << "\t\t use_access_group " << setting  << "\n";

	setting = (getParcelFlag(PF_USE_ACCESS_LIST) ? 1 : 0);
	output_stream << "\t\t use_access_list  " << setting  << "\n";

	setting = (getParcelFlag(PF_USE_BAN_LIST) ? 1 : 0);
	output_stream << "\t\t use_ban_list     " << setting  << "\n";

	mGroupID.toString(id_string);
	output_stream << "\t\t group_id  " << id_string  << "\n";

	//const char* group_name
	//	= (mGroupName.isEmpty() ? "" : mGroupName.c_str() );
	//output_stream << "\t\t group_name " << group_name << "\n";

	setting = (getParcelFlag(PF_USE_PASS_LIST) ? 1 : 0);
	output_stream << "\t\t use_pass_list    " << setting  << "\n";

	output_stream << "\t\t pass_price       " << mPassPrice  << "\n";
	output_stream << "\t\t pass_hours       " << mPassHours  << "\n";

	setting = (getParcelFlag(PF_SHOW_DIRECTORY) ? 1 : 0);
	output_stream << "\t\t show_directory   " << setting  << "\n";

	setting = (getParcelFlag(PF_ALLOW_PUBLISH) ? 1 : 0);
	output_stream << "\t\t allow_publish     " << setting  << "\n";

	setting = (getParcelFlag(PF_MATURE_PUBLISH) ? 1 : 0);
	output_stream << "\t\t mature_publish     " << setting  << "\n";

	setting = (getParcelFlag(PF_DENY_ANONYMOUS) ? 1 : 0);
	output_stream << "\t\t deny_anonymous     " << setting  << "\n";

//	setting = (getParcelFlag(PF_DENY_IDENTIFIED) ? 1 : 0);
//	output_stream << "\t\t deny_identified     " << setting  << "\n";

//	setting = (getParcelFlag(PF_DENY_TRANSACTED) ? 1 : 0);
//	output_stream << "\t\t deny_transacted     " << setting  << "\n";

	setting = (getParcelFlag(PF_DENY_AGEUNVERIFIED) ? 1 : 0);
	output_stream << "\t\t deny_age_unverified			 " << setting  << "\n";

	setting = (getParcelFlag(PF_RESTRICT_PUSHOBJECT) ? 1 : 0);
	output_stream << "\t\t restrict_pushobject " << setting  << "\n";

	output_stream << "\t\t aabb_min         " 
		<< mAABBMin.mV[VX]
		<< " " << mAABBMin.mV[VY]
		<< " " << mAABBMin.mV[VZ] << "\n";

	if (!mAccessList.empty())
	{
		output_stream << "\t\t access_list " << mAccessList.size()  << "\n";
		access_map_const_iterator cit = mAccessList.begin();
		access_map_const_iterator end = mAccessList.end();

		for ( ; cit != end; ++cit)
		{
			output_stream << "\t\t{\n";
			const LLAccessEntry& entry = (*cit).second;
			entry.mID.toString(id_string);
			output_stream << "\t\t\tid " << id_string << "\n";
			output_stream << "\t\t\ttime " << entry.mTime  << "\n";
			output_stream << "\t\t\tflags " << entry.mFlags  << "\n";
			output_stream << "\t\t}\n";
		}
	}

	if (!mBanList.empty())
	{
		output_stream << "\t\t ban_list " << mBanList.size()  << "\n";
		access_map_const_iterator cit = mBanList.begin();
		access_map_const_iterator end = mBanList.end();

		for ( ; cit != end; ++cit)
		{
			output_stream << "\t\t{\n";
			const LLAccessEntry& entry = (*cit).second;
			entry.mID.toString(id_string);
			output_stream << "\t\t\tid " << id_string << "\n";
			output_stream << "\t\t\ttime " << entry.mTime  << "\n";
			output_stream << "\t\t\tflags " << entry.mFlags  << "\n";
			output_stream << "\t\t}\n";
		}
	}

	/*if (mRenterList.count() > 0)
	{
		output_stream << "\t\t renter_list " << mRenterList.count()  << "\n";
		for (i = 0; i < mRenterList.count(); i++)
		{
			output_stream << "\t\t{\n";
			const LLAccessEntry& entry = mRenterList.get(i);
			entry.mID.toString(id_string);
			output_stream << "\t\t\tid " << id_string << "\n";
			output_stream << "\t\t\ttime " << entry.mTime  << "\n";
			output_stream << "\t\t\tflags " << entry.mFlags  << "\n";
			output_stream << "\t\t}\n";
		}
	}*/

	output_stream << "\t}\n";
	output_stream.flags(old_flags);

	return TRUE;
}


// Assumes we are in a block "ParcelData"
void LLParcel::packMessage(LLMessageSystem* msg)
{
    msg->addU32Fast( _PREHASH_ParcelFlags, getParcelFlags() );
    msg->addS32Fast( _PREHASH_SalePrice, getSalePrice() );
    msg->addStringFast( _PREHASH_Name,		 getName() );
    msg->addStringFast( _PREHASH_Desc,		 getDesc() );
    msg->addStringFast( _PREHASH_MusicURL,	 getMusicURL() );
    msg->addStringFast( _PREHASH_MediaURL,	 getMediaURL() );
    msg->addU8 ( "MediaAutoScale", getMediaAutoScale () );
    msg->addUUIDFast( _PREHASH_MediaID,	 getMediaID() );
    msg->addUUIDFast( _PREHASH_GroupID,	 getGroupID() );
    msg->addS32Fast( _PREHASH_PassPrice, mPassPrice );
    msg->addF32Fast( _PREHASH_PassHours, mPassHours );
    msg->addU8Fast(	 _PREHASH_Category,	 (U8)mCategory);
    msg->addUUIDFast( _PREHASH_AuthBuyerID, mAuthBuyerID);
    msg->addUUIDFast( _PREHASH_SnapshotID, mSnapshotID);
    msg->addVector3Fast(_PREHASH_UserLocation, mUserLocation);
    msg->addVector3Fast(_PREHASH_UserLookAt, mUserLookAt);
    msg->addU8Fast(	 _PREHASH_LandingType, (U8)mLandingType);
}

// Assumes we are in a block "ParcelData"
void LLParcel::packMessage(LLSD& msg)
{
	msg["local_id"] = getLocalID();
	msg["parcel_flags"] = ll_sd_from_U32(getParcelFlags());
	msg["sale_price"] = getSalePrice();
	msg["name"] = getName();
	msg["description"] = getDesc();
	msg["music_url"] = getMusicURL();
	msg["media_url"] = getMediaURL();
	msg["media_desc"] = getMediaDesc();
	msg["media_type"] = getMediaType();
	msg["media_width"] = getMediaWidth();
	msg["media_height"] = getMediaHeight();
	msg["auto_scale"] = getMediaAutoScale();
	msg["media_loop"] = getMediaLoop();
	msg["obscure_media"] = getObscureMedia();
	msg["obscure_music"] = getObscureMusic();
	msg["media_id"] = getMediaID();
	msg["group_id"] = getGroupID();
	msg["pass_price"] = mPassPrice;
	msg["pass_hours"] = mPassHours;
	msg["category"] = (U8)mCategory;
	msg["auth_buyer_id"] = mAuthBuyerID;
	msg["snapshot_id"] = mSnapshotID;
	msg["snapshot_id"] = mSnapshotID;
	msg["user_location"] = ll_sd_from_vector3(mUserLocation);
	msg["user_look_at"] = ll_sd_from_vector3(mUserLookAt);
	msg["landing_type"] = (U8)mLandingType;

}


void LLParcel::unpackMessage(LLMessageSystem* msg)
{
	std::string buffer;
	
    msg->getU32Fast( _PREHASH_ParcelData,_PREHASH_ParcelFlags, mParcelFlags );
    msg->getS32Fast( _PREHASH_ParcelData,_PREHASH_SalePrice, mSalePrice );
    msg->getStringFast( _PREHASH_ParcelData,_PREHASH_Name, buffer );
    setName(buffer);
    msg->getStringFast( _PREHASH_ParcelData,_PREHASH_Desc, buffer );
    setDesc(buffer);
    msg->getStringFast( _PREHASH_ParcelData,_PREHASH_MusicURL, buffer );
    setMusicURL(buffer);
    msg->getStringFast( _PREHASH_ParcelData,_PREHASH_MediaURL, buffer );
    setMediaURL(buffer);
    
    // non-optimized version
    msg->getU8 ( "ParcelData", "MediaAutoScale", mMediaAutoScale );
    
    msg->getUUIDFast( _PREHASH_ParcelData,_PREHASH_MediaID, mMediaID );
    msg->getUUIDFast( _PREHASH_ParcelData,_PREHASH_GroupID, mGroupID );
    msg->getS32Fast( _PREHASH_ParcelData,_PREHASH_PassPrice, mPassPrice );
    msg->getF32Fast( _PREHASH_ParcelData,_PREHASH_PassHours, mPassHours );
    U8 category;
    msg->getU8Fast(	 _PREHASH_ParcelData,_PREHASH_Category, category);
    mCategory = (ECategory)category;
    msg->getUUIDFast( _PREHASH_ParcelData,_PREHASH_AuthBuyerID, mAuthBuyerID);
    msg->getUUIDFast( _PREHASH_ParcelData,_PREHASH_SnapshotID, mSnapshotID);
    msg->getVector3Fast(_PREHASH_ParcelData,_PREHASH_UserLocation, mUserLocation);
    msg->getVector3Fast(_PREHASH_ParcelData,_PREHASH_UserLookAt, mUserLookAt);
    U8 landing_type;
    msg->getU8Fast(	 _PREHASH_ParcelData,_PREHASH_LandingType, landing_type);
    mLandingType = (ELandingType)landing_type;

	// New Media Data
	// Note: the message has been converted to TCP
	if(msg->has("MediaData"))
	{
		msg->getString("MediaData", "MediaDesc", buffer);
		setMediaDesc(buffer);
		msg->getString("MediaData", "MediaType", buffer);
		setMediaType(buffer);
		msg->getS32("MediaData", "MediaWidth", mMediaWidth);
		msg->getS32("MediaData", "MediaHeight", mMediaHeight);
		msg->getU8 ( "MediaData", "MediaLoop", mMediaLoop );
		msg->getU8 ( "MediaData", "ObscureMedia", mObscureMedia );
		msg->getU8 ( "MediaData", "ObscureMusic", mObscureMusic );
	}
	else
	{
		setMediaType(std::string("video/vnd.secondlife.qt.legacy"));
		setMediaDesc(std::string("No Description available without Server Upgrade"));
		mMediaLoop = true;
		mObscureMedia = true;
		mObscureMusic = true;
	}
}

void LLParcel::packAccessEntries(LLMessageSystem* msg,
								 const std::map<LLUUID,LLAccessEntry>& list)
{
    access_map_const_iterator cit = list.begin();
    access_map_const_iterator end = list.end();
    
    if (cit == end)
    {
        msg->nextBlockFast(_PREHASH_List);
        msg->addUUIDFast(_PREHASH_ID, LLUUID::null );
        msg->addS32Fast(_PREHASH_Time, 0 );
        msg->addU32Fast(_PREHASH_Flags, 0 );
        return;
    }
    
    for ( ; cit != end; ++cit)
    {
        const LLAccessEntry& entry = (*cit).second;
        
        msg->nextBlockFast(_PREHASH_List);
        msg->addUUIDFast(_PREHASH_ID,	 entry.mID );
        msg->addS32Fast(_PREHASH_Time,	 entry.mTime );
        msg->addU32Fast(_PREHASH_Flags, entry.mFlags );
    }
}


void LLParcel::unpackAccessEntries(LLMessageSystem* msg,
                                   std::map<LLUUID,LLAccessEntry>* list)
{
    LLUUID id;
    S32 time;
    U32 flags;
    
    S32 i;
    S32 count = msg->getNumberOfBlocksFast(_PREHASH_List);
    for (i = 0; i < count; i++)
    {
        msg->getUUIDFast(_PREHASH_List, _PREHASH_ID, id, i);
        msg->getS32Fast(		 _PREHASH_List, _PREHASH_Time, time, i);
        msg->getU32Fast(		 _PREHASH_List, _PREHASH_Flags, flags, i);
        
        if (id.notNull())
        {
            LLAccessEntry entry;
            entry.mID = id;
            entry.mTime = time;
            entry.mFlags = flags;
            
            (*list)[entry.mID] = entry;
        }
    }
}


void LLParcel::expirePasses(S32 now)
{
    access_map_iterator itor = mAccessList.begin();
    while (itor != mAccessList.end())
    {
        const LLAccessEntry& entry = (*itor).second;
        
        if (entry.mTime != 0 && entry.mTime < now)
        {
            mAccessList.erase(itor++);
        }
        else
        {
            ++itor;
        }
    }
}


bool LLParcel::operator==(const LLParcel &rhs) const
{
    if (mOwnerID != rhs.mOwnerID)
        return FALSE;
    
    if (mParcelFlags != rhs.mParcelFlags)
        return FALSE;
    
    if (mClaimDate != rhs.mClaimDate)
        return FALSE;
    
    if (mClaimPricePerMeter != rhs.mClaimPricePerMeter)
        return FALSE;
    
    if (mRentPricePerMeter != rhs.mRentPricePerMeter)
        return FALSE;
    
    return TRUE;
}

// Calculate rent
S32 LLParcel::getTotalRent() const
{
    return (S32)floor(0.5f + (F32)mArea * (F32)mRentPricePerMeter * (1.0f - mDiscountRate));
}

F32 LLParcel::getAdjustedRentPerMeter() const
{
    return ((F32)mRentPricePerMeter * (1.0f - mDiscountRate));
}

LLVector3 LLParcel::getCenterpoint() const
{
    LLVector3 rv;
    rv.mV[VX] = (getAABBMin().mV[VX] + getAABBMax().mV[VX]) * 0.5f;
    rv.mV[VY] = (getAABBMin().mV[VY] + getAABBMax().mV[VY]) * 0.5f;
    rv.mV[VZ] = 0.0f;
    return rv;
}

void LLParcel::extendAABB(const LLVector3& box_min, const LLVector3& box_max)
{
    // Patch up min corner of AABB
    S32 i;
    for (i=0; i<3; i++)
    {
        if (box_min.mV[i] < mAABBMin.mV[i])
        {
            mAABBMin.mV[i] = box_min.mV[i];
        }
    }
    
    // Patch up max corner of AABB
    for (i=0; i<3; i++)
    {
        if (box_max.mV[i] > mAABBMax.mV[i])
        {
            mAABBMax.mV[i] = box_max.mV[i];
        }
    }
}

BOOL LLParcel::addToAccessList(const LLUUID& agent_id, S32 time)
{
	if (mAccessList.size() >= (U32) PARCEL_MAX_ACCESS_LIST)
	{
		return FALSE;
	}
	if (agent_id == getOwnerID())
	{
		// Can't add owner to these lists
		return FALSE;
	}
	access_map_iterator itor = mAccessList.begin();
	while (itor != mAccessList.end())
	{
		const LLAccessEntry& entry = (*itor).second;
		if (entry.mID == agent_id)
		{
			if (time == 0 || (entry.mTime != 0 && entry.mTime < time))
			{
				mAccessList.erase(itor++);
			}
			else
			{
				// existing one expires later
				return FALSE;
			}
		}
		else
		{
			++itor;
		}
	}
    
    removeFromBanList(agent_id);
    
    LLAccessEntry new_entry;
    new_entry.mID			 = agent_id;
    new_entry.mTime	 = time;
    new_entry.mFlags = 0x0;
    mAccessList[new_entry.mID] = new_entry;
    return TRUE;
}

BOOL LLParcel::addToBanList(const LLUUID& agent_id, S32 time)
{
	if (mBanList.size() >= (U32) PARCEL_MAX_ACCESS_LIST)
	{
		// Not using ban list, so not a rational thing to do
		return FALSE;
	}
	if (agent_id == getOwnerID())
	{
		// Can't add owner to these lists
		return FALSE;
	}
    
    access_map_iterator itor = mBanList.begin();
    while (itor != mBanList.end())
    {
        const LLAccessEntry& entry = (*itor).second;
        if (entry.mID == agent_id)
        {
            if (time == 0 || (entry.mTime != 0 && entry.mTime < time))
            {
                mBanList.erase(itor++);
            }
            else
            {
                // existing one expires later
                return FALSE;
            }
        }
        else
        {
            ++itor;
        }
    }
    
    removeFromAccessList(agent_id);
    
    LLAccessEntry new_entry;
    new_entry.mID			 = agent_id;
    new_entry.mTime	 = time;
    new_entry.mFlags = 0x0;
    mBanList[new_entry.mID] = new_entry;
    return TRUE;
}

BOOL remove_from_access_array(std::map<LLUUID,LLAccessEntry>* list,
                              const LLUUID& agent_id)
{
    BOOL removed = FALSE;
    access_map_iterator itor = list->begin();
    while (itor != list->end())
    {
        const LLAccessEntry& entry = (*itor).second;
        if (entry.mID == agent_id)
        {
            list->erase(itor++);
            removed = TRUE;
        }
        else
        {
            ++itor;
        }
    }
    return removed;
}

BOOL LLParcel::removeFromAccessList(const LLUUID& agent_id)
{
    return remove_from_access_array(&mAccessList, agent_id);
}

BOOL LLParcel::removeFromBanList(const LLUUID& agent_id)
{
    return remove_from_access_array(&mBanList, agent_id);
}

// static
const std::string& LLParcel::getOwnershipStatusString(EOwnershipStatus status)
{
    return ownership_status_to_string(status);
}

// static
const std::string& LLParcel::getCategoryString(ECategory category)
{
    return category_to_string(category);
}

// static
const std::string& LLParcel::getCategoryUIString(ECategory category)
{
    return category_to_ui_string(category);
}

// static
LLParcel::ECategory LLParcel::getCategoryFromString(const std::string& string)
{
    return category_string_to_category(string);
}

// static
LLParcel::ECategory LLParcel::getCategoryFromUIString(const std::string& string)
{
    return category_ui_string_to_category(string);
}

// static
const std::string& LLParcel::getActionString(LLParcel::EAction action)
{
    S32 index = 0;
    if((action >= 0) && (action < LLParcel::A_COUNT))
    {
        index = action;
    }
    else
    {
        index = A_COUNT;
    }
    return PARCEL_ACTION_STRING[index];
}

BOOL LLParcel::isSaleTimerExpired(const U64& time)
{
    if (mSaleTimerExpires.getStarted() == FALSE)
    {
        return FALSE;
    }
    BOOL expired = mSaleTimerExpires.checkExpirationAndReset(0.0);
    if (expired)
    {
        mSaleTimerExpires.stop();
    }
    return expired;
}


void LLParcel::startSale(const LLUUID& buyer_id, BOOL is_buyer_group)
{
	// TODO -- this and all Sale related methods need to move out of the LLParcel 
	// base class and into server-side-only LLSimParcel class
	setPreviousOwnerID(mOwnerID);
	setPreviouslyGroupOwned(mGroupOwned);

	mOwnerID = buyer_id;
	mGroupOwned = is_buyer_group;
	if(mGroupOwned)
	{
		mGroupID = mOwnerID;
	}
	else
	{
		mGroupID.setNull();
	}
	mSaleTimerExpires.start();
	mSaleTimerExpires.setTimerExpirySec(DEFAULT_USEC_SALE_TIMEOUT / SEC_TO_MICROSEC);
	mStatus = OS_LEASE_PENDING;
	mClaimDate = time(NULL);
	setAuctionID(0);
	// clear the autoreturn whenever land changes hands
	setCleanOtherTime(0);
}

void LLParcel::expireSale(
	U32& type,
	U8& flags,
	LLUUID& from_id,
	LLUUID& to_id)
{
    mSaleTimerExpires.setTimerExpirySec(0.0);
    mSaleTimerExpires.stop();
    setPreviousOwnerID(LLUUID::null);
    setPreviouslyGroupOwned(FALSE);
    setSellWithObjects(FALSE);
    type = TRANS_LAND_RELEASE;
    mStatus = OS_NONE;
    flags = pack_transaction_flags(mGroupOwned, FALSE);
    mAuthBuyerID.setNull();
    from_id = mOwnerID;
    mOwnerID.setNull();
    to_id.setNull();
}

void LLParcel::completeSale(
	U32& type,
	U8& flags,
	LLUUID& to_id)
{
	mSaleTimerExpires.setTimerExpirySec(0.0);
	mSaleTimerExpires.stop();
	mStatus = OS_LEASED;
	type = TRANS_LAND_SALE;
	flags = pack_transaction_flags(mGroupOwned, mGroupOwned);
	to_id = mOwnerID;
	mAuthBuyerID.setNull();

	// Purchased parcels are assumed to no longer be for sale.
	// Otherwise someone can snipe the sale.
	setForSale(FALSE);
	setAuctionID(0);

	// Turn off show directory, since it's a recurring fee that
	// the buyer may not want.
	setParcelFlag(PF_SHOW_DIRECTORY, FALSE);

	//should be cleared on sale.
	mAccessList.clear();
	mBanList.clear();
}

void LLParcel::clearSale()
{
	mSaleTimerExpires.setTimerExpirySec(0.0);
	mSaleTimerExpires.stop();
	if(isPublic())
	{
		mStatus = OS_NONE;
	}
	else
	{
		mStatus = OS_LEASED;
	}
	mAuthBuyerID.setNull();
	setForSale(FALSE);
	setAuctionID(0);
	setPreviousOwnerID(LLUUID::null);
	setPreviouslyGroupOwned(FALSE);
	setSellWithObjects(FALSE);
}

BOOL LLParcel::isPublic() const
{
    return (mOwnerID.isNull());
}

BOOL LLParcel::isBuyerAuthorized(const LLUUID& buyer_id) const
{
    if(mAuthBuyerID.isNull())
    {
        return TRUE;
    }
    return (mAuthBuyerID == buyer_id);
}

void LLParcel::clearParcel()
{
	overrideParcelFlags(PF_DEFAULT);
	setName(LLStringUtil::null);
	setDesc(LLStringUtil::null);
	setMediaURL(LLStringUtil::null);
	setMediaType(LLStringUtil::null);
	setMediaID(LLUUID::null);
    setMediaDesc(LLStringUtil::null);
	setMediaAutoScale(0);
	setMediaLoop(TRUE);
	mObscureMedia = 1;
	mObscureMusic = 1;
	mMediaWidth = 0;
	mMediaHeight = 0;
	setMusicURL(LLStringUtil::null);
	setInEscrow(FALSE);
	setAuthorizedBuyerID(LLUUID::null);
	setCategory(C_NONE);
	setSnapshotID(LLUUID::null);
	setUserLocation(LLVector3::zero);
	setUserLookAt(LLVector3::x_axis);
	setLandingType(L_LANDING_POINT);
	setAuctionID(0);
	setGroupID(LLUUID::null);
	setPassPrice(0);
	setPassHours(0.f);
	mAccessList.clear();
	mBanList.clear();
	//mRenterList.reset();
}

void LLParcel::dump()
{
    llinfos << "parcel " << mLocalID << " area " << mArea << llendl;
    llinfos << "	 name <" << mName << ">" << llendl;
    llinfos << "	 desc <" << mDesc << ">" << llendl;
}

const std::string& ownership_status_to_string(LLParcel::EOwnershipStatus status)
{
    if(status >= 0 && status < LLParcel::OS_COUNT)
    {
        return PARCEL_OWNERSHIP_STATUS_STRING[status];
    }
    return PARCEL_OWNERSHIP_STATUS_STRING[LLParcel::OS_COUNT];
}

LLParcel::EOwnershipStatus ownership_string_to_status(const std::string& s)
{
    for(S32 i = 0; i < LLParcel::OS_COUNT; ++i)
    {
        if(s == PARCEL_OWNERSHIP_STATUS_STRING[i])
        {
            return (LLParcel::EOwnershipStatus)i;
        }
    }
    return LLParcel::OS_NONE;
}

//const char* revert_action_to_string(LLParcel::ESaleTimerExpireAction action)
//{
// S32 index = 0;
// if(action >= 0 && action < LLParcel::STEA_COUNT)
// {
//	 index = action;
// }
// return PARCEL_SALE_TIMER_ACTION[index];
//}
    
//LLParcel::ESaleTimerExpireAction revert_string_to_action(const char* s)
//{
// for(S32 i = 0; i < LLParcel::STEA_COUNT; ++i)
// {
//	 if(0 == strcmp(s, PARCEL_SALE_TIMER_ACTION[i]))
//	 {
//		 return (LLParcel::ESaleTimerExpireAction)i;
//	 }
// }
// return LLParcel::STEA_REVERT;
//}
    
const std::string& category_to_string(LLParcel::ECategory category)
{
    S32 index = 0;
    if((category >= 0) && (category < LLParcel::C_COUNT))
    {
        index = category;
    }
    return PARCEL_CATEGORY_STRING[index];
}

const std::string& category_to_ui_string(LLParcel::ECategory category)
{
    S32 index = 0;
    if((category >= 0) && (category < LLParcel::C_COUNT))
    {
        index = category;
    }
    else
    {
        // C_ANY = -1 , but the "Any" string is at the end of the list
        index = ((S32) LLParcel::C_COUNT);
    }
    return PARCEL_CATEGORY_UI_STRING[index];
}

LLParcel::ECategory category_string_to_category(const std::string& s)
{
    for(S32 i = 0; i < LLParcel::C_COUNT; ++i)
    {
        if(s == PARCEL_CATEGORY_STRING[i])
        {
            return (LLParcel::ECategory)i;
        }
    }
    llwarns << "Parcel category outside of possibilities " << s << llendl;
    return LLParcel::C_NONE;
}

LLParcel::ECategory category_ui_string_to_category(const std::string& s)
{
    for(S32 i = 0; i < LLParcel::C_COUNT; ++i)
    {
        if(s == PARCEL_CATEGORY_UI_STRING[i])
        {
            return (LLParcel::ECategory)i;
        }
    }
    // "Any" is a valid category for searches, and
    // is a distinct option from "None" and "Other"
    return LLParcel::C_ANY;
}
