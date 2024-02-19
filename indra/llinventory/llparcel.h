/** 
 * @file llparcel.h
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
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

#ifndef LL_LLPARCEL_H
#define LL_LLPARCEL_H

#include <time.h>
#include <iostream>

#include "lluuid.h"
#include "llparcelflags.h"
#include "llpermissions.h"
#include "lltimer.h"
#include "v3math.h"
#include "llsettingsdaycycle.h"

// Grid out of which parcels taken is stepped every 4 meters.
const F32 PARCEL_GRID_STEP_METERS	= 4.f;

// Area of one "square" of parcel
const S32 PARCEL_UNIT_AREA			= 16;

// Height _above_ground_ that parcel boundary ends
const F32 PARCEL_HEIGHT = 50.f;

//Height above ground which parcel boundries exist for explicitly banned avatars
const F32 BAN_HEIGHT = 5000.f;

// Maximum number of entries in an access list
const S32 PARCEL_MAX_ACCESS_LIST = 300;
//Maximum number of entires in an update packet
//for access/ban lists.
const F32 PARCEL_MAX_ENTRIES_PER_PACKET = 48.f;

// Maximum number of experiences
const S32 PARCEL_MAX_EXPERIENCE_LIST = 24;

// Weekly charge for listing a parcel in the directory
const S32 PARCEL_DIRECTORY_FEE = 30;

const S32 PARCEL_PASS_PRICE_DEFAULT = 10;
const F32 PARCEL_PASS_HOURS_DEFAULT = 1.f;

// Number of "chunks" in which parcel overlay data is sent
// Chunk 0 = southern rows, entire width
const S32 PARCEL_OVERLAY_CHUNKS = 4;

// Bottom three bits are a color index for the land overlay
const U8 PARCEL_COLOR_MASK	= 0x07;
const U8 PARCEL_PUBLIC		= 0x00;
const U8 PARCEL_OWNED		= 0x01;
const U8 PARCEL_GROUP		= 0x02;
const U8 PARCEL_SELF		= 0x03;
const U8 PARCEL_FOR_SALE	= 0x04;
const U8 PARCEL_AUCTION		= 0x05;
// unused 0x06
// unused 0x07
// flag, unused 0x08
const U8 PARCEL_HIDDENAVS   = 0x10;	// avatars not visible outside of parcel.  Used for 'see avs' feature, but must be off for compatibility
const U8 PARCEL_SOUND_LOCAL = 0x20;
const U8 PARCEL_WEST_LINE	= 0x40;	// flag, property line on west edge
const U8 PARCEL_SOUTH_LINE	= 0x80;	// flag, property line on south edge

// Transmission results for parcel properties
const S32 PARCEL_RESULT_NO_DATA = -1;
const S32 PARCEL_RESULT_SUCCESS = 0;	// got exactly one parcel
const S32 PARCEL_RESULT_MULTIPLE = 1;	// got multiple parcels

const S32 SELECTED_PARCEL_SEQ_ID = -10000;
const S32 COLLISION_NOT_IN_GROUP_PARCEL_SEQ_ID =  -20000;
const S32 COLLISION_BANNED_PARCEL_SEQ_ID = -30000;
const S32 COLLISION_NOT_ON_LIST_PARCEL_SEQ_ID =  -40000;
const S32 HOVERED_PARCEL_SEQ_ID =  -50000;

const U32 RT_NONE	= 0x1 << 0;
const U32 RT_OWNER	= 0x1 << 1;
const U32 RT_GROUP	= 0x1 << 2;
const U32 RT_OTHER	= 0x1 << 3;
const U32 RT_LIST	= 0x1 << 4;
const U32 RT_SELL	= 0x1 << 5;

const S32 INVALID_PARCEL_ID = -1;

const S32 INVALID_PARCEL_ENVIRONMENT_VERSION = -2;
// if Region settings are used, parcel env. version is -1
const S32 UNSET_PARCEL_ENVIRONMENT_VERSION = -1;

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



class LLMessageSystem;
class LLSD;

class LLAccessEntry
{
public:

	typedef std::map<LLUUID,LLAccessEntry> map;

	LLAccessEntry()
	:	mTime(0),
		mFlags(0)
	{}

	LLUUID		mID;		// Agent ID
	S32			mTime;		// Time (unix seconds) when entry expires
	U32			mFlags;		// Not used - currently should always be zero
};


class LLParcel
{
public:
	enum EOwnershipStatus
	{
		OS_LEASED = 0,
		OS_LEASE_PENDING = 1,
		OS_ABANDONED = 2,
		OS_COUNT = 3,
		OS_NONE = -1
	};
	enum ECategory
	{
		C_NONE = 0,
		C_LINDEN,
		C_ADULT,
		C_ARTS,			// "arts & culture"
		C_BUSINESS,		// was "store"
		C_EDUCATIONAL,
		C_GAMING,		// was "game"
		C_HANGOUT,		// was "gathering place"
		C_NEWCOMER,
		C_PARK,			// "parks & nature"
		C_RESIDENTIAL,	// was "homestead"
		C_SHOPPING,
		C_STAGE,
		C_OTHER,
		C_RENTAL,
		C_COUNT,
		C_ANY = -1		// only useful in queries
	};
	enum EAction
	{
		A_CREATE = 0,
		A_RELEASE = 1,
		A_ABSORB = 2,
		A_ABSORBED = 3,
		A_DIVIDE = 4,
		A_DIVISION = 5,
		A_ACQUIRE = 6,
		A_RELINQUISH = 7,
		A_CONFIRM = 8,
		A_COUNT = 9,
		A_UNKNOWN = -1
	};

	enum ELandingType
	{
		L_NONE = 0,
		L_LANDING_POINT = 1,
		L_DIRECT = 2
	};

	// CREATORS
	LLParcel();
	LLParcel(
		const LLUUID &owner_id,
		bool modify,
		bool terraform,
		bool damage,
		time_t claim_date,
		S32 claim_price,
		S32 rent_price,
		S32 area,
		S32 sim_object_limit,
		F32 parcel_object_bonus,
		bool is_group_owned = false);
	virtual ~LLParcel();

	void init(
		const LLUUID &owner_id,
		bool modify,
		bool terraform,
		bool damage,
		time_t claim_date,
		S32 claim_price,
		S32 rent_price,
		S32 area,
		S32 sim_object_limit,
		F32 parcel_object_bonus,
		bool is_group_owned = false);

	// TODO: make an actual copy constructor for this
	void overrideParcelFlags(U32 flags);
	// if you specify an agent id here, the group id will be zeroed
	void overrideOwner(
		const LLUUID& owner_id,
		bool is_group_owned = false);
	void overrideSaleTimerExpires(F32 secs_left) { mSaleTimerExpires.setTimerExpirySec(secs_left); }

	// MANIPULATORS
	void generateNewID() { mID.generate(); }
	void setName(const std::string& name);
	void setDesc(const std::string& desc);
	void setMusicURL(const std::string& url);
	void setMediaURL(const std::string& url);
	void setMediaType(const std::string& type);
	void setMediaDesc(const std::string& desc);
	void	setMediaID(const LLUUID& id) { mMediaID = id; }
	void	setMediaAutoScale ( U8 flagIn ) { mMediaAutoScale = flagIn; }
	void    setMediaLoop (U8 loop) { mMediaLoop = loop; }
	void setMediaWidth(S32 width);
	void setMediaHeight(S32 height);
	void setMediaCurrentURL(const std::string& url);
	void setMediaAllowNavigate(U8 enable) { mMediaAllowNavigate = enable; }
	void setMediaURLTimeout(F32 timeout) { mMediaURLTimeout = timeout; }
	void setMediaPreventCameraZoom(U8 enable) { mMediaPreventCameraZoom = enable; }

	void setMediaURLResetTimer(F32 time);
	virtual void	setLocalID(S32 local_id);

	// blow away all the extra stuff lurking in parcels, including urls, access lists, etc
	void clearParcel();

	// This value is not persisted out to the parcel file, it is only
	// a per-process blocker for attempts to purchase.
	void setInEscrow(bool in_escrow) { mInEscrow = in_escrow; }

	void setAuthorizedBuyerID(const LLUUID& id) { mAuthBuyerID = id; }
	//void overrideBuyerID(const LLUUID& id) { mBuyerID = id; }
	void setCategory(ECategory category) { mCategory = category; }
	void setSnapshotID(const LLUUID& id) { mSnapshotID = id; }
	void setUserLocation(const LLVector3& pos)	{ mUserLocation = pos; }
	void setUserLookAt(const LLVector3& rot)	{ mUserLookAt = rot; }
	void setLandingType(const ELandingType type) { mLandingType = type; }
	void setSeeAVs(bool see_avs)	{ mSeeAVs = see_avs;	}
	void setHaveNewParcelLimitData(bool have_new_parcel_data)		{ mHaveNewParcelLimitData = have_new_parcel_data;		}		// Remove this once hidden AV feature is fully available grid-wide

	void setAuctionID(U32 auction_id) { mAuctionID = auction_id;}

	void	setAllParcelFlags(U32 flags);
	void	setParcelFlag(U32 flag, bool b);

	virtual void setArea(S32 area, S32 sim_object_limit);
	void	setDiscountRate(F32 rate);

	void	setAllowModify(bool b)	{ setParcelFlag(PF_CREATE_OBJECTS, b); }
	void	setAllowGroupModify(bool b)	{ setParcelFlag(PF_CREATE_GROUP_OBJECTS, b); }
	void	setAllowAllObjectEntry(bool b)	{ setParcelFlag(PF_ALLOW_ALL_OBJECT_ENTRY, b); }
	void	setAllowGroupObjectEntry(bool b)	{ setParcelFlag(PF_ALLOW_GROUP_OBJECT_ENTRY, b); }
	void	setAllowTerraform(bool b){setParcelFlag(PF_ALLOW_TERRAFORM, b); }
	void	setAllowDamage(bool b)	{ setParcelFlag(PF_ALLOW_DAMAGE, b); }
	void	setAllowFly(bool b)		{ setParcelFlag(PF_ALLOW_FLY, b); }
	void	setAllowGroupScripts(bool b)	{ setParcelFlag(PF_ALLOW_GROUP_SCRIPTS, b); }
	void	setAllowOtherScripts(bool b)	{ setParcelFlag(PF_ALLOW_OTHER_SCRIPTS, b); }
	void	setAllowDeedToGroup(bool b) { setParcelFlag(PF_ALLOW_DEED_TO_GROUP, b); }
	void    setContributeWithDeed(bool b) { setParcelFlag(PF_CONTRIBUTE_WITH_DEED, b); }
	void	setForSale(bool b)		{ setParcelFlag(PF_FOR_SALE, b); }
	void	setSoundOnly(bool b)	{ setParcelFlag(PF_SOUND_LOCAL, b); }
	void	setDenyAnonymous(bool b) { setParcelFlag(PF_DENY_ANONYMOUS, b); }
	void	setDenyAgeUnverified(bool b) { setParcelFlag(PF_DENY_AGEUNVERIFIED, b); }
	void	setRestrictPushObject(bool b) { setParcelFlag(PF_RESTRICT_PUSHOBJECT, b); }
	void	setAllowGroupAVSounds(bool b)	{ mAllowGroupAVSounds = b;		}
	void	setAllowAnyAVSounds(bool b)		{ mAllowAnyAVSounds = b;		}
    void    setObscureMOAP(bool b)  { mObscureMOAP = b; }

	void	setDrawDistance(F32 dist)	{ mDrawDistance = dist; }
	void	setSalePrice(S32 price)		{ mSalePrice = price; }
	void	setGroupID(const LLUUID& id)	{ mGroupID = id; }
	//void	setGroupName(const std::string& s)	{ mGroupName.assign(s); }
	void	setPassPrice(S32 price)				{ mPassPrice = price; }
	void	setPassHours(F32 hours)				{ mPassHours = hours; }

//	bool	importStream(std::istream& input_stream);
	bool	importAccessEntry(std::istream& input_stream, LLAccessEntry* entry);
	// bool	exportStream(std::ostream& output_stream);

	void	packMessage(LLMessageSystem* msg);
	void	packMessage(LLSD& msg);
	void	unpackMessage(LLMessageSystem* msg);

	void	packAccessEntries(LLMessageSystem* msg,
								const std::map<LLUUID,LLAccessEntry>& list);
	void	unpackAccessEntries(LLMessageSystem* msg,
								std::map<LLUUID,LLAccessEntry>* list);

	void	unpackExperienceEntries(LLMessageSystem* msg, U32 type);


	void	setAABBMin(const LLVector3& min)	{ mAABBMin = min; }
	void	setAABBMax(const LLVector3& max)	{ mAABBMax = max; }

	// Extend AABB to include rectangle from min to max.
	void extendAABB(const LLVector3& box_min, const LLVector3& box_max);

	void dump();

	// Scans the pass list and removes any items with an expiration
	// time earlier than "now".
	void expirePasses(S32 now);

	// Add to list, suppressing duplicates.  Returns TRUE if added.
	bool addToAccessList(const LLUUID& agent_id, S32 time);
	bool addToBanList(const LLUUID& agent_id, S32 time);
	bool removeFromAccessList(const LLUUID& agent_id);
	bool removeFromBanList(const LLUUID& agent_id);

	// ACCESSORS
	const LLUUID&	getID() const				{ return mID; }
	const std::string&	getName() const			{ return mName; }
	const std::string&	getDesc() const			{ return mDesc; }
	const std::string&	getMusicURL() const		{ return mMusicURL; }
	const std::string&	getMediaURL() const		{ return mMediaURL; }
	const std::string&	getMediaDesc() const	{ return mMediaDesc; }
	const std::string&	getMediaType() const	{ return mMediaType; }
	const LLUUID&	getMediaID() const			{ return mMediaID; }
	S32				getMediaWidth() const		{ return mMediaWidth; }
	S32				getMediaHeight() const		{ return mMediaHeight; }
	U8				getMediaAutoScale() const	{ return mMediaAutoScale; }
	U8              getMediaLoop() const        { return mMediaLoop; }
	const std::string&  getMediaCurrentURL() const { return mMediaCurrentURL; }
	U8              getMediaAllowNavigate() const { return mMediaAllowNavigate; }
	F32				getMediaURLTimeout() const { return mMediaURLTimeout; }
	U8              getMediaPreventCameraZoom() const { return mMediaPreventCameraZoom; }

	S32				getLocalID() const			{ return mLocalID; }
	const LLUUID&	getOwnerID() const			{ return mOwnerID; }
	const LLUUID&	getGroupID() const			{ return mGroupID; }
	S32				getPassPrice() const		{ return mPassPrice; }
	F32				getPassHours() const		{ return mPassHours; }
	bool			getIsGroupOwned() const		{ return mGroupOwned; }

	U32 getAuctionID() const	{ return mAuctionID; }
	bool isInEscrow() const		{ return mInEscrow; }

	bool isPublic() const;

	// Region-local user-specified position
	const LLVector3& getUserLocation() const	{ return mUserLocation; }
	const LLVector3& getUserLookAt() const	{ return mUserLookAt; }
	ELandingType getLandingType() const	{ return mLandingType; }
	bool getSeeAVs() const			{ return mSeeAVs;		}
	bool getHaveNewParcelLimitData() const		{ return mHaveNewParcelLimitData;	}

	// User-specified snapshot
	const LLUUID&	getSnapshotID() const		{ return mSnapshotID; }

	// the authorized buyer id is the person who is the only
	// agent/group that has authority to purchase. (ie, ui specified a
	// particular agent could buy the plot).
	const LLUUID& getAuthorizedBuyerID() const { return mAuthBuyerID; }

	// helper function
	bool isBuyerAuthorized(const LLUUID& buyer_id) const;

	// The buyer of a plot is set when someone indicates they want to
	// buy the plot, and the system is simply waiting for tier-up
	// approval
	//const LLUUID& getBuyerID() const { return mBuyerID; }

	// functions to deal with ownership status.
	EOwnershipStatus getOwnershipStatus() const { return mStatus; }
	static const std::string& getOwnershipStatusString(EOwnershipStatus status);
	void setOwnershipStatus(EOwnershipStatus status) { mStatus = status; }

	// dealing with parcel category information
	ECategory getCategory() const {return mCategory; }
	static const std::string& getCategoryString(ECategory category);
	static const std::string& getCategoryUIString(ECategory category);
	static ECategory getCategoryFromString(const std::string& string);
	static ECategory getCategoryFromUIString(const std::string& string);

	// functions for parcel action (used for logging)
	static const std::string& getActionString(EAction action);

	// dealing with sales and parcel conversion.
	//
	// the isSaleTimerExpired will trivially return FALSE if there is
	// no sale going on. Pass in the current time in usec which will
	// be used for comparison.
	bool isSaleTimerExpired(const U64& time);

	F32 getSaleTimerExpires() { return mSaleTimerExpires.getRemainingTimeF32(); }

	// should the parcel join on complete?
	//U32 getJoinNeighbors() const { return mJoinNeighbors; }

	// need to record a few things with the parcel when a sale
	// starts.
	void startSale(const LLUUID& buyer_id, bool is_buyer_group);

	// do the expiration logic, which needs to return values usable in
	// a L$ transaction.
	void expireSale(U32& type, U8& flags, LLUUID& from_id, LLUUID& to_id);
	void completeSale(U32& type, U8& flags, LLUUID& to_id);
	void clearSale();


	bool isMediaResetTimerExpired(const U64& time);


	// more accessors
	U32		getParcelFlags() const			{ return mParcelFlags; }

	bool	getParcelFlag(U32 flag) const
					{ return (mParcelFlags & flag) ? true : false; }

	// objects can be added or modified by anyone (only parcel owner if disabled)
	bool	getAllowModify() const
					{ return (mParcelFlags & PF_CREATE_OBJECTS) ? true : false; }

	// objects can be added or modified by group members
	bool	getAllowGroupModify() const
					{ return (mParcelFlags & PF_CREATE_GROUP_OBJECTS) ? true : false; }

	// the parcel can be deeded to the group
	bool	getAllowDeedToGroup() const
					{ return (mParcelFlags & PF_ALLOW_DEED_TO_GROUP) ? true : false; }

	// Does the owner want to make a contribution along with the deed.
	bool getContributeWithDeed() const
	{ return (mParcelFlags & PF_CONTRIBUTE_WITH_DEED) ? true : false; }

	// heightfield can be modified
	bool	getAllowTerraform() const
					{ return (mParcelFlags & PF_ALLOW_TERRAFORM) ? true : false; }

	// avatars can be hurt here
	bool	getAllowDamage() const
					{ return (mParcelFlags & PF_ALLOW_DAMAGE) ? true : false; }

	bool	getAllowFly() const
					{ return (mParcelFlags & PF_ALLOW_FLY) ? true : false; }

	bool	getAllowGroupScripts() const
					{ return (mParcelFlags & PF_ALLOW_GROUP_SCRIPTS) ? true : false; }

	bool	getAllowOtherScripts() const
					{ return (mParcelFlags & PF_ALLOW_OTHER_SCRIPTS) ? true : false; }

	bool	getAllowAllObjectEntry() const
					{ return (mParcelFlags & PF_ALLOW_ALL_OBJECT_ENTRY) ? true : false; }

	bool	getAllowGroupObjectEntry() const
					{ return (mParcelFlags & PF_ALLOW_GROUP_OBJECT_ENTRY) ? true : false; }

	bool	getForSale() const
					{ return (mParcelFlags & PF_FOR_SALE) ? true : false; }
	bool	getSoundLocal() const
					{ return (mParcelFlags & PF_SOUND_LOCAL) ? true : false; }
	bool	getParcelFlagAllowVoice() const
					{ return (mParcelFlags & PF_ALLOW_VOICE_CHAT) ? true : false; }
	bool	getParcelFlagUseEstateVoiceChannel() const
					{ return (mParcelFlags & PF_USE_ESTATE_VOICE_CHAN) ? true : false; }
	bool	getAllowPublish() const
					{ return (mParcelFlags & PF_ALLOW_PUBLISH) ? true : false; }
	bool	getMaturePublish() const
					{ return (mParcelFlags & PF_MATURE_PUBLISH) ? true : false; }
	bool	getRestrictPushObject() const
					{ return (mParcelFlags & PF_RESTRICT_PUSHOBJECT) ? true : false; }
	bool	getRegionPushOverride() const
					{ return mRegionPushOverride; }
	bool	getRegionDenyAnonymousOverride() const
					{ return mRegionDenyAnonymousOverride; }
	bool	getRegionDenyAgeUnverifiedOverride() const
					{ return mRegionDenyAgeUnverifiedOverride; }
    bool    getRegionAllowAccessOverride() const
                    { return mRegionAllowAccessoverride; }
    bool    getRegionAllowEnvironmentOverride() const
                    { return mRegionAllowEnvironmentOverride; }
    S32     getParcelEnvironmentVersion() const 
                    { return mCurrentEnvironmentVersion; }


	bool	getAllowGroupAVSounds()	const	{ return mAllowGroupAVSounds;	}
	bool	getAllowAnyAVSounds()	const	{ return mAllowAnyAVSounds;		}
 
    bool    getObscureMOAP() const { return mObscureMOAP; }

	F32		getDrawDistance() const			{ return mDrawDistance; }
	S32		getSalePrice() const			{ return mSalePrice; }
	time_t	getClaimDate() const			{ return mClaimDate; }
	S32		getClaimPricePerMeter() const	{ return mClaimPricePerMeter; }
	S32		getRentPricePerMeter() const	{ return mRentPricePerMeter; }

	// Area is NOT automatically calculated.  You must calculate it
	// and store it with setArea.
	S32		getArea() const					{ return mArea; }

	// deprecated 12/11/2003
	//F32		getDiscountRate() const			{ return mDiscountRate; }

	S32		getClaimPrice() const			{ return mClaimPricePerMeter * mArea; }

	// Can this agent create objects here?
	bool	allowModifyBy(const LLUUID &agent_id, const LLUUID &group_id) const;

	// Can this agent change the shape of the land?
	bool	allowTerraformBy(const LLUUID &agent_id) const;

	bool	operator==(const LLParcel &rhs) const;

	// Calculate rent - area * rent * discount rate
	S32		getTotalRent() const;
	F32		getAdjustedRentPerMeter() const;

	const LLVector3&	getAABBMin() const		{ return mAABBMin; }
	const LLVector3&	getAABBMax() const		{ return mAABBMax; }
	LLVector3 getCenterpoint() const;

	// simwide
	S32		getSimWideMaxPrimCapacity() const { return mSimWideMaxPrimCapacity; }
	S32 getSimWidePrimCount() const { return mSimWidePrimCount; }

	// this parcel only (not simwide)
	S32		getMaxPrimCapacity() const	{ return mMaxPrimCapacity; }	// Does not include prim bonus
	S32		getPrimCount() const		{ return mOwnerPrimCount + mGroupPrimCount + mOtherPrimCount + mSelectedPrimCount; }
	S32		getOwnerPrimCount() const	{ return mOwnerPrimCount; }
	S32		getGroupPrimCount() const	{ return mGroupPrimCount; }
	S32		getOtherPrimCount() const	{ return mOtherPrimCount; }
	S32		getSelectedPrimCount() const{ return mSelectedPrimCount; }
	S32		getTempPrimCount() const	{ return mTempPrimCount; }
	F32		getParcelPrimBonus() const	{ return mParcelPrimBonus; }

	S32		getCleanOtherTime() const			{ return mCleanOtherTime; }

	void	setMaxPrimCapacity(S32 max) { mMaxPrimCapacity = max; }		// Does not include prim bonus
	// simwide
	void	setSimWideMaxPrimCapacity(S32 current)	{ mSimWideMaxPrimCapacity = current; }
	void setSimWidePrimCount(S32 current) { mSimWidePrimCount = current; }

	// this parcel only (not simwide)
	void	setOwnerPrimCount(S32 current)	{ mOwnerPrimCount = current; }
	void	setGroupPrimCount(S32 current)	{ mGroupPrimCount = current; }
	void	setOtherPrimCount(S32 current)	{ mOtherPrimCount = current; }
	void	setSelectedPrimCount(S32 current)	{ mSelectedPrimCount = current; }
	void	setTempPrimCount(S32 current)	{ mTempPrimCount = current; }
	void	setParcelPrimBonus(F32 bonus) 	{ mParcelPrimBonus = bonus; }

	void	setCleanOtherTime(S32 time)					{ mCleanOtherTime = time; }
	void	setRegionPushOverride(bool override) {mRegionPushOverride = override; }
	void	setRegionDenyAnonymousOverride(bool override)	{ mRegionDenyAnonymousOverride = override; }
	void	setRegionDenyAgeUnverifiedOverride(bool override)	{ mRegionDenyAgeUnverifiedOverride = override; }
    void    setRegionAllowAccessOverride(bool override) { mRegionAllowAccessoverride = override; }
    void    setRegionAllowEnvironmentOverride(bool override) { mRegionAllowEnvironmentOverride = override; }

    void    setParcelEnvironmentVersion(S32 version) { mCurrentEnvironmentVersion = version; }

	// Accessors for parcel sellWithObjects
	void	setPreviousOwnerID(LLUUID prev_owner)	{ mPreviousOwnerID = prev_owner; }
	void	setPreviouslyGroupOwned(bool b)			{ mPreviouslyGroupOwned = b; }
	void	setSellWithObjects(bool b)				{ setParcelFlag(PF_SELL_PARCEL_OBJECTS, b); }

	LLUUID	getPreviousOwnerID() const		{ return mPreviousOwnerID; }
	bool	getPreviouslyGroupOwned() const	{ return mPreviouslyGroupOwned; }
	bool	getSellWithObjects() const		{ return (mParcelFlags & PF_SELL_PARCEL_OBJECTS) ? true : false; }

protected:
	LLUUID mID;
	LLUUID				mOwnerID;
	LLUUID				mGroupID;
	bool				mGroupOwned; // true if mOwnerID is a group_id
	LLUUID				mPreviousOwnerID;
	bool				mPreviouslyGroupOwned;

	EOwnershipStatus mStatus;
	ECategory mCategory;
	LLUUID mAuthBuyerID;
	LLUUID mSnapshotID;
	LLVector3 mUserLocation;
	LLVector3 mUserLookAt;
	ELandingType mLandingType;
	bool mSeeAVs;							// Avatars on this parcel are visible from outside it
	bool mHaveNewParcelLimitData;			// Remove once hidden AV feature is grid-wide
	LLTimer mSaleTimerExpires;
	LLTimer mMediaResetTimer;

	S32 mGraceExtension;

	// This value is non-zero if there is an auction associated with
	// the parcel.
	U32 mAuctionID;

	// value used to temporarily lock attempts to purchase the parcel.
	bool mInEscrow;

	time_t				mClaimDate;				// UTC Unix-format time
	S32					mClaimPricePerMeter;	// meter squared
	S32					mRentPricePerMeter;		// meter squared
	S32					mArea;					// meter squared
	F32					mDiscountRate;			// 0.0-1.0
	F32					mDrawDistance;
	U32					mParcelFlags;
	S32					mSalePrice;				// linden dollars
	std::string			mName;
	std::string			mDesc;
	std::string			mMusicURL;
	std::string			mMediaURL;
	std::string			mMediaDesc;
	std::string 		mMediaType;
	S32					mMediaWidth;
	S32					mMediaHeight;
	U8					mMediaAutoScale;
	U8                  mMediaLoop;
	std::string         mMediaCurrentURL;
	LLUUID				mMediaID;
	U8                  mMediaAllowNavigate;
	U8					mMediaPreventCameraZoom;
	F32					mMediaURLTimeout;
	S32					mPassPrice;
	F32					mPassHours;
	LLVector3			mAABBMin;
	LLVector3			mAABBMax;
	S32					mMaxPrimCapacity;		// Prims allowed on parcel, does not include prim bonus
	S32					mSimWidePrimCount;
	S32					mSimWideMaxPrimCapacity;
	//S32					mSimWidePrimCorrection;
	S32					mOwnerPrimCount;
	S32					mGroupPrimCount;
	S32					mOtherPrimCount;
	S32					mSelectedPrimCount;
	S32					mTempPrimCount;
	F32					mParcelPrimBonus;
	S32					mCleanOtherTime;
	bool				mRegionPushOverride;
	bool				mRegionDenyAnonymousOverride;
	bool				mRegionDenyAgeUnverifiedOverride;
    bool                mRegionAllowAccessoverride;
    bool                mRegionAllowEnvironmentOverride;
	bool				mAllowGroupAVSounds;
	bool				mAllowAnyAVSounds;
    bool                mObscureMOAP;
    S32                 mCurrentEnvironmentVersion;
	
    bool                mIsDefaultDayCycle;

public:
	// HACK, make private
	S32					mLocalID;
	LLUUID			    mBanListTransactionID;
	LLUUID			    mAccessListTransactionID;
	std::map<LLUUID,LLAccessEntry>	mAccessList;
	std::map<LLUUID,LLAccessEntry>	mBanList;
	std::map<LLUUID,LLAccessEntry>	mTempBanList;
	std::map<LLUUID,LLAccessEntry>	mTempAccessList;

	typedef std::map<LLUUID, U32> xp_type_map_t;

	void setExperienceKeyType(const LLUUID& experience_key, U32 type);
	U32 countExperienceKeyType(U32 type);
	LLAccessEntry::map getExperienceKeysByType(U32 type)const;
	void clearExperienceKeysByType(U32 type);

private:
	xp_type_map_t mExperienceKeys;
};


const std::string& ownership_status_to_string(LLParcel::EOwnershipStatus status);
LLParcel::EOwnershipStatus ownership_string_to_status(const std::string& s);
LLParcel::ECategory category_string_to_category(const std::string& s);
const std::string& category_to_string(LLParcel::ECategory category);


#endif
