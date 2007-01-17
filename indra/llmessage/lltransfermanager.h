/** 
 * @file lltransfermanager.h
 * @brief Improved transfer mechanism for moving data through the
 * message system.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSFERMANAGER_H
#define LL_LLTRANSFERMANAGER_H

#include <map>
#include <list>

#include "llhost.h"
#include "lluuid.h"
#include "llthrottle.h"
#include "llpriqueuemap.h"
#include "llassettype.h"

//
// Definition of the manager class for the new LLXfer replacement.
// Provides prioritized, bandwidth-throttled transport of arbitrary
// binary data between host/circuit combos
//


typedef enum e_transfer_channel_type
{
	LLTCT_UNKNOWN = 0,
	LLTCT_MISC,
	LLTCT_ASSET,
	LLTCT_NUM_TYPES
} LLTransferChannelType;


typedef enum e_transfer_source_type
{
	LLTST_UNKNOWN = 0,
	LLTST_FILE,
	LLTST_ASSET,
	LLTST_SIM_INV_ITEM,	// Simulator specific, may not be handled
	LLTST_SIM_ESTATE,	// Simulator specific, may not be handled
	LLTST_NUM_TYPES
} LLTransferSourceType;


typedef enum e_transfer_target_type
{
	LLTTT_UNKNOWN = 0,
	LLTTT_FILE,
	LLTTT_VFILE,
	LLTTT_NUM_TYPES
} LLTransferTargetType;


// Errors are negative, expected values are positive.
typedef enum e_status_codes
{
	LLTS_OK = 0,
	LLTS_DONE = 1,
	LLTS_SKIP = 2,
	LLTS_ABORT = 3,
	LLTS_ERROR = -1,
	LLTS_UNKNOWN_SOURCE = -2, // Equivalent of a 404
	LLTS_INSUFFICIENT_PERMISSIONS = -3	// Not enough permissions
} LLTSCode;

// Types of requests for estate wide information
typedef enum e_estate_type
{
	ET_Covenant = 0,
	ET_NONE = -1
} EstateAssetType;

class LLMessageSystem;
class LLDataPacker;

class LLTransferConnection;
class LLTransferSourceChannel;
class LLTransferTargetChannel;
class LLTransferSourceParams;
class LLTransferTargetParams;
class LLTransferSource;
class LLTransferTarget;

class LLTransferManager
{
public:
	LLTransferManager();
	virtual ~LLTransferManager();

	void init();
	void cleanup();

	void updateTransfers();	// Called per frame to push packets out on the various different channels.
	void cleanupConnection(const LLHost &host);


	LLTransferSourceChannel *getSourceChannel(const LLHost &host, const LLTransferChannelType stype);
	LLTransferTargetChannel *getTargetChannel(const LLHost &host, const LLTransferChannelType stype);

	LLTransferSource *findTransferSource(const LLUUID &transfer_id);

	BOOL						isValid() const			{ return mValid; }

	static void processTransferRequest(LLMessageSystem *mesgsys, void **);
	static void processTransferInfo(LLMessageSystem *mesgsys, void **);
	static void processTransferPacket(LLMessageSystem *mesgsys, void **);
	static void processTransferAbort(LLMessageSystem *mesgsys, void **);
	static void processTransferPriority(LLMessageSystem *mesgsys, void **);

	static void reliablePacketCallback(void **, S32 result);

	S32	getTransferBitsIn(const LLTransferChannelType tctype) const		{ return mTransferBitsIn[tctype]; }
	S32 getTransferBitsOut(const LLTransferChannelType tctype) const	{ return mTransferBitsOut[tctype]; }
	void resetTransferBitsIn(const LLTransferChannelType tctype)		{ mTransferBitsIn[tctype] = 0; }
	void resetTransferBitsOut(const LLTransferChannelType tctype)		{ mTransferBitsOut[tctype] = 0; }
	void addTransferBitsIn(const LLTransferChannelType tctype, const S32 bits)	{ mTransferBitsIn[tctype] += bits; }
	void addTransferBitsOut(const LLTransferChannelType tctype, const S32 bits)	{ mTransferBitsOut[tctype] += bits; }
protected:
	LLTransferConnection		*getTransferConnection(const LLHost &host);
	BOOL						removeTransferConnection(const LLHost &host);

protected:
	// Convenient typedefs
	typedef std::map<LLHost, LLTransferConnection *> host_tc_map;

	BOOL	mValid;
	LLHost	mHost;

	S32		mTransferBitsIn[LLTTT_NUM_TYPES];
	S32		mTransferBitsOut[LLTTT_NUM_TYPES];

	// We keep a map between each host and LLTransferConnection.
	host_tc_map mTransferConnections;
};


//
// Keeps tracks of all channels to/from a particular host.
//
class LLTransferConnection
{
public:
	LLTransferConnection(const LLHost &host);
	virtual ~LLTransferConnection();

	void updateTransfers();

	LLTransferSourceChannel *getSourceChannel(const LLTransferChannelType type);
	LLTransferTargetChannel *getTargetChannel(const LLTransferChannelType type);

	// Convenient typedefs
	typedef std::list<LLTransferSourceChannel *>::iterator tsc_iter;
	typedef std::list<LLTransferTargetChannel *>::iterator ttc_iter;
	friend class LLTransferManager;
protected:

	LLHost									mHost;
	std::list<LLTransferSourceChannel *>	mTransferSourceChannels;
	std::list<LLTransferTargetChannel *>	mTransferTargetChannels;

};


//
// A channel which is pushing data out.
//

class LLTransferSourceChannel
{
public:
	LLTransferSourceChannel(const LLTransferChannelType channel_type,
							const LLHost &host);
	virtual ~LLTransferSourceChannel();

	void updateTransfers();

	void updatePriority(LLTransferSource *tsp, const F32 priority);

	void				addTransferSource(LLTransferSource *sourcep);
	LLTransferSource	*findTransferSource(const LLUUID &transfer_id);
	BOOL				deleteTransfer(LLTransferSource *tsp);

	void					setThrottleID(const S32 throttle_id)	{ mThrottleID = throttle_id; }

	LLTransferChannelType	getChannelType() const		{ return mChannelType; }
	LLHost					getHost() const				{ return mHost; }

protected:
	typedef std::list<LLTransferSource *>::iterator ts_iter;

	LLTransferChannelType				mChannelType;
	LLHost								mHost;
	LLPriQueueMap<LLTransferSource*>	mTransferSources;

	// The throttle that this source channel should use
	S32									mThrottleID;
};


//
// A channel receiving data from a source.
//
class LLTransferTargetChannel
{
public:
	LLTransferTargetChannel(const LLTransferChannelType channel_type, const LLHost &host);
	virtual ~LLTransferTargetChannel();

	void requestTransfer(const LLTransferSourceParams &source_params,
						 const LLTransferTargetParams &target_params,
						 const F32 priority);

	LLTransferTarget		*findTransferTarget(const LLUUID &transfer_id);
	BOOL					deleteTransfer(LLTransferTarget *ttp);


	LLTransferChannelType	getChannelType() const		{ return mChannelType; }
	LLHost					getHost() const				{ return mHost; }

protected:
	void sendTransferRequest(LLTransferTarget *targetp,
							 const LLTransferSourceParams &params,
							 const F32 priority);

	void					addTransferTarget(LLTransferTarget *targetp);

	friend class LLTransferTarget;
	friend class LLTransferManager;
protected:
	typedef std::list<LLTransferTarget *>::iterator tt_iter;

	LLTransferChannelType			mChannelType;
	LLHost							mHost;
	std::list<LLTransferTarget *>	mTransferTargets;
};


class LLTransferSourceParams
{
public:
	LLTransferSourceParams(const LLTransferSourceType type) : mType(type) { }
	virtual ~LLTransferSourceParams();

	virtual void packParams(LLDataPacker &dp) const	= 0;
	virtual BOOL unpackParams(LLDataPacker &dp) = 0;

	LLTransferSourceType getType() const			{ return mType; }
	
protected:
	LLTransferSourceType mType;
};


//
// LLTransferSource is an interface, all transfer sources should be derived from it.
//
typedef LLTransferSource *(*LLTransferSourceCreateFunc)(const LLUUID &id, const F32 priority);

class LLTransferSource
{
public:

	LLUUID getID()				{ return mID; }

	friend class LLTransferManager;
	friend class LLTransferSourceChannel;

protected:
	LLTransferSource(const LLTransferSourceType source_type,
					 const LLUUID &request_id,
					 const F32 priority);
	virtual ~LLTransferSource();

	void					sendTransferStatus(LLTSCode status);	// When you've figured out your transfer status, do this

	virtual void			initTransfer() = 0;
	virtual F32				updatePriority() = 0;
	virtual LLTSCode		dataCallback(const S32 packet_id,
										 const S32 max_bytes,
										 U8 **datap,
										 S32 &returned_bytes,
										 BOOL &delete_returned) = 0;

	// The completionCallback is GUARANTEED to be called before the destructor.
	virtual void			completionCallback(const LLTSCode status) = 0;

	virtual void packParams(LLDataPacker& dp) const = 0;
	virtual BOOL unpackParams(LLDataPacker& dp) = 0;

	virtual S32				getNextPacketID()						{ return mLastPacketID + 1; }
	virtual void			setLastPacketID(const S32 packet_id)	{ mLastPacketID = packet_id; }


	// For now, no self-induced priority changes
	F32						getPriority()							{ return mPriority; }
	void					setPriority(const F32 pri)				{ mPriority = pri; }

	virtual void			abortTransfer(); // DON'T USE THIS ONE, used internally by LLTransferManager

	static LLTransferSource *createSource(const LLTransferSourceType stype,
										  const LLUUID &request_id,
										  const F32 priority);
	static void registerSourceType(const LLTransferSourceType stype, LLTransferSourceCreateFunc);

	static void sSetPriority(LLTransferSource *&tsp, const F32 priority);
	static F32	sGetPriority(LLTransferSource *&tsp);
protected:
	typedef std::map<LLTransferSourceType, LLTransferSourceCreateFunc> stype_scfunc_map;
	static stype_scfunc_map sSourceCreateMap;

	LLTransferSourceType mType;
	LLUUID mID;
	LLTransferSourceChannel *mChannelp;
	F32		mPriority;
	S32		mSize;
	S32		mLastPacketID;
};


class LLTransferTargetParams
{
public:
	LLTransferTargetParams(const LLTransferTargetType type) : mType(type) {}
	LLTransferTargetType getType() const		{ return mType; }
protected:
	LLTransferTargetType mType;
};


class LLTransferPacket
{
	// Used for storing a packet that's being delivered later because it's out of order.
	// ONLY should be accessed by the following two classes, for now.
	friend class LLTransferTarget;
	friend class LLTransferManager;

protected:

	LLTransferPacket(const S32 packet_id, const LLTSCode status, const U8 *datap, const S32 size);
	virtual ~LLTransferPacket();

protected:
	S32			mPacketID;
	LLTSCode	mStatus;
	U8			*mDatap;
	S32			mSize;
};


class LLTransferTarget
{
public:
	LLTransferTarget(
		LLTransferTargetType target_type,
		const LLUUID& transfer_id,
		LLTransferSourceType source_type);
	virtual ~LLTransferTarget();

	// Accessors
	LLUUID					getID() const			{ return mID; }
	LLTransferTargetType	getType() const			{ return mType; }
	LLTransferTargetChannel *getChannel() const		{ return mChannelp; }
	LLTransferSourceType getSourceType() const { return mSourceType; }

	// Static functionality
	static LLTransferTarget* createTarget(
		LLTransferTargetType target_type,
		const LLUUID& request_id,
		LLTransferSourceType source_type);

	// friends
	friend class LLTransferManager;
	friend class LLTransferTargetChannel;

protected:
	// Implementation
	virtual bool unpackParams(LLDataPacker& dp) = 0;
	virtual void applyParams(const LLTransferTargetParams &params) = 0;
	virtual LLTSCode		dataCallback(const S32 packet_id, U8 *in_datap, const S32 in_size) = 0;

	// The completionCallback is GUARANTEED to be called before the destructor, so all handling
	// of errors/aborts should be done here.
	virtual void			completionCallback(const LLTSCode status) = 0;

	void					abortTransfer();

	virtual S32				getNextPacketID()						{ return mLastPacketID + 1; }
	virtual void			setLastPacketID(const S32 packet_id)	{ mLastPacketID = packet_id; }
	void					setSize(const S32 size)					{ mSize = size; }
	void					setGotInfo(const BOOL got_info)			{ mGotInfo = got_info; }
	BOOL					gotInfo() const							{ return mGotInfo; }

	bool addDelayedPacket(
		const S32 packet_id,
		const LLTSCode status,
		U8* datap,
		const S32 size);

protected:
	typedef std::map<S32, LLTransferPacket *> transfer_packet_map;
	typedef std::map<S32, LLTransferPacket *>::iterator tpm_iter;

	LLTransferTargetType	mType;
	LLTransferSourceType mSourceType;
	LLUUID					mID;
	LLTransferTargetChannel *mChannelp;
	BOOL					mGotInfo;
	S32						mSize;
	S32						mLastPacketID;

	transfer_packet_map		mDelayedPacketMap; // Packets that are waiting because of missing/out of order issues
};


// Hack, here so it's publicly available even though LLTransferSourceInvItem is only available on the simulator
class LLTransferSourceParamsInvItem: public LLTransferSourceParams
{
public:
	LLTransferSourceParamsInvItem();
	virtual ~LLTransferSourceParamsInvItem() {}
	/*virtual*/ void packParams(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpackParams(LLDataPacker &dp);

	void setAgentSession(const LLUUID &agent_id, const LLUUID &session_id);
	void setInvItem(const LLUUID &owner_id, const LLUUID &task_id, const LLUUID &item_id);
	void setAsset(const LLUUID &asset_id, const LLAssetType::EType at);

	LLUUID getAgentID() const						{ return mAgentID; }
	LLUUID getSessionID() const						{ return mSessionID; }
	LLUUID getOwnerID() const						{ return mOwnerID; }
	LLUUID getTaskID() const						{ return mTaskID; }
	LLUUID getItemID() const						{ return mItemID; }
	LLUUID getAssetID() const						{ return mAssetID; }
	LLAssetType::EType getAssetType() const			{ return mAssetType; }

protected:
	LLUUID				mAgentID;
	LLUUID				mSessionID;
	LLUUID				mOwnerID;
	LLUUID				mTaskID;
	LLUUID				mItemID;
	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;
};


// Hack, here so it's publicly available even though LLTransferSourceEstate is only available on the simulator
class LLTransferSourceParamsEstate: public LLTransferSourceParams
{
public:
	LLTransferSourceParamsEstate();
	virtual ~LLTransferSourceParamsEstate() {}
	/*virtual*/ void packParams(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpackParams(LLDataPacker &dp);

	void setAgentSession(const LLUUID &agent_id, const LLUUID &session_id);
	void setEstateAssetType(const EstateAssetType etype);
	void setAsset(const LLUUID &asset_id, const LLAssetType::EType at);

	LLUUID getAgentID() const						{ return mAgentID; }
	LLUUID getSessionID() const						{ return mSessionID; }
	EstateAssetType getEstateAssetType() const		{ return mEstateAssetType; }
	LLUUID getAssetID() const					{ return mAssetID; }
	LLAssetType::EType getAssetType() const		{ return mAssetType; }

protected:
	LLUUID				mAgentID;
	LLUUID				mSessionID;
	EstateAssetType		mEstateAssetType;
	// these are set on the sim based on estateinfotype
	LLUUID				mAssetID;
	LLAssetType::EType	mAssetType;
};


extern LLTransferManager gTransferManager;

#endif//LL_LLTRANSFERMANAGER_H
