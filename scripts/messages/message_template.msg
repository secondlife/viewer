// Linden Lab development message templates

version 2.0

// The Version 2.0 template requires preservation of message
// numbers. Each message must be numbered relative to the
// other messages of that type. The current highest number 
// for each type is listed below:
// Low: 423
// Medium: 18
// High: 29
// PLEASE UPDATE THIS WHEN YOU ADD A NEW MESSAGE!


// *************************************************************************
// Test Message
// *************************************************************************

// Test Message

{
	TestMessage Low 1 NotTrusted Zerocoded
	{
		TestBlock1		Single
		{	Test1		U32	}
	}
	{
		NeighborBlock		Multiple		4
		{	Test0		U32	}
		{	Test1		U32	}
		{	Test2		U32	}
	}
}

// *************************************************************************
// Messaging Internal Data Management Message
// *************************************************************************

// *************************
// List fixed messages first
// *************************


// Packet Ack - Ack a list of packets sent reliable
{
	PacketAck Fixed 0xFFFFFFFB NotTrusted Unencoded
	{
		Packets			Variable		
		{	ID			U32	}
	}
}


// OpenCircuit - Tells the recipient's messaging system to open the descibed circuit
{
	OpenCircuit Fixed 0xFFFFFFFC NotTrusted Unencoded UDPBlackListed
	{
		CircuitInfo		Single
		{	IP			IPADDR	}
		{	Port		IPPORT	}
	}
}


// CloseCircuit - Tells the recipient's messaging system to close the descibed circuit
{
	CloseCircuit Fixed 0xFFFFFFFD NotTrusted Unencoded
}


// ******************
// End fixed messages
// ******************

// StartPingCheck - used to measure circuit ping times
// PingID is used to determine how backlogged the ping was that was
// returned (or how hosed the other side is)
{
	StartPingCheck High 1 NotTrusted Unencoded
	{
		PingID Single
		{	PingID	U8	}
		{	OldestUnacked	U32	}	// Current oldest "unacked" packet on the sender side
	}
}

// CompletePingCheck - used to measure circuit ping times

{
	CompletePingCheck High 2 NotTrusted Unencoded
	{
		PingID Single
		{	PingID	U8	}
	}
}

// space->sim
// sim->sim
// AddCircuitCode - Tells the recipient's messaging system that this code
// is for a legal circuit
{
	AddCircuitCode Low 2 Trusted Unencoded
	{
		CircuitCode		Single
		{	Code		U32		}
		{	SessionID	LLUUID	}
		{	AgentID		LLUUID	} // WARNING - may be null in valid message
	}
}

// viewer->sim
// UseCircuitCode - Attempts to provide the recipient with IP and Port
// info. In the case of viewers, the id is the session id. For other
// machines it may be null. The session id will always be the session
// id of the process, which every server will generate on startup and
// the viewer will be handed after login.
{
	UseCircuitCode Low 3 NotTrusted Unencoded
	{
		CircuitCode		Single
		{	Code		U32		}
		{	SessionID	LLUUID	}
		{	ID			LLUUID	} // agent id
	}
}


// *************************************************************************
// SpaceServer to Simulator Messages
// ************************************************************************

// Neighbor List - Passed anytime neighbors change
{
	NeighborList High 3 Trusted Unencoded
	{
		NeighborBlock	Multiple		4
		{	IP			IPADDR	}
		{	Port		IPPORT	}
		{	PublicIP    IPADDR	}
		{	PublicPort  IPPORT	}
		{	RegionID	LLUUID	}
		{	Name		Variable		1	}	// string
		{	SimAccess	U8	}
	}
}


// AvatarTextureUpdate
// simulator -> dataserver
// reliable
{
	AvatarTextureUpdate Low 4 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	TexturesChanged	BOOL	}
	}
	{
		WearableData		Variable
		{	CacheID			LLUUID }
		{	TextureIndex	U8 }
		{	HostName		Variable	1	}
	}
	{
		TextureData		Variable
		{	TextureID		LLUUID	}
	}
}


// SimulatorMapUpdate
// simulator -> dataserver
// reliable
{
	SimulatorMapUpdate Low 5 Trusted Unencoded
	{
		MapData		Single
		{	Flags		U32		}
	}
}

// SimulatorSetMap
// simulator -> dataserver
// reliable
// Used to upload a map image into the database (currently used only for Land For Sale)
{
	SimulatorSetMap Low 6 Trusted Unencoded
	{
		MapData		Single
		{	RegionHandle	U64		}
		{	Type			S32		}
		{	MapImage		LLUUID	}
	}
}

// SubscribeLoad
// spaceserver -> simulator
// reliable
{
	SubscribeLoad	Low	7 Trusted Unencoded
}

// UnsubscribeLoad
// spaceserver -> simulator
// reliable
{
	UnsubscribeLoad Low 8 Trusted Unencoded
}


// ************************************************************************
// Simulator to SpaceServer Messages
// ************************************************************************

// SimulatorReady - indicates the sim has finished loading its state
// and is ready to receive updates from others
{
	SimulatorReady Low 9 Trusted Zerocoded
	{
		SimulatorBlock		Single
		{	SimName			Variable	1	}
		{	SimAccess		U8	}
		{	RegionFlags		U32	}
		{	RegionID		LLUUID	}
		{	EstateID		U32	}
		{	ParentEstateID	U32	}
	}
	{
		TelehubBlock		Single
		{	HasTelehub		BOOL		}
		{	TelehubPos		LLVector3	}
	}
}

// TelehubInfo - fill in the UI for telehub creation floater.
// sim -> viewer
// reliable
{
	TelehubInfo	Low	10 Trusted Unencoded
	{
		TelehubBlock		Single
		{	ObjectID		LLUUID			}	// null if no telehub
		{	ObjectName		Variable 1		}	// string
		{	TelehubPos		LLVector3		}	// fallback if viewer can't find object
		{	TelehubRot		LLQuaternion	}
	}
	{
		SpawnPointBlock		Variable
		{	SpawnPointPos	LLVector3		}	// relative to telehub position
	}
}

// SimulatorPresentAtLocation - indicates that the sim is present at a grid
// location and passes what it believes its neighbors are
{
	SimulatorPresentAtLocation Low 11 Trusted Unencoded
	{
		SimulatorPublicHostBlock	Single
		{	Port        IPPORT }
		{	SimulatorIP IPADDR }
		{   GridX       U32	   }
		{	GridY       U32    }
	}
	{
		NeighborBlock	Multiple		4
		{	IP			IPADDR	}
		{	Port		IPPORT	}
	}
	{
		SimulatorBlock		Single
		{	SimName			Variable	1	}
		{	SimAccess		U8	}
		{	RegionFlags		U32	}
		{	RegionID		LLUUID	}
		{	EstateID		U32	}
		{	ParentEstateID	U32	}
	}
	{
		TelehubBlock		Variable
		{	HasTelehub		BOOL		}
		{	TelehubPos		LLVector3	}
	}
}

// SimulatorLoad
// simulator -> spaceserver
// reliable
{
	SimulatorLoad Low 12 Trusted Unencoded
	{
		SimulatorLoad		Single
		{	TimeDilation 	F32 }
		{	AgentCount 		S32 }
		{	CanAcceptAgents	BOOL	}
	}
	{
		AgentList	Variable
		{	CircuitCode		U32	}
		{	X				U8	}
		{	Y				U8	}
	}
}

// Simulator Shutdown Request - Tells spaceserver that a simulator is trying to shutdown
{
	SimulatorShutdownRequest Low 13 Trusted Unencoded
}

// ****************************************************************************
// Presense messages
// ****************************************************************************

// sim -> dataserver
{
	RegionPresenceRequestByRegionID Low 14 Trusted Unencoded
	{
		RegionData		Variable
		{	RegionID	LLUUID	}
	}
}

// sim -> dataserver
{
	RegionPresenceRequestByHandle Low 15 Trusted Unencoded
	{
		RegionData		Variable
		{	RegionHandle	U64	}
	}
}

// dataserver -> sim
{
	RegionPresenceResponse Low 16 Trusted Zerocoded
	{
		RegionData		Variable
		{	RegionID			LLUUID	}
		{	RegionHandle		U64	}
		{	InternalRegionIP	IPADDR	}
		{	ExternalRegionIP	IPADDR	}
		{	RegionPort			IPPORT	}
		{	ValidUntil			F64	}
		{	Message				Variable	1	}
	}
}
 

// ****************************************************************************
// Simulator to dataserver messages
// ****************************************************************************

// Updates SimName, EstateID and SimAccess using RegionID as a key
{
	UpdateSimulator Low 17 Trusted Unencoded
	{
		SimulatorInfo	Single
		{	RegionID	LLUUID	}
		{	SimName		Variable	1	}
		{	EstateID	U32		}
		{	SimAccess	U8		}
	}
}


// record dwell time.
{
	LogDwellTime	Low	18 Trusted Unencoded
	{
		DwellInfo	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Duration	F32		}
		{	SimName		Variable	1 }
		{	RegionX		U32		}
		{	RegionY		U32		}
		{	AvgAgentsInView U8  }
		{	AvgViewerFPS U8		}
	}
}

// Disabled feature response message
{
	FeatureDisabled	Low 19 Trusted Unencoded
	{
		FailureInfo			Single
		{	ErrorMessage	Variable	1	}
		{	AgentID			LLUUID			}
		{	TransactionID	LLUUID			}
	}
}


// record lost money transactions.  This message could be generated
// from either the simulator or the dataserver, depending on how
// the transaction failed.
{
	LogFailedMoneyTransaction	Low 20 Trusted Unencoded
	{
		TransactionData	Single
		{	TransactionID	LLUUID	}
		{ 	TransactionTime	U32		} // utc seconds since epoch
		{	TransactionType	S32		}	// see lltransactiontypes.h
		{	SourceID		LLUUID  }
		{	DestID			LLUUID	}	// destination of the transfer
		{	Flags			U8		}
		{	Amount			S32		}
		{	SimulatorIP		IPADDR	}	// U32 encoded IP
		{	GridX			U32		}
		{	GridY			U32		}
		{	FailureType		U8		}
	}
}

// complaint/bug-report - sim -> dataserver. see UserReport for details.
// reliable
{
	UserReportInternal Low 21 Trusted Zerocoded
	{
		ReportData	Single
		{   ReportType		U8   }
		{   Category		U8   }
		{   ReporterID		LLUUID  }
		{	ViewerPosition	LLVector3	}
		{	AgentPosition	LLVector3	}
		{   ScreenshotID	LLUUID  }
		{   ObjectID		LLUUID  }
		{   OwnerID			LLUUID  }
		{   LastOwnerID		LLUUID  }
		{   CreatorID		LLUUID  }
		{	RegionID		LLUUID	}
		{   AbuserID		LLUUID  }
		{	AbuseRegionName	Variable	1	}
		{	AbuseRegionID	LLUUID	}
		{   Summary			Variable	1   }
		{   Details 		Variable	2   }
		{	VersionString	Variable	1	}
	}
}

// SetSimStatusInDatabase
// alters the "simulator" table in the database
// sim -> dataserver
// reliable
{
	SetSimStatusInDatabase Low 22 Trusted Unencoded
	{
		Data	Single
		{	RegionID	LLUUID	}
		{   HostName 	Variable 1	}
		{	X			S32	}
		{	Y			S32	}
		{	PID			S32	}
		{	AgentCount	S32	}
		{	TimeToLive  S32 } // in seconds
		{	Status		Variable	1	}
	}
}

// SetSimPresenceInDatabase
// updates the "presence" table in the database to ensure
// that a given simulator is present and valid for a set amount of
// time
{
	SetSimPresenceInDatabase Low 23 Trusted Unencoded
	{
		SimData Single
		{   RegionID 	LLUUID	}
		{   HostName 	Variable 1	}
		{	GridX		U32	}
		{	GridY		U32	}
		{	PID			S32	}
		{	AgentCount	S32	}
		{	TimeToLive  S32 } // in seconds
		{	Status		Variable	1	}
	}
}

// ***************************************************************************
// Economy messages
// ***************************************************************************

// once we use local stats, this will include a region handle
{
	EconomyDataRequest Low 24 NotTrusted Unencoded
}

// dataserver to sim, response w/ econ data
{
	EconomyData Low 25 Trusted Zerocoded
	{
		Info					Single
		{	ObjectCapacity			S32	}
		{	ObjectCount				S32	}
		{	PriceEnergyUnit			S32	}
		{	PriceObjectClaim		S32	}
		{	PricePublicObjectDecay	S32	}
		{	PricePublicObjectDelete	S32	}
		{	PriceParcelClaim		S32	}
		{	PriceParcelClaimFactor	F32	}
		{	PriceUpload				S32	}
		{	PriceRentLight			S32	}
		{	TeleportMinPrice		S32	}
		{	TeleportPriceExponent 	F32	}
		{	EnergyEfficiency		F32	}
		{	PriceObjectRent			F32	}
		{	PriceObjectScaleFactor 	F32	}
		{	PriceParcelRent			S32	}
		{	PriceGroupCreate		S32	}
	}
}

// ***************************************************************************
// Search messages
// ***************************************************************************

// AvatarPickerRequest
// Get a list of names to select a person
// viewer -> sim -> data
// reliable
{
	AvatarPickerRequest Low 26 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	QueryID		LLUUID		}
	}
	{
		Data			Single
		{	Name		Variable 1	}
	}
}

// backend implementation which tracks if the user is a god.
{
	AvatarPickerRequestBackend Low 27 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	QueryID		LLUUID		}
		{	GodLevel	U8	}
	}
	{
		Data			Single
		{	Name		Variable 1	}
	}
}

// AvatarPickerReply
// List of names to select a person
// reliable
{
	AvatarPickerReply Low 28 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	QueryID		LLUUID		}
	}
	{
		Data			Variable
		{	AvatarID	LLUUID		}
		{	FirstName	Variable 1	}
		{	LastName	Variable 1	}
	}
}

// PlacesQuery
// Used for getting a list of places for the group land panel
// and the user land holdings panel.  NOT for the directory.
{
	PlacesQuery Low 29 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	QueryID		LLUUID		}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
	}
	{
		QueryData		Single
		{	QueryText	Variable	1	}
		{	QueryFlags	U32				}
		{	Category	S8				}
		{	SimName		Variable	1	}
	}
}

// PlacesReply
// dataserver -> simulator -> viewer
// If the user has specified a location, use that to compute
// global x,y,z.  Otherwise, use center of the AABB.
// reliable
{
	PlacesReply Low 30 Trusted Zerocoded UDPDeprecated
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	QueryID		LLUUID		}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
	}
	{
		QueryData		Variable
		{	OwnerID			LLUUID			}
		{	Name			Variable	1	}
		{	Desc			Variable	1	}
		{	ActualArea		S32				}
		{	BillableArea	S32				}
		{	Flags			U8				}
		{	GlobalX			F32				}	// meters
		{	GlobalY			F32				}	// meters
		{	GlobalZ			F32				}	// meters
		{	SimName			Variable	1	}
		{	SnapshotID		LLUUID			}
		{	Dwell			F32				}
		{	Price			S32				}
		//{	ProductSKU		Variable	1	}
	}
}

// DirFindQuery viewer->sim
// Message to start asking questions for the directory 
{
	DirFindQuery Low 31 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
		{	QueryText	Variable 1	}
		{	QueryFlags	U32			}
		{	QueryStart	S32			}	// prev/next page support
	}
}

// DirFindQueryBackend sim->data
// Trusted message generated by receipt of DirFindQuery to sim.
{
	DirFindQueryBackend Low 32 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
		{	QueryText	Variable 1	}
		{	QueryFlags	U32			}
		{	QueryStart	S32			}	// prev/next page support
		{	EstateID	U32			}
		{	Godlike		BOOL		}
	}
}


// DirPlacesQuery viewer->sim
// Used for the Find directory of places
{
	DirPlacesQuery Low 33 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
		{	QueryText	Variable	1	}
		{	QueryFlags	U32				}
		{	Category	S8				}
		{	SimName		Variable	1	}
		{	QueryStart	S32			}
	}
}

// DirPlacesQueryBackend sim->dataserver
// Used for the Find directory of places.
{
	DirPlacesQueryBackend Low 34 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
		{	QueryText	Variable	1	}
		{	QueryFlags	U32				}
		{	Category	S8				}
		{	SimName		Variable	1	}
		{	EstateID	U32				}
		{	Godlike		BOOL			}
		{	QueryStart	S32			}
	}
}

// DirPlacesReply dataserver->sim->viewer
// If the user has specified a location, use that to compute
// global x,y,z.  Otherwise, use center of the AABB.
// reliable
{
	DirPlacesReply Low 35 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
	}
	{
		QueryData		Variable
		{	QueryID		LLUUID		}
	}
	{
		QueryReplies            Variable
		{	ParcelID		LLUUID		}
		{	Name			Variable 1	}
		{	ForSale			BOOL		}
		{	Auction			BOOL		}
		{	Dwell			F32			}
	}
	{
		StatusData            Variable
		{	Status		U32		}
	}
}

// DirPeopleReply
{
	DirPeopleReply Low 36 Trusted Zerocoded
	{
		AgentData				Single
		{	AgentID				LLUUID	}
	}
	{
		QueryData		        Single
		{	QueryID		        LLUUID	}
	}
	{
		QueryReplies            Variable
 		{	AgentID	  	 	    LLUUID			}
		{	FirstName	        Variable	1	}
		{	LastName	        Variable	1	}
		{	Group		        Variable	1	}
		{	Online  	        BOOL			}
		{	Reputation	        S32				}
	}
}

// DirEventsReply
{
	DirEventsReply Low 37 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
	}
	{
		QueryReplies	Variable
		{	OwnerID		LLUUID			}
		{	Name		Variable	1	}
		{	EventID		U32				}
		{	Date		Variable	1	}
		{	UnixTime	U32				}
		{	EventFlags	U32				}
	}
	{
		StatusData Variable
		{	Status		U32			}
	}
}

// DirGroupsReply
// dataserver -> userserver -> viewer
// reliable
{
	DirGroupsReply Low 38 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID			}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID			}
	}
	{
		QueryReplies	Variable
		{	GroupID			LLUUID			}
		{	GroupName		Variable	1	}	// string
		{	Members			S32				}
		{	SearchOrder		F32				}
	}
}


// DirClassifiedQuery viewer->sim
// reliable
{
	DirClassifiedQuery Low 39 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID			}
		{	QueryText		Variable	1	}
		{	QueryFlags		U32				}
		{	Category		U32				}
		{	QueryStart	S32					}
	}
}

// DirClassifiedQueryBackend sim->dataserver
// reliable
{
	DirClassifiedQueryBackend Low 40 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID			}
		{	QueryText		Variable	1	}
		{	QueryFlags		U32				}
		{	Category		U32				}
		{	EstateID		U32				}
		{	Godlike			BOOL			}
		{	QueryStart	S32					}
	}
}

// DirClassifiedReply dataserver->sim->viewer
// reliable
{
	DirClassifiedReply Low 41 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
	}
	{
		QueryReplies	Variable
		{	ClassifiedID	LLUUID		}
		{	Name			Variable 1	}
		{	ClassifiedFlags	U8			}
		{	CreationDate	U32			}
		{	ExpirationDate	U32			}
		{	PriceForListing	S32			}
	}
	{
		StatusData Variable
		{	Status			U32		}
	}
}


// AvatarClassifiedReply
// dataserver -> simulator -> viewer
// Send the header information for this avatar's classifieds
// This fills in the tabs of the Classifieds panel.
// reliable
{
	AvatarClassifiedReply Low 42 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
		{	TargetID		LLUUID		}
	}
	{
		Data			Variable
		{	ClassifiedID	LLUUID		}
		{	Name			Variable 1	}
	}
}


// ClassifiedInfoRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	ClassifiedInfoRequest Low 43 NotTrusted Zerocoded
	{
		AgentData        Single
		{    AgentID        LLUUID		}
		{    SessionID      LLUUID		}
	}
	{
		Data        Single
		{	ClassifiedID	LLUUID		}
	}
}


// ClassifiedInfoReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	ClassifiedInfoReply Low 44 Trusted Unencoded
    {
        AgentData    Single
        {	AgentID			LLUUID		}
    }
    {
		Data        Single
		{	ClassifiedID	LLUUID		}
		{	CreatorID		LLUUID		}
		{	CreationDate	U32			}
		{	ExpirationDate	U32			}
		{	Category		U32			}
		{	Name			Variable 1	}
        {	Desc			Variable 2	}
		{	ParcelID		LLUUID		}
		{	ParentEstate	U32			}
        {	SnapshotID		LLUUID		}
        {	SimName			Variable 1	}
        {	PosGlobal		LLVector3d	}
        {	ParcelName		Variable 1	}
		{	ClassifiedFlags	U8			}
		{	PriceForListing	S32			}
    }
}


// ClassifiedInfoUpdate
// Update a classified.  ParcelID and EstateID are set
// on the simulator as the message passes through.
// viewer -> simulator -> dataserver
// reliable
{
	ClassifiedInfoUpdate Low 45 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data		Single
		{	ClassifiedID	LLUUID		}
		{	Category		U32			}
		{	Name			Variable 1	}
		{	Desc			Variable 2	}
		{	ParcelID		LLUUID		}
		{	ParentEstate	U32			}
		{	SnapshotID		LLUUID		}
		{	PosGlobal		LLVector3d	}
		{	ClassifiedFlags	U8			}
		{	PriceForListing	S32			}
	}
}


// ClassifiedDelete
// Delete a classified from the database.
// viewer -> simulator -> dataserver
// reliable
{
	ClassifiedDelete Low 46 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data	Single
		{	ClassifiedID	LLUUID		}
	}
}

// ClassifiedGodDelete
// Delete a classified from the database.
// QueryID is needed so database can send a repeat list of 
// classified.
// viewer -> simulator -> dataserver
// reliable
{
	ClassifiedGodDelete Low 47 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data	Single
		{	ClassifiedID	LLUUID		}
		{	QueryID			LLUUID		}
	}
}


// DirLandQuery viewer->sim
// Special query for the land for sale/auction panel.
// reliable
{
	DirLandQuery Low 48 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID	}
		{	QueryFlags		U32		}
		{	SearchType		U32		}
		{	Price			S32		}
		{	Area			S32		}
		{	QueryStart		S32		}
	}
}

// DirLandQueryBackend sim->dataserver
// Special query for the land for sale/auction panel.
{
	DirLandQueryBackend Low 49 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID	}
		{	QueryFlags		U32		}
		{	SearchType		U32		}
		{	Price			S32		}
		{	Area			S32		}
		{	QueryStart		S32		}
		{	EstateID		U32		}
		{	Godlike			BOOL	}
	}
}

// DirLandReply
// dataserver -> simulator -> viewer
// reliable
{
	DirLandReply Low 50 Trusted Zerocoded UDPDeprecated
	{
		AgentData			Single
		{	AgentID			LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID		}
	}
	{
		QueryReplies		Variable
		{	ParcelID		LLUUID		}
		{	Name			Variable 1	}
		{	Auction			BOOL		}
		{	ForSale			BOOL		}
		{	SalePrice		S32			}
		{	ActualArea		S32			}
		//{	ProductSKU		Variable 1	}
	}
}

// DEPRECATED: DirPopularQuery viewer->sim
// Special query for the land for sale/auction panel.
// reliable
{
	DirPopularQuery Low 51 NotTrusted Zerocoded Deprecated
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID	}
		{	QueryFlags		U32		}
	}
}

// DEPRECATED: DirPopularQueryBackend sim->dataserver
// Special query for the land for sale/auction panel.
// reliable
{
	DirPopularQueryBackend Low 52 Trusted Zerocoded Deprecated
	{
		AgentData			Single
		{	AgentID			LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID	}
		{	QueryFlags		U32		}
		{	EstateID		U32		}
		{	Godlike			BOOL	}
	}
}

// DEPRECATED: DirPopularReply
// dataserver -> simulator -> viewer
// reliable
{
	DirPopularReply Low 53 Trusted Zerocoded Deprecated
	{
		AgentData			Single
		{	AgentID			LLUUID	}
	}
	{
		QueryData			Single
		{	QueryID			LLUUID		}
	}
	{
		QueryReplies		Variable
		{	ParcelID		LLUUID		}
		{	Name			Variable 1	}
		{	Dwell			F32			}
	}
}

// ParcelInfoRequest
// viewer -> simulator -> dataserver
// reliable
{
	ParcelInfoRequest Low 54 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID	}
	}
	{
		Data			Single
		{	ParcelID		LLUUID		}
	}
}

// ParcelInfoReply
// dataserver -> simulator -> viewer
// reliable
{
	ParcelInfoReply Low 55 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
	}
	{
		Data			Single
		{	ParcelID		LLUUID			}
		{	OwnerID			LLUUID			}
		{	Name			Variable	1	}
		{	Desc			Variable	1	}
		{	ActualArea		S32				}
		{	BillableArea	S32				}
		{	Flags			U8				}
		{	GlobalX			F32				}	// meters
		{	GlobalY			F32				}	// meters
		{	GlobalZ			F32				}	// meters
		{	SimName			Variable	1	}
		{	SnapshotID		LLUUID			}
		{	Dwell			F32				}
		{	SalePrice		S32				}
		{	AuctionID		S32				}
	}
}


// ParcelObjectOwnersRequest
// viewer -> simulator
// reliable
{
	ParcelObjectOwnersRequest Low 56 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData		Single
		{	LocalID			S32		}
	}
}


// ParcelObjectOwnersReply
// simulator -> viewer
// reliable
{
	ParcelObjectOwnersReply Low 57 Trusted Zerocoded UDPDeprecated
	{
		Data			Variable
		{	OwnerID			LLUUID			}
		{	IsGroupOwned	BOOL			}
		{	Count			S32				}
		{	OnlineStatus	BOOL			}
	}
}

// GroupNoticeListRequest
// viewer -> simulator -> dataserver
// reliable
{
	GroupNoticesListRequest	Low	58 NotTrusted	Unencoded
	{
		AgentData        Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data        Single
		{	GroupID			LLUUID		}
	}
}

// GroupNoticesListReply
// dataserver -> simulator -> viewer
// reliable
{
	GroupNoticesListReply	Low	59 Trusted	Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	GroupID			LLUUID		}
	}
	{
		Data		Variable
		{	NoticeID		LLUUID		}
		{	Timestamp		U32			}
		{	FromName		Variable 2	}
		{	Subject			Variable 2	}
		{	HasAttachment	BOOL		}
		{	AssetType		U8			}
	}
}

// GroupNoticeRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	GroupNoticeRequest Low 60 NotTrusted Unencoded
	{
		AgentData        Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data        Single
		{	GroupNoticeID	LLUUID		}
	}
}

// GroupNoticeAdd
// Add a group notice. 
// simulator -> dataserver
// reliable
{
	GroupNoticeAdd Low 61 Trusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
	}
	{
		MessageBlock		Single
		{	ToGroupID		LLUUID	}
		{	ID				LLUUID	}
		{	Dialog			U8	}
		{	FromAgentName	Variable	1	}
		{	Message			Variable	2	}
		{	BinaryBucket	Variable	2	}
	}
}


// ****************************************************************************
// Teleport messages
//
// The teleport messages are numerous, so I have attempted to give them a 
// consistent naming convention. Since there is a bit of glob pattern
// aliasing, the rules are applied in order.
//
// Teleport* - viewer->sim or sim->viewer message which announces a 
//             teleportation request, progrees, start, or end.
// Data* - sim->data or data->sim trusted message.
// Space* - sim->space or space->sim trusted messaging
// *Lure - A lure message to pass around information.
//
// All actual viewer teleports will begin with a Teleport* message and
// end in a TeleportStart, TeleportLocal or TeleportFailed message. The TeleportFailed
// message may be returned by any process and must be routed through the
// teleporting agent's simulator and back to the viewer.
// ****************************************************************************

// TeleportRequest
// viewer -> sim specifying exact teleport destination
{
	TeleportRequest	Low	62 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		Info		Single
		{	RegionID	LLUUID		}
		{	Position	LLVector3	}
		{	LookAt		LLVector3	}
	}
}

// TeleportLocationRequest
// viewer -> sim specifying exact teleport destination
{
	TeleportLocationRequest	Low	63 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		Info	Single
		{	RegionHandle	U64			}
		{	Position		LLVector3	}
		{	LookAt			LLVector3	}
	}
}

// TeleportLocal
// sim -> viewer reply telling the viewer that we've successfully TP'd
// to somewhere else within the sim
{
	TeleportLocal Low 64 Trusted Unencoded
	{
		Info		Single
		{	AgentID			LLUUID			}
		{	LocationID		U32				}
		{	Position		LLVector3		}	// region
		{	LookAt			LLVector3		}
		{	TeleportFlags 	U32				}
	}
}

// TeleportLandmarkRequest viewer->sim
// teleport to landmark asset ID destination. use LLUUD::null for home.
{
	TeleportLandmarkRequest Low 65 NotTrusted Zerocoded
	{
		Info		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	LandmarkID		LLUUID	}
	}
}

// TeleportProgress sim->viewer
// Tell the agent how the teleport is going.
{
	TeleportProgress Low 66 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		Info			Single
		{	TeleportFlags	U32			}
		{	Message		Variable	1	}  // string
	}
}

// DataHomeLocationRequest sim->data
// Request 
{
	DataHomeLocationRequest Low 67 Trusted Zerocoded
	{
		Info		Single
		{	AgentID			LLUUID	}
		{	KickedFromEstateID	U32	}
	}
	{
		AgentInfo	Single
		{   AgentEffectiveMaturity U32  }
	}
}

// DataHomeLocationReply data->sim
// response is the location of agent home.
{
	DataHomeLocationReply Low 68 Trusted Unencoded
	{
		Info		Single
		{	AgentID			LLUUID		}
		{	RegionHandle	U64			}
		{	Position		LLVector3	}	// region coords
		{	LookAt			LLVector3	}
	}
}


// TeleportFinish sim->viewer
// called when all of the information has been collected and readied for 
// the agent.
{
	TeleportFinish Low 69 Trusted Unencoded UDPBlackListed
	{
		Info		Single
		{	AgentID			LLUUID			}
		{	LocationID		U32				}
		{	SimIP			IPADDR			}
		{	SimPort			IPPORT			}
		{	RegionHandle	U64				}
		{	SeedCapability	Variable	2	}	// URL
		{	SimAccess		U8				}
		{	TeleportFlags 	U32				}
	}
}

// StartLure viewer->sim
// Sent from viewer to the local simulator to lure target id to near 
// agent id. This will generate an instant message that will be routed 
// through the space server and out to the userserver. When that IM 
// goes through the userserver and the TargetID is online, the 
// userserver will send an InitializeLure to the spaceserver. When that
// packet is acked, the original instant message is finally forwarded to 
// TargetID.
{
	StartLure	Low	70 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		Info	Single
		{	LureType 		U8	 			}
		{	Message			Variable	1	}
	}
	{
		TargetData Variable
		{	TargetID		LLUUID			}
	}
}

// TeleportLureRequest viewer->sim
// Message from target of lure to begin the teleport process on the 
// local simulator.
{
	TeleportLureRequest	Low	71 NotTrusted Unencoded
	{
		Info	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	LureID		LLUUID	}
		{	TeleportFlags 	U32				}
	}
}

// TeleportCancel viewer->sim
// reliable
{
	TeleportCancel	Low	72 NotTrusted Unencoded
	{
		Info	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}


// TeleportStart sim->viewer
// announce a successful teleport request to the viewer.
{
	TeleportStart Low 73 Trusted Unencoded
	{
		Info	Single
		{	TeleportFlags	U32		}
	}
}

// TeleportFailed somewhere->sim->viewer
// announce failure of teleport request
{
	TeleportFailed Low 74 Trusted Unencoded
	{
		Info		Single
		{	AgentID		LLUUID			}
		{	Reason		Variable	1	}  // string
	}
	{
		AlertInfo			Variable
		{	Message			Variable	1	}	// string id
		{	ExtraParams		Variable	1	}	// llsd extra parameters
	}
}


// ***************************************************************************
// Viewer to Simulator Messages
// ***************************************************************************

// Undo
{
	Undo Low 75 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectID	LLUUID	}
	}
}


// Redo
{
	Redo Low 76 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectID	LLUUID	}
	}
}

// UndoLand
{
	UndoLand Low 77 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}


// AgentPause - viewer occasionally will block, inform simulator of this fact
{
	AgentPause Low 78 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	SerialNum	U32		}	// U32, used by both pause and resume. Non-increasing numbers are ignored.
	}
}

// AgentResume - unblock the agent
{
	AgentResume Low 79 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	SerialNum	U32		}	// U32, used by both pause and resume. Non-increasing numbers are ignored.
	}
}


// AgentUpdate - Camera info sent from viewer to simulator
// or, more simply, two axes and compute cross product
// State data is temporary, indicates current behavior state:
//	0 = walking
//  1 = mouselook
//  2 = typing 
//  
// Center is region local (JNC 8.16.2001)
// Camera center is region local (JNC 8.29.2001)
{
	AgentUpdate High 4 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	BodyRotation	LLQuaternion	}
		{	HeadRotation	LLQuaternion	}
		{	State			U8	}
		{	CameraCenter	LLVector3	}
		{	CameraAtAxis	LLVector3	}
		{	CameraLeftAxis	LLVector3	}
		{	CameraUpAxis	LLVector3	}
		{	Far				F32	}
		{	ControlFlags	U32	}
		{	Flags			U8	}
	}
}

// ChatFromViewer
// Specifies the text to be said and the "type", 
// normal speech, shout, whisper.
// with the specified radius
{
	ChatFromViewer Low 80 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		ChatData			Single
		{	Message			Variable 2	}
		{	Type			U8			}
		{	Channel			S32			}
	}
}


// AgentThrottle
{
	AgentThrottle Low 81 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
		{	CircuitCode		U32		}
	}
	{
		Throttle			Single
		{	GenCounter		U32		}
		{	Throttles		Variable 1	}
	}
}


// AgentFOV - Update to agent's field of view, angle is vertical, single F32 float in radians
{
	AgentFOV Low 82 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
		{	CircuitCode		U32		}
	}
	{
		FOVBlock			Single
		{	GenCounter		U32		}
		{	VerticalAngle	F32	}
	}
}


// AgentHeightWidth - Update to height and aspect, sent as height/width to save space
// Usually sent when window resized or created
{
	AgentHeightWidth Low 83 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
		{	CircuitCode		U32		}
	}
	{
		HeightWidthBlock	Single
		{	GenCounter		U32		}
		{	Height			U16	}
		{	Width			U16	}
	}
}


// AgentSetAppearance - Update to agent appearance
{
	AgentSetAppearance Low 84 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
		{	SerialNum		U32	}	// U32, Increases every time the appearance changes. A value of 0 resets.
		{ 	Size			LLVector3	}
	}
	{
		WearableData		Variable
		{	CacheID			LLUUID }
		{	TextureIndex	U8 }
	}
	{
		ObjectData			Single
		{	TextureEntry	Variable	2	}
	}
	{
		VisualParam			Variable
		{	ParamValue		U8	}
	}
}

// AgentAnimation - Update animation state
// viewer --> simulator
{
	AgentAnimation High 5 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		AnimationList Variable
		{ AnimID		LLUUID }
		{ StartAnim		BOOL }
	}
	{
		PhysicalAvatarEventList Variable
		{ TypeData		Variable	1 }
	}
}

// AgentRequestSit - Try to sit on an object
{
	AgentRequestSit		High 6  NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		TargetObject	Single
		{	TargetID	LLUUID	}
		{	Offset		LLVector3	}
	}
}

// AgentSit - Actually sit on object
{
	AgentSit			High 7  NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
}


// quit message sent between simulators
{
	AgentQuitCopy Low 85 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		FuseBlock			Single
		{	ViewerCircuitCode	U32	}
	}
}


// Request Image - Sent by the viewer to request a specified image at a specified resolution

{
	RequestImage High 8 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		RequestImage			Variable
		{	Image				LLUUID	}
		{	DiscardLevel		S8	}
		{	DownloadPriority	F32	}
		{	Packet				U32	}
		{	Type				U8	}
	}
}

// ImageNotInDatabase
// Simulator informs viewer that a requsted image definitely does not exist in the asset database
{
	ImageNotInDatabase Low 86 Trusted Unencoded
	{
		ImageID				Single
		{	ID				LLUUID	}
	}
}

// RebakeAvatarTextures
// simulator -> viewer request when a temporary baked avatar texture is not found
{
	RebakeAvatarTextures Low 87 Trusted Unencoded
	{
		TextureData			Single
		{	TextureID		LLUUID	}
	}
}


// SetAlwaysRun
// Lets the viewer choose between running and walking
{
	SetAlwaysRun Low 88 NotTrusted Unencoded
	{
		AgentData				Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	AlwaysRun		BOOL	}
	}
}

// ObjectAdd - create new object in the world
// Simulator will assign ID and send message back to signal
// object actually created.
//
// AddFlags (see also ObjectUpdate)
// 0x01 - use physics
// 0x02 - create selected
//
// If only one ImageID is sent for an object type that has more than
// one face, the same image is repeated on each subsequent face.
// 
// Data field is opaque type-specific data for this object
{
	ObjectAdd Medium 1 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID			LLUUID	}
	}
	{
		ObjectData			Single
		{	PCode			U8	}
		{	Material		U8	}
		{	AddFlags		U32	}	// see object_flags.h

		{	PathCurve		U8	}
		{	ProfileCurve	U8	}
		{	PathBegin		U16	}	// 0 to 1, quanta = 0.01
		{	PathEnd			U16	}	// 0 to 1, quanta = 0.01
		{	PathScaleX		U8	}	// 0 to 1, quanta = 0.01
		{	PathScaleY		U8	}	// 0 to 1, quanta = 0.01
		{	PathShearX		U8	}	// -.5 to .5, quanta = 0.01
		{	PathShearY		U8	}	// -.5 to .5, quanta = 0.01
		{	PathTwist		S8	}	// -1 to 1, quanta = 0.01
		{	PathTwistBegin		S8	}	// -1 to 1, quanta = 0.01
		{ 	PathRadiusOffset 	S8	} 	// -1 to 1, quanta = 0.01
		{ 	PathTaperX		S8	}	// -1 to 1, quanta = 0.01
		{	PathTaperY		S8	}	// -1 to 1, quanta = 0.01
		{	PathRevolutions		U8	}	// 0 to 3, quanta = 0.015
		{	PathSkew		S8	}	// -1 to 1, quanta = 0.01
		{	ProfileBegin	U16	}	// 0 to 1, quanta = 0.01
		{	ProfileEnd		U16	}	// 0 to 1, quanta = 0.01
		{	ProfileHollow	U16	}	// 0 to 1, quanta = 0.01

		{	BypassRaycast	U8	}
		{	RayStart		LLVector3	}
		{	RayEnd			LLVector3	}
		{	RayTargetID		LLUUID	}
		{	RayEndIsIntersection	U8	}

		{	Scale			LLVector3	}
		{	Rotation		LLQuaternion	}

		{	State			U8	}
	}
}


// ObjectDelete
// viewer -> simulator
{
	ObjectDelete Low 89 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Force		BOOL	}	// BOOL, god trying to force delete
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}
}


// ObjectDuplicate
// viewer -> simulator
// Makes a copy of a set of objects, offset by a given amount
{
	ObjectDuplicate Low 90 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		SharedData			Single
		{	Offset			LLVector3	}
		{	DuplicateFlags	U32			}	// see object_flags.h
	}
	{
		ObjectData			Variable
		{	ObjectLocalID		U32		}
	}
}


// ObjectDuplicateOnRay
// viewer -> simulator
// Makes a copy of an object, using the add object raycast
// code to abut it to other objects.
{
	ObjectDuplicateOnRay Low 91 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID					LLUUID	}
		{	SessionID				LLUUID	}
		{	GroupID					LLUUID	}
		{	RayStart				LLVector3	}	// region local
		{	RayEnd					LLVector3	}	// region local
		{	BypassRaycast			BOOL	}
		{	RayEndIsIntersection	BOOL	}
		{	CopyCenters				BOOL	}
		{	CopyRotates				BOOL	}
		{	RayTargetID				LLUUID	}
		{	DuplicateFlags			U32		}	// see object_flags.h
	}
	{
		ObjectData			Variable
		{	ObjectLocalID			U32		}
	}
}


// MultipleObjectUpdate
// viewer -> simulator
// updates position, rotation and scale in one message
// positions sent as region-local floats
{
	MultipleObjectUpdate Medium 2 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData		Variable 
		{	ObjectLocalID	U32		}
		{   Type			U8		}
		{	Data			Variable	1	}	// custom type
	}
}

// RequestMultipleObjects
// viewer -> simulator
// reliable
//
// When the viewer gets a local_id/crc for an object that
// it either doesn't have, or doesn't have the current version
// of, it sends this upstream get get an update.
//
// CacheMissType 0 => full object (viewer doesn't have it)
// CacheMissType 1 => CRC mismatch only
{
	RequestMultipleObjects Medium 3 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData	Variable 
		{	CacheMissType	U8	}
		{	ID				U32	}
	}
}


// DEPRECATED: ObjectPosition
// == Old Behavior ==
// Set the position on objects
//
// == Reason for deprecation ==
// Unused code path was removed in the move to Havok4
// Object position, scale and rotation messages were already unified
// to MultipleObjectUpdate and this message was unused cruft.
//
// == New Location ==
// MultipleObjectUpdate can be used instead.
{
	ObjectPosition Medium 4 NotTrusted Zerocoded Deprecated
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32			}
		{	Position		LLVector3	}	// region
	}
}


// DEPRECATED: ObjectScale
// == Old Behavior ==
// Set the scale on objects
//
// == Reason for deprecation ==
// Unused code path was removed in the move to Havok4
// Object position, scale and rotation messages were already unified
// to MultipleObjectUpdate and this message was unused cruft.
//
// == New Location ==
// MultipleObjectUpdate can be used instead.
{
	ObjectScale Low 92 NotTrusted Zerocoded Deprecated
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32			}
		{	Scale			LLVector3	}
	}
}


// ObjectRotation
// viewer -> simulator
{
	ObjectRotation Low 93 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32				}
		{	Rotation		LLQuaternion	}
	}
}


// ObjectFlagUpdate
// viewer -> simulator
{
	ObjectFlagUpdate Low 94 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	ObjectLocalID	U32		}
		{	UsePhysics		BOOL	}
		{	IsTemporary		BOOL	}
		{	IsPhantom		BOOL	}
		{	CastsShadows	BOOL	}
	}
    {
		ExtraPhysics        Variable
		{   PhysicsShapeType U8     }
        {   Density          F32    }
        {   Friction         F32    }
        {   Restitution      F32    }
        {   GravityMultiplier    F32    }

	}
}


// ObjectClickAction
// viewer -> simulator
{
	ObjectClickAction Low 95 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32		}
		{	ClickAction		U8		}
	}
}


// ObjectImage
// viewer -> simulator
{
	ObjectImage Low 96 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData			Variable
		{	ObjectLocalID		U32				}
		{	MediaURL			Variable	1	}
		{	TextureEntry		Variable	2	}
	}
}


{
	ObjectMaterial Low 97 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
		{	Material	U8	}
	}
}


{
	ObjectShape Low 98 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData			Variable
		{	ObjectLocalID	U32	}
		{	PathCurve		U8	}
		{	ProfileCurve	U8	}
		{	PathBegin		U16	}	// 0 to 1, quanta = 0.01
		{	PathEnd			U16	}	// 0 to 1, quanta = 0.01
		{	PathScaleX		U8	}	// 0 to 1, quanta = 0.01
		{	PathScaleY		U8	}	// 0 to 1, quanta = 0.01
		{	PathShearX		U8	}	// -.5 to .5, quanta = 0.01
		{	PathShearY		U8	}	// -.5 to .5, quanta = 0.01
		{	PathTwist		S8	}	// -1 to 1, quanta = 0.01
		{	PathTwistBegin		S8	}	// -1 to 1, quanta = 0.01
		{ 	PathRadiusOffset 	S8	} 	// -1 to 1, quanta = 0.01
		{ 	PathTaperX		S8	}	// -1 to 1, quanta = 0.01
		{	PathTaperY		S8	}	// -1 to 1, quanta = 0.01
		{	PathRevolutions		U8	}	// 0 to 3, quanta = 0.015
		{	PathSkew		S8	}	// -1 to 1, quanta = 0.01
		{	ProfileBegin	U16	}	// 0 to 1, quanta = 0.01
		{	ProfileEnd		U16	}	// 0 to 1, quanta = 0.01
		{	ProfileHollow	U16	}	// 0 to 1, quanta = 0.01
	}
}

{
	ObjectExtraParams Low 99 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData			Variable
		{	ObjectLocalID	U32				}
		{	ParamType		U16				}
		{	ParamInUse		BOOL			}
		{	ParamSize		U32				}
		{	ParamData		Variable	1	}
	}
}


// ObjectOwner
// To make public, set OwnerID to LLUUID::null.
// TODO: Eliminate god-bit. Maybe not. God-bit is ok, because it's
// known on the server.
{
	ObjectOwner Low 100 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		HeaderData		Single
		{	Override	BOOL	}	// BOOL, God-bit.
		{	OwnerID		LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		ObjectData	Variable
		{	ObjectLocalID	U32	}
	}
}

// ObjectGroup
// To make the object part of no group, set GroupID = LLUUID::null.
// This call only works if objectid.ownerid == agentid.
{
	ObjectGroup	Low	101 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		ObjectData	Variable
		{	ObjectLocalID	U32	}
	}
}

// Attempt to buy an object. This will only pack root objects.
{
	ObjectBuy Low 102 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
		{	CategoryID	LLUUID	}	// folder where it goes (if derezed)
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
		{	SaleType		U8	}   // U8 -> EForSale
		{	SalePrice		S32	}
	}
}

// viewer -> simulator

// buy object inventory. If the transaction succeeds, it will add
// inventory to the agent, and potentially remove the original.
{
	BuyObjectInventory	Low	103 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data	Single
		{	ObjectID	LLUUID	}
		{	ItemID		LLUUID	}
		{	FolderID	LLUUID	}
	}
}

// sim -> viewer
// Used to propperly handle buying asset containers
{
	DerezContainer		Low	104 	Trusted Zerocoded
	{
		Data			Single
		{	ObjectID	LLUUID	}
		{	Delete		BOOL	}  // BOOL
	}
}

// ObjectPermissions
// Field - see llpermissionsflags.h
// If Set is true, tries to turn on bits in mask.
// If set is false, tries to turn off bits in mask.
// BUG: This just forces the permissions field.
{
	ObjectPermissions Low 105 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		HeaderData		Single
		{	Override	BOOL	}	// BOOL, God-bit.
	}
	{
		ObjectData	Variable
		{	ObjectLocalID	U32	}
		{	Field		U8	}
		{	Set			U8	}
		{	Mask		U32	}
	}
}

// set object sale information
{
	ObjectSaleInfo Low 106 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData			Variable
		{	LocalID			U32	}
		{	SaleType		U8	}   // U8 -> EForSale
		{	SalePrice		S32	}
	}
}


// set object names
{
	ObjectName Low 107 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	LocalID		U32	}
		{	Name		Variable	1	}
	}
}

// set object descriptions
{
	ObjectDescription Low 108 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	LocalID		U32	}
		{	Description	Variable	1	}
	}
}

// set object category
{
	ObjectCategory Low 109 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	LocalID		U32	}
		{	Category	U32	}
	}
}

// ObjectSelect
// Variable object data because rectangular selection can
// generate a large list very quickly.
{
	ObjectSelect Low 110 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}

}


// ObjectDeselect
{
	ObjectDeselect Low 111 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}

}

// ObjectAttach
{
	ObjectAttach Low 112 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	AttachmentPoint	U8	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32				}
		{	Rotation		LLQuaternion	}
	}
}

// ObjectDetach -- derezzes an attachment, marking its item in your inventory as not "(worn)"
{
	ObjectDetach Low 113 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}
}


// ObjectDrop -- drops an attachment from your avatar onto the ground
{
	ObjectDrop Low 114 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}
}


// ObjectLink
{
	ObjectLink Low 115 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}
}

// ObjectDelink
{
	ObjectDelink Low 116 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32	}
	}
}


// ObjectGrab
{
	ObjectGrab Low 117 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData			Single
		{	LocalID				U32  }
		{	GrabOffset			LLVector3 }
	}
	{
		SurfaceInfo     Variable
		{   UVCoord     LLVector3 }
		{   STCoord     LLVector3 }
       	{   FaceIndex   S32 }
       	{   Position    LLVector3 }
       	{   Normal      LLVector3 }
       	{   Binormal    LLVector3 }
	}
}


// ObjectGrabUpdate
// TODO: Quantize this data, reduce message size.
// TimeSinceLast could go to 1 byte, since capped
// at 100 on sim.
{
	ObjectGrabUpdate Low 118 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData	Single
		{	ObjectID			LLUUID	}
		{	GrabOffsetInitial	LLVector3	}	// LLVector3
		{	GrabPosition		LLVector3	}	// LLVector3, region local
		{	TimeSinceLast		U32	}
	}
	{
		SurfaceInfo     Variable
		{   UVCoord     LLVector3 }
		{   STCoord     LLVector3 }
       	{   FaceIndex   S32 }
       	{   Position    LLVector3 }
       	{   Normal      LLVector3 }
       	{   Binormal    LLVector3 }
	}

}


// ObjectDeGrab				
{
	ObjectDeGrab Low 119 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData			Single
		{	LocalID				U32  }
	}
	{
		SurfaceInfo     Variable
		{   UVCoord     LLVector3 }
		{   STCoord     LLVector3 }
       	{   FaceIndex   S32 }
       	{   Position    LLVector3 }
       	{   Normal      LLVector3 }
       	{   Binormal    LLVector3 }
	}
}


// ObjectSpinStart
{
	ObjectSpinStart Low 120 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData			Single
		{	ObjectID		LLUUID	}
	}
}


// ObjectSpinUpdate
{
	ObjectSpinUpdate Low 121 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData			Single
		{	ObjectID		LLUUID	}
		{	Rotation		LLQuaternion }
	}
}


// ObjectSpinStop
{
	ObjectSpinStop Low 122 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData			Single
		{	ObjectID		LLUUID	}
	}
}

// Export selected objects
// viewer->sim
{
	ObjectExportSelected Low 123 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	RequestID		LLUUID  }
		{	VolumeDetail	S16		}
	}
	{
		ObjectData			Variable
		{	ObjectID		LLUUID	}
	}
}


// ModifyLand - sent to modify a piece of land on a simulator.
// viewer -> sim
{
	ModifyLand Low 124 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ModifyBlock			Single
		{	Action			U8	}
		{	BrushSize		U8	}
		{	Seconds			F32	}
		{	Height			F32	}
	}
	{
		ParcelData			Variable
		{	LocalID			S32		}
		{	West			F32		}
		{	South			F32		}
		{	East			F32		}
		{	North			F32		}
	}
	{
		ModifyBlockExtended Variable
		{   BrushSize       F32 }
	}
}


// VelocityInterpolateOn
// viewer->sim
// requires administrative access
{
	VelocityInterpolateOn Low 125 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID		LLUUID	}
	}
}


// VelocityInterpolateOff
// viewer->sim
// requires administrative access
{
	VelocityInterpolateOff Low 126 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID		LLUUID	}
	}
}

// Save State
// viewer->sim
// requires administrative access
{
	StateSave Low 127 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		DataBlock		Single
		{	Filename	Variable	1	}
	}
}

// ReportAutosaveCrash
// sim->launcher
{
	ReportAutosaveCrash Low 128 NotTrusted Unencoded
	{
		AutosaveData	Single
		{	PID		S32	}
		{	Status	S32 }
	}
}

// SimWideDeletes
{
	SimWideDeletes Low 129 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		DataBlock			Single
		{	TargetID		LLUUID	}
		{	Flags			U32	}
	}
}

// RequestObjectPropertiesFamily
// Ask for extended information, such as creator, permissions, resources, etc.
// Medium frequency because it is driven by mouse hovering over objects, which
// occurs at high rates.
{
	RequestObjectPropertiesFamily Medium 5 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Single
		{	RequestFlags		U32	}
		{	ObjectID			LLUUID	}
	}
}


// Track agent - this information is used when sending out the
// coarse location update so that we know who you are tracking. 
// To stop tracking - send a null uuid as the prey.
{
	TrackAgent		Low	130 	NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		TargetData		Single
		{	PreyID		LLUUID	}
	}
}

// end viewer to simulator section

{
       ViewerStats Low 131 NotTrusted Zerocoded UDPDeprecated
       {
               AgentData                       Single
               {       AgentID                 LLUUID      }
               {       SessionID               LLUUID      }
               {       IP                      IPADDR       }
               {       StartTime               U32       }
               {       RunTime                 F32       }  // F32
               {       SimFPS                  F32       }  // F32
               {       FPS                     F32       }  // F32
               {       AgentsInView			   U8 } //
               {       Ping                    F32       }  // F32
               {       MetersTraveled  F64       }
               {       RegionsVisited  S32       }
               {       SysRAM                  U32       }
               {       SysOS                   Variable        1       }  // String
               {       SysCPU                  Variable        1       }  // String
               {       SysGPU                  Variable        1       }  // String
       }

       {
               DownloadTotals          Single
               {       World                   U32       }
               {       Objects                 U32       }
               {       Textures                U32       }
       }

       {
               NetStats                        Multiple        2
               {       Bytes                   U32       }
               {       Packets                 U32       }
               {       Compressed              U32       }
               {       Savings                 U32       }
       }

       {       
               FailStats                       Single
               {       SendPacket              U32       }
               {       Dropped                 U32       }
               {       Resent                  U32       }
               {       FailedResends   U32       }
               {       OffCircuit              U32       }
               {       Invalid                 U32       }
       }

       {
               MiscStats                       Variable
               {       Type                    U32       }
               {       Value                   F64       }
       }
}
		
// ScriptAnswerYes
// reliable
{
	ScriptAnswerYes Low 132 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}			   
	{
		Data	Single
		{	TaskID			LLUUID	}
		{	ItemID			LLUUID	}
		{	Questions		S32	}
	}
}


// complaint/bug-report
// reliable
{
	UserReport	Low	133 	NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ReportData	Single
		{   ReportType		U8   }   // BUG=1, COMPLAINT=2
		{   Category		U8   }   // see sequence.user_report_category
		{	Position		LLVector3	} // screenshot position, region-local
		{   CheckFlags		U8	} // checkboxflags
		{   ScreenshotID	LLUUID  }
		{   ObjectID		LLUUID  }
		{   AbuserID		LLUUID  }
		{	AbuseRegionName	Variable	1	}
		{	AbuseRegionID	LLUUID	}
		{   Summary			Variable	1   }
		{   Details 		Variable	2   }
		{	VersionString	Variable	1	}
	}
}


// ***************************************************************************
// Simulator to Viewer Messages
// ***************************************************************************

// AlertMessage
// Specifies the text to be posted in an alert dialog
{
	AlertMessage Low 134 Trusted Unencoded
	{
		AlertData			Single
		{	Message			Variable	1	}
	}
	{
		AlertInfo			Variable
		{	Message			Variable	1	}
		{	ExtraParams		Variable	1	}
	}
}

// Send an AlertMessage to the named agent.
// usually dataserver->simulator
{
	AgentAlertMessage	Low	135 Trusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
	{
		AlertData			Single
		{	Modal			BOOL			}
		{	Message			Variable	1	}
	}
}


// MeanCollisionAlert
// Specifies the text to be posted in an alert dialog
{
	MeanCollisionAlert Low 136 Trusted Zerocoded
	{
		MeanCollision		Variable
		{	Victim			LLUUID	}
		{	Perp			LLUUID	}
		{	Time			U32		}
		{	Mag				F32		}
		{	Type			U8		}
	}
}

// ViewerFrozenMessage
// Specifies the text to be posted in an alert dialog
{
	ViewerFrozenMessage Low 137 Trusted Unencoded
	{
		FrozenData			Single
		{	Data			BOOL	}
	}
}

// Health Message
// Tells viewer what agent health is
{
	HealthMessage Low 138 Trusted Zerocoded
	{
		HealthData			Single
		{	Health			F32	}
	}
}

// ChatFromSimulator
// Chat text to appear on a user's screen
// Position is region local.
// Viewer can optionally use position to animate
// If audible is CHAT_NOT_AUDIBLE, message will not be valid
{
	ChatFromSimulator Low 139 Trusted Unencoded
	{
		ChatData			Single
		{	FromName		Variable 1	}
		{	SourceID		LLUUID		}	// agent id or object id
		{	OwnerID			LLUUID		}	// object's owner
		{	SourceType		U8			}
		{	ChatType		U8			}
		{	Audible			U8			}
		{	Position		LLVector3	}
		{	Message			Variable 2	}	// UTF-8 text
	}
}


// Simulator statistics packet (goes out to viewer and dataserver/spaceserver)
{
	SimStats Low 140 Trusted Unencoded
	{
		Region Single
		{	RegionX				U32				}
		{	RegionY				U32				}
		{	RegionFlags			U32				}
		{	ObjectCapacity		U32				}
	}
	{
		Stat	Variable
		{	StatID		U32	}
		{	StatValue	F32	}
	}
	{
		PidStat Single
		{	PID					S32				}
	}
	{
		RegionInfo Variable
		{	RegionFlagsExtended	U64				}
	}
}

// viewer -> sim
// reliable
{
	RequestRegionInfo Low 141 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
}

// RegionInfo
// Used to populate UI for both region/estate floater
// and god tools floater
// sim -> viewer
// reliable
{
	RegionInfo Low 142 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		RegionInfo	Single
		{	SimName			Variable	1	}	// string
		{	EstateID		U32				}
		{	ParentEstateID	U32				}
		{	RegionFlags		U32				}
		{	SimAccess		U8				}
		{	MaxAgents		U8				}
		{	BillableFactor		F32			}
		{	ObjectBonusFactor 	F32			}
		{	WaterHeight			F32			}
		{	TerrainRaiseLimit	F32 		}
		{	TerrainLowerLimit	F32 		}
		{	PricePerMeter 		S32			}
		{	RedirectGridX 		S32			}
		{	RedirectGridY 		S32			}
		{	UseEstateSun		BOOL		}
		{	SunHour				F32			}	// last value set by estate or region controls JC
	}
	{
		RegionInfo2		Single
		{	ProductSKU			Variable 1	}	// string
		{	ProductName			Variable 1	}	// string
		{	MaxAgents32			U32			}	// Identical to RegionInfo.MaxAgents but allows greater range
		{	HardMaxAgents		U32			}
		{	HardMaxObjects		U32			}
	}
	{
		RegionInfo3		Variable
		{	RegionFlagsExtended	U64			}
	}
}

// GodUpdateRegionInfo
// Sent from viewer to sim after a god has changed some
// of the parameters in the god tools floater
// viewer -> sim
// reliable
{
	GodUpdateRegionInfo Low 143 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		RegionInfo	Single
		{	SimName			Variable	1	}	// string
		{	EstateID		U32				}
		{	ParentEstateID	U32				}
		{	RegionFlags		U32				}
		{	BillableFactor		F32			}
		{	PricePerMeter 		S32			}
		{	RedirectGridX 		S32			}
		{	RedirectGridY 		S32			}
	}
	{
		RegionInfo2	Variable
		{	RegionFlagsExtended	U64			}
	}
}

//NearestLandingRegionRequest
//sim->dataserver
//Sent from the region to the data server
//to request the most up to date region for the requesting
//region to redirect teleports to
{
	NearestLandingRegionRequest Low 144 Trusted Unencoded
	{
		RequestingRegionData Single
		{	RegionHandle 		U64			}
	}
}

//NearestLandingPointReply
//dataserver->sim
//Sent from the data server to a region in reply
//to the redirectregion request stating which region
//the requesting region should redirect teleports to if necessary
{
	NearestLandingRegionReply Low 145 Trusted Unencoded
	{
		LandingRegionData Single
		{	RegionHandle		U64			}
	}
}

//NearestLandingPointUpdated
//sim->dataserver
//Sent from a region to the data server
//to have the dataserver note/clear in the db
//that the region has updated it's nearest landing point
{
	NearestLandingRegionUpdated Low 146 Trusted Unencoded
	{
		RegionData Single
		{	RegionHandle		U64			}
	}
}


//TeleportLandingStatusChanged
//sim->dataserver
//Sent from the region to the data server
//to note that the region's teleportation landing status has changed
{
	TeleportLandingStatusChanged Low 147 Trusted Unencoded
	{
		RegionData Single
		{	RegionHandle		U64			}
	}
}

// RegionHandshake
// Sent by region to viewer after it has received UseCircuitCode
// from that viewer.
// sim -> viewer
// reliable
{
	RegionHandshake			Low	148 	Trusted Zerocoded
	{
		RegionInfo	Single
		{	RegionFlags		U32	}
		{	SimAccess		U8	}
		{	SimName			Variable	1	} // string
		{	SimOwner		LLUUID	}
		{	IsEstateManager	BOOL	}	// this agent, for this sim
		{	WaterHeight		F32	}
		{	BillableFactor	F32	}
		{	CacheID			LLUUID }
		{	TerrainBase0		LLUUID		}
		{	TerrainBase1		LLUUID		}
		{	TerrainBase2		LLUUID		}
		{	TerrainBase3		LLUUID		}
		{	TerrainDetail0		LLUUID		}
		{	TerrainDetail1		LLUUID		}
		{	TerrainDetail2		LLUUID		}
		{	TerrainDetail3		LLUUID		}
		{	TerrainStartHeight00	F32		}
		{	TerrainStartHeight01	F32		}
		{	TerrainStartHeight10	F32		}
		{	TerrainStartHeight11	F32		}
		{	TerrainHeightRange00	F32		}
		{	TerrainHeightRange01	F32		}
		{	TerrainHeightRange10	F32		}
		{	TerrainHeightRange11	F32		}
	}
	{
		RegionInfo2	Single
		{	RegionID		LLUUID	}
	}
	{
		RegionInfo3	Single
		{	CPUClassID				S32		}
		{	CPURatio				S32		}
		{	ColoName				Variable	1	}	// string
		{	ProductSKU				Variable	1	}	// string
		{	ProductName				Variable	1	}	// string
	}
	{
		RegionInfo4		Variable
		{	RegionFlagsExtended	U64			}
		{	RegionProtocols		U64			}
	}
}

// RegionHandshakeReply
// viewer -> sim
// reliable
// Sent after viewer has initialized the (pre-existing)
// LLViewerRegion with the name, access level, etc. and
// has loaded the cache for the region.
// After the simulator receives this, it will start sending
// data about objects.
{
	RegionHandshakeReply			Low	149 	NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		RegionInfo	Single
		{	Flags			U32		}
	}
}

// The CoarseLocationUpdate message is sent to notify the viewer of 
// the location of mappable objects in the region. 1 meter resolution is
// sufficient for this. The index block is used to show where you are, 
// and where someone you are tracking is located. They are -1 if not
// applicable.
{
	CoarseLocationUpdate Medium 6 Trusted Unencoded
	{
		Location	Variable
		{	X		U8	}
		{	Y		U8	}
		{	Z		U8	}	// Z in meters / 4
	}
	{
		Index 		Single
		{	You		S16	}
		{	Prey	S16	}
	}
    {
         AgentData       Variable
         {       AgentID                 LLUUID                  }
    }
}

// ImageData - sent to viewer to transmit information about an image 
{
	ImageData High 9 Trusted Unencoded
	{
		ImageID				Single
		{	ID				LLUUID	}
		{	Codec			U8	}
		{	Size			U32	}
		{	Packets			U16	}
	}
	{
		ImageData			Single
		{	Data			Variable	2	}
	}
}

// ImagePacket - follow on image data for images having > 1 packet of data 
{
	ImagePacket High 10 Trusted Unencoded
	{
		ImageID				Single
		{	ID				LLUUID	}
		{	Packet			U16	}
	}
	{
		ImageData			Single
		{	Data			Variable	2	}
	}
}

// LayerData - Sent to viewer - encodes layer data

{
	LayerData High 11 Trusted Unencoded
	{
		LayerID				Single
		{	Type			U8	}

	}
	{
		LayerData			Single
		{	Data			Variable	2	}
	}
}

// ObjectUpdate - Sent by objects from the simulator to the viewer
//
// If only one ImageID is sent for an object type that has more than
// one face, the same image is repeated on each subsequent face.
// 
// NameValue is a list of name-value strings, separated by \n characters,
// terminated by \0
//
// Data is type-specific opaque data for this object
{
	ObjectUpdate High 12 Trusted Zerocoded
	{
		RegionData			Single
		{	RegionHandle	U64	}
		{   TimeDilation	U16   }
	}
	{
		ObjectData			Variable
		{	ID				U32	}
		{	State			U8	}

		{	FullID			LLUUID	}
		{	CRC				U32	}	// TEMPORARY HACK FOR JAMES
		{	PCode			U8	}
		{	Material		U8	}
		{	ClickAction		U8	}
		{	Scale			LLVector3	}
		{	ObjectData		Variable	1	}

		{	ParentID		U32	}
		{	UpdateFlags		U32	}	// U32, see object_flags.h

		{	PathCurve		U8	}
		{	ProfileCurve	U8	}
		{	PathBegin		U16	}	// 0 to 1, quanta = 0.01
		{	PathEnd			U16	}	// 0 to 1, quanta = 0.01
		{	PathScaleX		U8	}	// 0 to 1, quanta = 0.01
		{	PathScaleY		U8	}	// 0 to 1, quanta = 0.01
		{	PathShearX		U8	}	// -.5 to .5, quanta = 0.01
		{	PathShearY		U8	}	// -.5 to .5, quanta = 0.01
		{	PathTwist		S8	}	// -1 to 1, quanta = 0.01
		{	PathTwistBegin		S8	}	// -1 to 1, quanta = 0.01
		{ 	PathRadiusOffset 	S8	} 	// -1 to 1, quanta = 0.01
		{ 	PathTaperX		S8	}	// -1 to 1, quanta = 0.01
		{	PathTaperY		S8	}	// -1 to 1, quanta = 0.01
		{	PathRevolutions		U8	}	// 0 to 3, quanta = 0.015
		{	PathSkew		S8	}	// -1 to 1, quanta = 0.01
		{	ProfileBegin	U16	}	// 0 to 1, quanta = 0.01
		{	ProfileEnd		U16	}	// 0 to 1, quanta = 0.01
		{	ProfileHollow	U16	}	// 0 to 1, quanta = 0.01

		{	TextureEntry	Variable	2	}
		{	TextureAnim		Variable	1	}

		{	NameValue		Variable	2	}
		{	Data			Variable	2	}
		{	Text			Variable	1	}	// llSetText() hovering text
		{	TextColor		Fixed		4	}	// actually, a LLColor4U
		{	MediaURL		Variable	1	}	// URL for web page, movie, etc.

		// Info for particle systems
		{	PSBlock			Variable	1	}
		
		// Extra parameters
		{	ExtraParams		Variable	1	}
		
		// info for looped attached sounds
        // because these are almost always all zero
		// the hit after zero-coding is only 2 bytes
		// not the 42 you see here
		{	Sound           LLUUID  }
		{	OwnerID			LLUUID	}	// HACK object's owner id, only set if non-null sound, for muting
		{	Gain            F32		}
		{	Flags           U8		}
		{	Radius          F32		}  // cutoff radius

		// joint info -- is sent in the update of each joint-child-root
		{	JointType 			U8		}
		{	JointPivot			LLVector3	}
		{	JointAxisOrAnchor 	LLVector3	}
	}
}


// ObjectUpdateCompressed
{
	ObjectUpdateCompressed High 13 Trusted Unencoded
	{
		RegionData			Single
		{	RegionHandle	U64		}
		{   TimeDilation	U16		}
	}
	{
		ObjectData			Variable
		{   UpdateFlags			U32	}
		{	Data			Variable   2	}
	}
}

// ObjectUpdateCached
// reliable
{
	ObjectUpdateCached High 14 Trusted Unencoded
	{
		RegionData			Single
		{	RegionHandle	U64		}
		{   TimeDilation	U16		}
	}
	{
		ObjectData			Variable
		{   ID				U32		}
		{	CRC				U32		}
		{   UpdateFlags		U32	}
	}
}

// packed terse object update format
{
	ImprovedTerseObjectUpdate High 15 Trusted Unencoded
	{
		RegionData			Single
		{	RegionHandle	U64	}
		{   TimeDilation	U16   }
	}
	{
		ObjectData			Variable
		{	Data			Variable	1	}
		{	TextureEntry	Variable	2	}
	}
}

// KillObject - Sent by objects to the viewer to tell them to kill themselves

{
	KillObject High 16 Trusted Unencoded
	{
		ObjectData			Variable
		{	ID				U32	}
	}
}


// CrossedRegion - new way to tell a viewer it has gone across a region 
// boundary
{
	CrossedRegion Medium 7 Trusted Unencoded UDPBlackListed
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		RegionData		Single
		{	SimIP			IPADDR	}
		{	SimPort			IPPORT	}
		{	RegionHandle	U64	}
		{	SeedCapability	Variable	2	}	// URL
	}
	{
		Info			Single
		{	Position		LLVector3	}
		{	LookAt			LLVector3	}
	}
}

// SimulatorViewerTimeMessage - Allows viewer to resynch to world time

{
	SimulatorViewerTimeMessage Low 150 Trusted Unencoded
	{
		TimeInfo		Single
		{	UsecSinceStart	U64	}
		{	SecPerDay		U32	}
		{	SecPerYear		U32	}
		{	SunDirection	LLVector3	}
		{	SunPhase		F32	}
		{	SunAngVelocity	LLVector3	}
	}
}

// EnableSimulator - Preps a viewer to receive data from a simulator

{
	EnableSimulator Low 151 Trusted Unencoded UDPBlackListed
	{
		SimulatorInfo	Single
		{	Handle		U64	}
		{	IP			IPADDR	}
		{	Port		IPPORT	}
	}
}

// DisableThisSimulator - Tells a viewer not to expect data from this simulator anymore

{
	DisableSimulator Low 152 Trusted Unencoded
}

// ConfirmEnableSimulator - A confirmation message sent from simulator to neighbors that the simulator
// has successfully been enabled by the viewer

{
	ConfirmEnableSimulator Medium 8 Trusted Unencoded
	{
		AgentData				Single
		{	AgentID				LLUUID	}
		{	SessionID			LLUUID	}
	}
}

//-----------------------------------------------------------------------------
// New Transfer system
//-----------------------------------------------------------------------------

// Request a new transfer (target->source)
{
	TransferRequest Low 153 NotTrusted Zerocoded
	{
		TransferInfo		Single
		{	TransferID		LLUUID	}
		{	ChannelType		S32	}
		{	SourceType		S32	}
		{	Priority		F32	}
		{	Params			Variable	2	}
	}
}

// Return info about a transfer/initiate transfer (source->target)
// Possibly should have a Params field like above
{
	TransferInfo Low 154 NotTrusted Zerocoded
	{
		TransferInfo		Single
		{	TransferID		LLUUID	}
		{	ChannelType		S32	}
		{	TargetType		S32	}
		{	Status			S32	}
		{	Size			S32	}
		{	Params			Variable	2	}
	}
}

{
	TransferPacket High 17 NotTrusted Unencoded
	{
		TransferData Single
		{	TransferID	LLUUID	}
		{	ChannelType	S32	}
		{	Packet		S32	}
		{	Status		S32	}
		{	Data		Variable 2	}
	}
}

// Abort a transfer in progress (either from target->source or source->target)
{
	TransferAbort Low 155 NotTrusted Zerocoded
	{
		TransferInfo		Single
		{	TransferID		LLUUID	}
		{	ChannelType		S32	}
	}
}


//-----------------------------------------------------------------------------
// General file transfer
//-----------------------------------------------------------------------------

// RequestXfer - request an arbitrary xfer
{
	RequestXfer Low 156 NotTrusted Zerocoded
	{
		XferID				Single
		{	ID				U64	}
		{	Filename		Variable	1	}
		{	FilePath		U8	} // ELLPath
		{	DeleteOnCompletion BOOL	} // BOOL
		{	UseBigPackets	BOOL	} // BOOL
		{	VFileID			LLUUID	}
		{	VFileType		S16	}
	}
}

// SendXferPacket - send an additional packet of an arbitrary xfer from sim -> viewer 
{
	SendXferPacket High 18 NotTrusted Unencoded
	{
		XferID				Single
		{	ID				U64	}
		{	Packet			U32	}
	}
	{
		DataPacket			Single
		{	Data			Variable	2	}
	}
}

// ConfirmXferPacket
{
	ConfirmXferPacket High 19 NotTrusted Unencoded
	{
		XferID				Single
		{	ID				U64	}
		{	Packet			U32	}
	}
}

// AbortXfer
{
	AbortXfer Low 157 NotTrusted Unencoded
	{
		XferID				Single
		{	ID				U64	}
		{	Result			S32	}
	}
}

//-----------------------------------------------------------------------------
// Avatar information
//-----------------------------------------------------------------------------


// AvatarAnimation - Update animation state
// simulator --> viewer 
{
	AvatarAnimation High 20 Trusted Unencoded
	{
		Sender			Single
		{	ID			LLUUID	}
	}
	{
		AnimationList Variable
		{ AnimID		LLUUID }
		{ AnimSequenceID S32 }
	}
	{
		AnimationSourceList Variable
		{ ObjectID		LLUUID }
	}
	{
		PhysicalAvatarEventList Variable
		{ TypeData		Variable	1 }
	}

}

// AvatarAppearance - Update visual params
{
	AvatarAppearance Low 158 Trusted Zerocoded
	{
		Sender				Single
		{	ID				LLUUID	}
		{	IsTrial			BOOL    }
	}
	{	
		ObjectData			Single
		{	TextureEntry	Variable	2	}
	}
	{
		VisualParam			Variable
		{	ParamValue		U8	}
	}
	{
		AppearanceData		Variable
		{	AppearanceVersion	U8	}
		{	CofVersion			S32	}
		{	Flags				U32	}
	}
}

// AvatarSitResponse - response to a request to sit on an object
{
	AvatarSitResponse	High 21 Trusted Zerocoded
	{
		SitObject			Single
		{	ID				LLUUID	}
	}
	{
		SitTransform		Single
		{	AutoPilot		BOOL }
		{	SitPosition		LLVector3	}
		{	SitRotation		LLQuaternion	}
		{	CameraEyeOffset	LLVector3	}
		{	CameraAtOffset	LLVector3	}
		{	ForceMouselook	BOOL }
	}
}

// SetFollowCamProperties
{ 
	SetFollowCamProperties		Low	159 Trusted	Unencoded
	{
		ObjectData			Single
		{	ObjectID				LLUUID	}
	}
	{
		CameraProperty	Variable
		{ 	Type 	S32 }
		{ 	Value 	F32 }
	}
}

// ClearFollowCamProperties
{
	ClearFollowCamProperties	Low 160 Trusted Unencoded
	{
		ObjectData			Single
		{	ObjectID				LLUUID	}
	}
}

// CameraConstraint - new camera distance limit (based on collision with objects)
{
	CameraConstraint High 22 Trusted Zerocoded
	{
		CameraCollidePlane	Single
		{	Plane		LLVector4 }
	}
}

// ObjectProperties
// Extended information such as creator, permissions, etc.
// Medium because potentially driven by mouse hover events.
{
	ObjectProperties Medium 9 Trusted Zerocoded
	{
		ObjectData			Variable
		{	ObjectID		LLUUID	}
		{	CreatorID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	GroupID			LLUUID	}
		{	CreationDate	U64	}
		{	BaseMask		U32	}
		{	OwnerMask		U32	}
		{	GroupMask		U32	}
		{	EveryoneMask	U32	}
		{	NextOwnerMask	U32	}
		{	OwnershipCost	S32	}
//		{	TaxRate			F32	}	// F32
		{	SaleType		U8	}   // U8 -> EForSale
		{	SalePrice		S32	}
		{	AggregatePerms	U8	}
		{	AggregatePermTextures		U8	}
		{	AggregatePermTexturesOwner	U8	}
		{	Category		U32	}	// LLCategory
		{	InventorySerial	S16	}	// S16
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	FromTaskID		LLUUID	}
		{	LastOwnerID		LLUUID	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	TouchName		Variable	1	}
		{	SitName			Variable	1	}
		{	TextureID		Variable	1	}
	}
}

// ObjectPropertiesFamily
// Medium because potentially driven by mouse hover events.
{
	ObjectPropertiesFamily Medium 10 Trusted Zerocoded
	{
		ObjectData			Single
		{	RequestFlags	U32	}
		{	ObjectID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	GroupID			LLUUID	}
		{	BaseMask		U32	}
		{	OwnerMask		U32	}
		{	GroupMask		U32	}
		{	EveryoneMask	U32	}
		{	NextOwnerMask	U32	}
		{	OwnershipCost	S32	}
		{	SaleType		U8	}   // U8 -> EForSale
		{	SalePrice		S32	}
		{	Category		U32	}	// LLCategory
		{	LastOwnerID		LLUUID	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
	}
}

// RequestPayPrice
// viewer -> sim
{
	RequestPayPrice Low 161 NotTrusted Unencoded
	{
		ObjectData		Single
		{	ObjectID	LLUUID	}
	}
}

// PayPriceReply
// sim -> viewer
{
	PayPriceReply Low 162 Trusted Unencoded
	{
		ObjectData		Single
		{	ObjectID	LLUUID }
		{	DefaultPayPrice	S32	}
	}
	{
		ButtonData		Variable
		
		{	PayButton	S32	}
	}
}

// KickUser
// *FIXME*
// Kick off a logged-in user, such as when two people log in with the
// same account name.
// ROUTED dataserver -> userserver -> spaceserver -> simulator -> viewer
// reliable, but that may not matter if a system component is quitting
{
	KickUser Low 163 Trusted Unencoded
	{
		TargetBlock			Single
		{	TargetIP		IPADDR	}	// U32 encoded IP
		{	TargetPort		IPPORT	}
	}
	{
		UserInfo		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Reason		Variable	2	}	// string
	}
}

// ack sent from the simulator up to the main database so that login
// can continue.
{
	KickUserAck Low 164 Trusted Unencoded
	{
		UserInfo		Single
		{	SessionID	LLUUID	}
		{	Flags		U32		}
	}
}

// GodKickUser
// When a god wants someone kicked
// viewer -> sim
// reliable
{
	GodKickUser Low 165 NotTrusted Unencoded
	{
		UserInfo		Single
		{	GodID			LLUUID	}
		{	GodSessionID	LLUUID	}
		{	AgentID			LLUUID	}
		{	KickFlags		U32	}
		{	Reason			Variable	2	}	// string
	}
}

// SystemKickUser
// user->space, reliable
{
	SystemKickUser	Low	166 Trusted Unencoded
	{
		AgentInfo	Variable
		{	AgentID	LLUUID	}
	}
}

// EjectUser
// viewer -> sim
// reliable
{
	EjectUser	Low	167 	NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data		Single
		{	TargetID	LLUUID	}
		{	Flags		U32		}
	}
}

// FreezeUser
// Freeze someone who is on my land.
// viewer -> sim
// reliable
{
	FreezeUser	Low	168 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data		Single
		{	TargetID	LLUUID	}
		{	Flags		U32		}
	}
}


// AvatarPropertiesRequest
// viewer -> simulator
// reliable
{
	AvatarPropertiesRequest Low 169 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	AvatarID		LLUUID	}
	}
}

// AvatarPropertiesRequestBackend
// simulator -> dataserver
// reliable
{
	AvatarPropertiesRequestBackend Low 170 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	AvatarID		LLUUID	}
		{	GodLevel		U8		}
		{	WebProfilesDisabled		BOOL	}
	}
}
// AvatarPropertiesReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	AvatarPropertiesReply Low 171 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}	// your id
		{	AvatarID		LLUUID		}	// avatar you're asking about
	}
	{
		PropertiesData		Single
		{	ImageID			LLUUID		}
		{	FLImageID		LLUUID		}
		{	PartnerID		LLUUID		}
		{	AboutText		Variable 2	}	// string, up to 512
		{	FLAboutText		Variable 1	}	// string
		{	BornOn			Variable 1	}	// string
		{	ProfileURL		Variable 1	}	// string
		{	CharterMember	Variable 1	}	// special - usually U8
		{	Flags			U32			}
	}
}

{
	AvatarInterestsReply Low 172 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}	// your id
		{	AvatarID		LLUUID		}	// avatar you're asking about
	}
	{
		PropertiesData		Single
		{	WantToMask		U32			}
		{	WantToText		Variable 1	}	// string
		{	SkillsMask		U32			}
		{	SkillsText		Variable 1	}	// string
		{	LanguagesText	Variable 1	}	// string
	}
}

// AvatarGroupsReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	AvatarGroupsReply Low 173 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID		}	// your id
		{	AvatarID		LLUUID		}	// avatar you're asking about
	}
	{
		GroupData			Variable
		{	GroupPowers			U64			}
		{	AcceptNotices		BOOL		}
		{	GroupTitle			Variable 1	}
		{	GroupID				LLUUID		}
		{	GroupName			Variable 1	}
		{	GroupInsigniaID		LLUUID		}
	}
	{
		NewGroupData		Single
		{	ListInProfile	BOOL		}	// whether group displays in profile
	}
}


// AvatarPropertiesUpdate
// viewer -> simulator
// reliable
{
	AvatarPropertiesUpdate Low 174 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		PropertiesData	Single
		{	ImageID			LLUUID			}
		{	FLImageID		LLUUID			}
		{	AboutText		Variable	2	}	// string, up to 512
		{	FLAboutText		Variable	1	}
		{	AllowPublish		BOOL			}	// whether profile is externally visible or not
		{	MaturePublish		BOOL		}	// profile is "mature"
		{	ProfileURL		Variable	1	}	// string
	}
}

// AvatarInterestsUpdate
// viewer -> simulator
// reliable
{
	AvatarInterestsUpdate Low 175 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		PropertiesData	Single
		{	WantToMask		U32				}
		{	WantToText		Variable	1	}	// string
		{	SkillsMask		U32				}
		{	SkillsText		Variable	1	}	// string
		{	LanguagesText	Variable	1	}	// string	
	}
}



// AvatarNotesReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	AvatarNotesReply Low 176 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
	}
	{
		Data		Single
		{	TargetID		LLUUID		}
		{	Notes			Variable 2	}	// string
	}
}


// AvatarNotesUpdate
// viewer -> simulator -> dataserver
// reliable
{
	AvatarNotesUpdate Low 177 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data		Single
		{	TargetID		LLUUID		}
		{	Notes			Variable 2	}	// string
	}
}


// AvatarPicksReply
// dataserver -> simulator -> viewer
// Send the header information for this avatar's picks
// This fills in the tabs of the Picks panel.
// reliable
{
	AvatarPicksReply Low 178 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID		}
		{	TargetID		LLUUID		}
	}
	{
		Data			Variable
		{	PickID			LLUUID		}
		{	PickName		Variable 1	}	// string
	}
}


// EventInfoRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	EventInfoRequest Low 179 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		EventData		Single
		{	EventID		U32	}
	}
}


// EventInfoReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	EventInfoReply Low 180 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		EventData		Single
		{	EventID		U32				}
		{	Creator		Variable	1	}
		{	Name		Variable	1	}
		{	Category	Variable	1	}
		{	Desc		Variable	2	}
		{	Date		Variable	1	}
		{	DateUTC		U32				}
		{	Duration	U32				}
		{	Cover		U32				}
		{	Amount		U32				}
		{	SimName		Variable	1	}
		{	GlobalPos	LLVector3d		}
		{	EventFlags	U32				}
	}
}


// EventNotificationAddRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	EventNotificationAddRequest Low 181 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		EventData		Single
		{	EventID		U32		}
	}
}


// EventNotificationRemoveRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	EventNotificationRemoveRequest Low 182 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		EventData		Single
		{	EventID		U32		}
	}
}

// EventGodDelete
// viewer -> simulator
// simulator -> dataserver
// QueryData is used to resend a search result after the deletion
// reliable
{
	EventGodDelete Low 183 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		EventData		Single
		{	EventID			U32		}
	}
	{
		QueryData		Single
		{	QueryID		LLUUID		}
		{	QueryText	Variable 1	}
		{	QueryFlags	U32			}
		{	QueryStart	S32			}	// prev/next page support
	}
}


// PickInfoReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	PickInfoReply Low 184 Trusted Unencoded
    {
        AgentData    Single
        {    AgentID        LLUUID    }
    }
    {
		Data        Single
		{	PickID			LLUUID		}
		{	CreatorID		LLUUID		}
		{	TopPick			BOOL		}
		{	ParcelID		LLUUID		}
		{	Name			Variable 1	}
        {	Desc			Variable 2	}
        {	SnapshotID		LLUUID		}
        {	User			Variable 1	}
        {	OriginalName	Variable 1	}
        {	SimName			Variable 1	}
        {	PosGlobal		LLVector3d	}
        {	SortOrder		S32			}
        {	Enabled			BOOL		}
    }
}


// PickInfoUpdate
// Update a pick.  ParcelID is set on the simulator as the message
// passes through.
// If TopPick is TRUE, the simulator will only pass on the message
// if the agent_id is a god.
// viewer -> simulator -> dataserver
// reliable
{
	PickInfoUpdate Low 185 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data		Single
		{	PickID			LLUUID		}
		{	CreatorID		LLUUID		}
		{	TopPick			BOOL		}
		{	ParcelID		LLUUID		}
		{	Name			Variable 1	}
		{	Desc			Variable 2	}
		{	SnapshotID		LLUUID		}
		{	PosGlobal		LLVector3d	}
        {	SortOrder		S32			}
        {	Enabled			BOOL		}
	}
}


// PickDelete
// Delete a non-top pick from the database.
// viewer -> simulator -> dataserver
// reliable
{
	PickDelete Low 186 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data	Single
		{	PickID			LLUUID		}
	}
}

// PickGodDelete
// Delete a pick from the database.
// QueryID is needed so database can send a repeat list of 
// picks.
// viewer -> simulator -> dataserver
// reliable
{
	PickGodDelete Low 187 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID		}
		{	SessionID		LLUUID		}
	}
	{
		Data	Single
		{	PickID			LLUUID		}
		{	QueryID			LLUUID		}
	}
}


// ScriptQuestion
// reliable
{
	ScriptQuestion Low 188 Trusted Unencoded
	{
		Data	Single
		{	TaskID			LLUUID	}
		{	ItemID			LLUUID	}
		{	ObjectName		Variable	1	}
		{	ObjectOwner		Variable	1	}
		{	Questions		S32	}
	}
}

// ScriptControlChange
// reliable
{
	ScriptControlChange Low 189 Trusted Unencoded
	{
		Data	Variable
		{	TakeControls	BOOL 	}
		{	Controls		U32		}
		{	PassToAgent		BOOL	}
	}
}

// ScriptDialog
// sim -> viewer
// reliable
{
	ScriptDialog Low 190 Trusted Zerocoded
	{
		Data	Single
		{	ObjectID	LLUUID			}
		{	FirstName	Variable	1	}
		{	LastName	Variable	1	}
		{	ObjectName	Variable	1	}
		{	Message		Variable	2	}
		{	ChatChannel	S32				}
		{	ImageID		LLUUID			}
	}
	{
		Buttons	Variable
		{	ButtonLabel		Variable	1	}
	}
	{
		OwnerData Variable
		{	OwnerID		LLUUID	}
	}
}


// ScriptDialogReply
// viewer -> sim
// reliable
{
	ScriptDialogReply Low 191 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data	Single
		{	ObjectID	LLUUID			}
		{	ChatChannel	S32				}
		{	ButtonIndex	S32				}
		{	ButtonLabel	Variable 1		}
	}
}


// ForceScriptControlRelease
// reliable
{
	ForceScriptControlRelease Low 192 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}

// RevokePermissions
// reliable
{
	RevokePermissions Low 193 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data	Single
		{ ObjectID				LLUUID }
		{ ObjectPermissions		U32 }
	}
}

// LoadURL
// sim -> viewer
// Ask the user if they would like to load a URL
// reliable
{
	LoadURL Low 194 Trusted Unencoded
	{
		Data	Single
		{	ObjectName		Variable	1	}
		{	ObjectID		LLUUID			}
		{	OwnerID			LLUUID			}
		{	OwnerIsGroup	BOOL			}
		{	Message			Variable	1	}
		{	URL				Variable	1	}
	}
}

// ScriptTeleportRequest
// reliable
{
	ScriptTeleportRequest Low 195 Trusted Unencoded
	{
		Data	Single
		{	ObjectName		Variable	1	}
		{	SimName			Variable	1	}
		{	SimPosition		LLVector3		}
		{	LookAt			LLVector3		}
	}
}




// ***************************************************************************
// Land Parcel system
// ***************************************************************************

// ParcelOverlay
// We send N packets per region to the viewer.
// N = 4, currently.  At 256x256 meter regions, 4x4 meter parcel grid,
// there are 4096 parcel units per region.  At N = 4, that's 1024 units
// per packet, allowing 8 bit bytes.
// sim -> viewer
// reliable
{
	ParcelOverlay Low 196 Trusted Zerocoded
	{
		ParcelData		Single
		{	SequenceID	S32				}	// 0...3, which piece of region
		{	Data		Variable	2	}	// packed bit-field, (grids*grids)/N
	}
}


// ParcelPropertiesRequest
// SequenceID should be -1 or -2, and is echoed back in the
// parcel properties message.
// viewer -> sim
// reliable
{
	ParcelPropertiesRequest Medium 11 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ParcelData			Single
		{	SequenceID	S32	}
		{	West		F32	}
		{	South		F32	}
		{	East		F32	}
		{	North		F32	}
		{	SnapSelection	BOOL	}
	}
}

// ParcelPropertiesRequestByID
// viewer -> sim
// reliable
{
	ParcelPropertiesRequestByID Low 197 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ParcelData		Single
		{	SequenceID	S32		}
		{	LocalID		S32		}
	}
}

// ParcelProperties
// sequence id = -1 for parcels that you explicitly selected
// For agents, sequence id increments every time the agent transits into
// a new parcel.  It is used to detect out-of-order agent parcel info updates.
// Bitmap = packed bit field, one bit per parcel grid, on if that grid is
//        part of the selected parcel.
// sim -> viewer
// WARNING: This packet is potentially large.  With max length name,
// description, music URL and media URL, it is 1526 + sizeof ( LLUUID ) bytes.
{
	ParcelProperties High 23 Trusted Zerocoded
	{
		ParcelData			Single
		{	RequestResult	S32				}
		{	SequenceID		S32				}
		{	SnapSelection	BOOL			}
		{	SelfCount		S32				}
		{	OtherCount		S32				}
		{	PublicCount		S32				}
		{	LocalID			S32				}
		{	OwnerID			LLUUID			}
		{	IsGroupOwned	BOOL			}
		{	AuctionID		U32				}
		{	ClaimDate		S32				}	// time_t
		{	ClaimPrice		S32				}
		{	RentPrice		S32				}
		{	AABBMin			LLVector3		}
		{	AABBMax			LLVector3		}
		{	Bitmap			Variable	2	}	// packed bit-field
		{	Area			S32				}
		{	Status			U8	}  // owned vs. pending
		{	SimWideMaxPrims		S32			}
		{	SimWideTotalPrims	S32			}
		{	MaxPrims		S32				}
		{	TotalPrims		S32				}
		{	OwnerPrims		S32				}
		{	GroupPrims		S32				}
		{	OtherPrims		S32				}
		{	SelectedPrims	S32				}
		{	ParcelPrimBonus	F32				}

		{	OtherCleanTime	S32				}

		{	ParcelFlags		U32				}
		{	SalePrice		S32				}
		{	Name			Variable	1	}	// string
		{	Desc			Variable	1	}	// string
		{	MusicURL		Variable	1	}	// string
		{	MediaURL		Variable	1	}	// string
		{	MediaID			LLUUID			}
		{	MediaAutoScale	U8				}
		{	GroupID			LLUUID			}
		{	PassPrice		S32				}
		{	PassHours		F32				}
		{	Category		U8				}
		{	AuthBuyerID		LLUUID			}
		{	SnapshotID		LLUUID			}
		{	UserLocation	LLVector3		}
		{	UserLookAt		LLVector3		}
		{	LandingType		U8				}
		{	RegionPushOverride	BOOL		}
		{	RegionDenyAnonymous	BOOL		}
		{	RegionDenyIdentified	BOOL		}
		{	RegionDenyTransacted	BOOL		}
	}
	{
		AgeVerificationBlock Single
		{   RegionDenyAgeUnverified BOOL    }
	}
}

// ParcelPropertiesUpdate
// viewer -> sim
// reliable
{
	ParcelPropertiesUpdate Low 198 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
		{	Flags			U32				}

		{	ParcelFlags		U32				}
		{	SalePrice		S32				}
		{	Name			Variable	1	}	// string
		{	Desc			Variable	1	}	// string
		{	MusicURL		Variable	1	}	// string
		{	MediaURL		Variable	1	}	// string
		{	MediaID			LLUUID			}
		{	MediaAutoScale	U8				}
		{	GroupID			LLUUID			}
		{	PassPrice		S32				}
		{	PassHours		F32				}
		{	Category		U8				}
		{	AuthBuyerID		LLUUID			}
		{	SnapshotID		LLUUID			}
		{	UserLocation	LLVector3		}
		{	UserLookAt		LLVector3		}
		{	LandingType		U8				}
	}
}

// ParcelReturnObjects
// viewer -> sim
// reliable
{
	ParcelReturnObjects Low 199 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
		{	ReturnType		U32				}
	}
	{
		TaskIDs			Variable
		{	TaskID			LLUUID			}
	}
	{
		OwnerIDs			Variable
		{	OwnerID			LLUUID			}
	}
}

// ParcelSetOtherCleanTime
// viewer -> sim
// reliable
{
	ParcelSetOtherCleanTime Low 200 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
		{	OtherCleanTime	S32				}
	}
}


// Disable makes objects nonphysical and turns off their scripts.
// ParcelDisableObjects
// viewer -> sim
// reliable
{
	ParcelDisableObjects Low 201 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
		{	ReturnType		U32				}
	}
	{
		TaskIDs			Variable
		{	TaskID			LLUUID			}
	}
	{
		OwnerIDs			Variable
		{	OwnerID			LLUUID			}
	}
}


// ParcelSelectObjects
// viewer -> sim
// reliable
{
	ParcelSelectObjects Low 202 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
		{	ReturnType		U32				}
	}
	{
		ReturnIDs			Variable
		{	ReturnID		LLUUID	}
	}
}


// EstateCovenantRequest
// viewer -> sim
// reliable
{
    EstateCovenantRequest Low 203 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
}

// EstateCovenantReply
// sim -> viewer
// reliable
{
    EstateCovenantReply Low 204 Trusted Unencoded
	{
		Data        Single
		{	CovenantID		    LLUUID		    }
		{	CovenantTimestamp	U32 		    }
		{	EstateName		    Variable    1	}   // string
        {   EstateOwnerID       LLUUID          }
    }
}


// ForceObjectSelect
// sim -> viewer
// reliable
{
	ForceObjectSelect Low 205 Trusted Unencoded
	{
		Header				Single
		{	ResetList		BOOL			}
	}
	{
		Data				Variable
		{	LocalID			U32				}
	}
}


// ParcelBuyPass - purchase a temporary access pass
// viewer -> sim
// reliable
{
	ParcelBuyPass	Low	206 	NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	LocalID			S32				}
	}
}

// ParcelDeedToGroup - deed a patch of land to a group
// viewer -> sim
// reliable
{
	ParcelDeedToGroup Low 207 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data				Single
		{	GroupID			LLUUID	}
		{	LocalID			S32		}	// parcel id
	}
}

// reserved for when island owners force re-claim parcel
{
	ParcelReclaim Low 208 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data				Single
		{	LocalID			S32		}	// parcel id
	}
}

// ParcelClaim - change the owner of a patch of land
// viewer -> sim
// reliable
{
	ParcelClaim Low 209 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data				Single
		{	GroupID			LLUUID	}
		{	IsGroupOwned	BOOL 	}
		{	Final			BOOL	} // true if buyer is in tier
	}
	{
		ParcelData			Variable
		{	West			F32		}
		{	South			F32		}
		{	East			F32		}
		{	North			F32		}
	}
}

// ParcelJoin - Take all parcels which are owned by agent and inside
// rectangle, and make them 1 parcel if they all are leased.
// viewer -> sim
// reliable
{
	ParcelJoin Low 210 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	West		F32		}
		{	South		F32		}
		{	East		F32		}
		{	North		F32		}
	}
}

// ParcelDivide
// If the selection is a subsection of exactly one parcel,
// chop out that section and make a new parcel of it.
// viewer -> sim
// reliable
{
	ParcelDivide Low 211 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ParcelData			Single
		{	West		F32		}
		{	South		F32		}
		{	East		F32		}
		{	North		F32		}
	}
}

// ParcelRelease
// Release a parcel to public
// viewer -> sim
// reliable
{
	ParcelRelease Low 212 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data			Single
		{	LocalID			S32		}	// parcel ID
	}
}

// ParcelBuy - change the owner of a patch of land.
// viewer -> sim
// reliable
{
	ParcelBuy Low 213 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data			Single
		{	GroupID			LLUUID	}
		{	IsGroupOwned	BOOL 	}
		{	RemoveContribution BOOL	}
		{	LocalID			S32		}
		{	Final			BOOL	} // true if buyer is in tier
	}
	{
		ParcelData		Single
		{	Price			S32		}
		{	Area			S32		}
	}
}


// ParcelGodForceOwner Unencoded
{
	ParcelGodForceOwner	Low	214 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data			Single
		{	OwnerID			LLUUID	}
		{	LocalID			S32		}	// parcel ID
	}
}


// viewer -> sim
// ParcelAccessListRequest
{
	ParcelAccessListRequest Low 215 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data	Single
		{	SequenceID		S32		}
		{	Flags			U32		}
		{	LocalID			S32		}
	}
}


// sim -> viewer
// ParcelAccessListReply
{
	ParcelAccessListReply Low 216 Trusted Zerocoded
	{
		Data	Single
		{	AgentID			LLUUID	}
		{	SequenceID		S32		}
		{	Flags			U32		}
		{	LocalID			S32		}
	}
	{
		List	Variable
		{	ID			LLUUID		}
		{	Time		S32			} // time_t
		{	Flags		U32			}
	}
}

// viewer -> sim
// ParcelAccessListUpdate
{
	ParcelAccessListUpdate Low 217 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data	Single
		{	Flags			U32		}
		{	LocalID			S32		}
		{	TransactionID	LLUUID	}
		{	SequenceID		S32		}
		{	Sections		S32		}
	}
	{
		List	Variable
		{	ID			LLUUID		}
		{	Time		S32			} // time_t
		{	Flags		U32			}
	}
}


// viewer -> sim -> dataserver
// reliable
{
	ParcelDwellRequest Low 218 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		Data		Single
		{	LocalID		S32			}
		{	ParcelID	LLUUID		}	// filled in on sim
	}
}


// dataserver -> sim -> viewer
// reliable
{
	ParcelDwellReply Low 219 Trusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID		}
	}
	{
		Data		Single
		{	LocalID		S32			}
		{	ParcelID	LLUUID		}
		{	Dwell		F32			}
	}
}

// sim -> dataserver
// This message is used to check if a user can buy a parcel. If
// successful, the transaction is approved through a money balance reply
// with the same transaction id. 
{
	RequestParcelTransfer	Low	220 Trusted Zerocoded
	{
		Data	Single
		{	TransactionID	LLUUID	}
		{ 	TransactionTime	U32	} // utc seconds since epoch
		{	SourceID		LLUUID  }
		{	DestID			LLUUID	}
		{	OwnerID			LLUUID	}
		{	Flags			U8		}	// see lltransactiontypes.h
		{	TransactionType	S32		}	// see lltransactiontypes.h
		{	Amount			S32		}
		{	BillableArea	S32		}
		{	ActualArea		S32		}
		{	Final			BOOL	}  // true if buyer should be in tier
	}
    {
       RegionData Single  // included so region name shows up in transaction logs
       {   RegionID   LLUUID }
       {   GridX      U32    }
       {   GridY      U32    }
    }
}

// sim ->dataserver
// This message is used to send up complete parcel properties for
// persistance in the database.
// If you add something here, you should probably also change the
// simulator's database update query on startup.
{
	UpdateParcel Low 221 Trusted Zerocoded
	{
		ParcelData 		Single
		{	ParcelID		LLUUID		}
		{	RegionHandle	U64			}
		{	OwnerID			LLUUID		}
		{	GroupOwned		BOOL		}
		{	Status			U8			}
		{	Name			Variable 1	}
		{	Description		Variable 1	}
		{	MusicURL		Variable 1	}
		{	RegionX			F32			}
		{	RegionY			F32			}
		{	ActualArea		S32			}
		{	BillableArea	S32			}
		{	ShowDir			BOOL		}
		{	IsForSale		BOOL		}
		{	Category		U8			}
		{	SnapshotID		LLUUID		}
		{	UserLocation	LLVector3	}
		{	SalePrice		S32			}
		{	AuthorizedBuyerID	LLUUID		}
		{	AllowPublish	BOOL		}
		{	MaturePublish	BOOL		}
	}
}

// sim -> dataserver or space ->sim
// This message is used to tell the dataserver that a parcel has been
// removed.
{
	RemoveParcel Low 222 Trusted Unencoded
	{
		ParcelData 		Variable
		{	ParcelID	LLUUID	}
	}
}

// sim -> dataserver
// Merges some of the database information for parcels (dwell).
{
	MergeParcel Low 223 Trusted Unencoded
	{
		MasterParcelData	Single
		{	MasterID	LLUUID	}
	}
	{
		SlaveParcelData 	Variable
		{	SlaveID		LLUUID	}
	}
}

// sim -> dataserver
{
	LogParcelChanges	Low	224 Trusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		RegionData		Single
		{	RegionHandle	U64		}
	}
	{
		ParcelData		Variable
		{	ParcelID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	IsOwnerGroup	BOOL	}
		{	ActualArea		S32		}
		{	Action			S8		}
		{	TransactionID	LLUUID	}
	}
}

// sim -> dataserver
{
	CheckParcelSales	Low	225 Trusted Unencoded
	{
		RegionData 		Variable
		{	RegionHandle	U64	}
	}
}

// dataserver -> simulator
// tell a particular simulator to finish parcel sale.
{
	ParcelSales	Low	226 Trusted Unencoded
	{
		ParcelData 		Variable
		{	ParcelID		LLUUID	}
		{	BuyerID			LLUUID	}
	}
}

// viewer -> sim
// mark parcel and double secret agent content on parcel as owned by
// governor/maint and adjusts permissions approriately. Godlike request.
{
	ParcelGodMarkAsContent Low 227 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ParcelData	Single
		{	LocalID		S32		}
	}
}


// viewer -> sim
// start an auction. viewer fills in the appropriate date, simulator
// validates and fills in the rest of the information to start an auction
// on a parcel. Processing currently requires that AgentID is a god.
{
	ViewerStartAuction	Low 228 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ParcelData	Single
		{	LocalID		S32		}
		{	SnapshotID	LLUUID	}
	}
}

// sim -> dataserver
// Once all of the data has been gathered, 
{
	StartAuction	Low 229 Trusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
	{
		ParcelData	Single
		{	ParcelID	LLUUID	}
		{	SnapshotID	LLUUID	}
		{	Name		Variable	1	}	// string
	}
}

// dataserver -> sim
{
	ConfirmAuctionStart	Low	230 Trusted	Unencoded
	{
		AuctionData	Single
		{	ParcelID	LLUUID	}
		{	AuctionID	U32		}
	}
}

// sim -> dataserver
// Tell the dataserver that an auction has completed.
{
	CompleteAuction	Low	231 Trusted	Unencoded
	{
		ParcelData	Variable
		{	ParcelID	LLUUID	}
	}
}

// Tell the dataserver that an auction has been canceled.
{
	CancelAuction	Low	232 Trusted	Unencoded
	{
		ParcelData	Variable
		{	ParcelID	LLUUID	}
	}
}

// sim -> dataserver
{
	CheckParcelAuctions	Low	233 Trusted Unencoded
	{
		RegionData 		Variable
		{	RegionHandle	U64	}
	}
}

// dataserver -> sim
// tell a particular simulator to finish parcel sale.
{
	ParcelAuctions	Low	234 Trusted Unencoded
	{
		ParcelData 		Variable
		{	ParcelID		LLUUID	}
		{	WinnerID		LLUUID	}
	}
}

// ***************************************************************************
// UUID to name lookup
// ***************************************************************************

// UUIDNameRequest
// Translate a UUID into first and last names
{
	UUIDNameRequest Low 235 NotTrusted Unencoded
	{
		UUIDNameBlock	Variable
		{	ID			LLUUID	}
	}
}

// UUIDNameReply
// Translate a UUID into first and last names
{
	UUIDNameReply Low 236 Trusted Unencoded
	{
		UUIDNameBlock	Variable
		{	ID			LLUUID	}
		{	FirstName	Variable	1	}
		{	LastName	Variable	1	}
	}
}

// UUIDGroupNameRequest
// Translate a UUID into a group name
{
	UUIDGroupNameRequest Low 237 NotTrusted Unencoded
	{
		UUIDNameBlock	Variable
		{	ID			LLUUID	}
	}
}

// UUIDGroupNameReply
// Translate a UUID into a group name
{
	UUIDGroupNameReply Low 238 Trusted Unencoded
	{ 
		UUIDNameBlock	Variable
		{	ID			LLUUID	}
		{	GroupName	Variable	1	}
	}
}

// end uuid to name lookup

// ***************************************************************************
// Simulator to Simulator Messages
// ***************************************************************************

// ChatPass
// Chat message transmission to neighbors
// Chat is region local to receiving simulator.
// Type is one of CHAT_TYPE_NORMAL, _WHISPER, _SHOUT
{
	ChatPass Low 239 Trusted Zerocoded
	{
		ChatData			Single
		{	Channel			S32	}
		{	Position		LLVector3	}
		{	ID				LLUUID	}
		{	OwnerID			LLUUID	}
		{	Name			Variable	1	}
		{	SourceType		U8	}
		{	Type			U8	}
		{	Radius			F32 }
		{	SimAccess		U8	}
		{	Message			Variable	2	}
	}
}

// Edge data - compressed edge data

{
	EdgeDataPacket High 24 Trusted Zerocoded
	{
		EdgeData			Single
		{	LayerType		U8	}
		{	Direction		U8	}
		{	LayerData		Variable	2	}
	}
}

// Sim status, condition of this sim
// sent reliably, when dirty
{
	SimStatus Medium 12 Trusted Unencoded
	{
		SimStatus				Single
		{	CanAcceptAgents		BOOL	}
		{	CanAcceptTasks		BOOL	}
	}
}

// Child Agent Update - agents send child agents to neighboring simulators.
// This will create a child camera if there isn't one at the target already
// Can't send viewer IP and port between simulators -- the port may get remapped
// if the viewer is behind a Network Address Translation (NAT) box.
//
// Note: some of the fields of this message really only need to be sent when an 
// agent crosses a region boundary and changes from a child to a main agent
// (such as Head/BodyRotation, ControlFlags, Animations etc)
// simulator -> simulator
// reliable
{
	ChildAgentUpdate High 25 Trusted Zerocoded
	{
		AgentData				Single
		
		{	RegionHandle		U64	}
		{	ViewerCircuitCode	U32	}
		{	AgentID							LLUUID	}
		{	SessionID						LLUUID	}
		
		{	AgentPos			LLVector3	}
		{	AgentVel			LLVector3	}
		{	Center				LLVector3	}
		{ 	Size 				LLVector3 	}
		{	AtAxis				LLVector3	}
		{	LeftAxis			LLVector3	}
		{	UpAxis				LLVector3	}
		{	ChangedGrid			BOOL	}	// BOOL
		 
		{	Far					F32	}
		{	Aspect				F32	}
		{	Throttles			Variable 1	}
 		{	LocomotionState		U32	}
		{	HeadRotation		LLQuaternion	}
		{	BodyRotation		LLQuaternion	}
		{	ControlFlags		U32	}
		{	EnergyLevel			F32	}
		{	GodLevel			U8	}	// Changed from BOOL to U8, and renamed GodLevel (from Godlike)
		{   AlwaysRun			BOOL	}
		{	PreyAgent						LLUUID	}
		{	AgentAccess		U8	}
		{	AgentTextures		Variable	2	}
		{	ActiveGroupID	        LLUUID	}	
	}
	{
		GroupData			Variable
		{	GroupID				LLUUID	}
		{	GroupPowers			U64		}
		{	AcceptNotices		BOOL	}
	}
	{	
		AnimationData			Variable
		{	Animation			LLUUID	}
		{	ObjectID			LLUUID	}
	}
	{
		GranterBlock			Variable
		{	GranterID						LLUUID	}
	}
	{
		NVPairData				Variable
		{	NVPairs				Variable	2	}
	}
	{
		VisualParam				Variable
		{	ParamValue			U8	}
	}
	{
		AgentAccess				Variable
		{	AgentLegacyAccess		U8	}
		{	AgentMaxAccess			U8	}
	}
	{
		AgentInfo				Variable
		{	Flags					U32	}
	}
}

// ChildAgentAlive
// sent to child agents just to keep them alive
{
	ChildAgentAlive High 26 Trusted Unencoded
	{
		AgentData				Single
		{	RegionHandle		U64	}
		{	ViewerCircuitCode	U32	}
		{	AgentID				LLUUID	}
		{	SessionID			LLUUID	}
	}
}

// ChildAgentPositionUpdate
// sent to child agents just to keep them alive
{
	ChildAgentPositionUpdate High 27 Trusted Unencoded
	{
		AgentData				Single

		{	RegionHandle		U64	}
		{	ViewerCircuitCode	U32	}
		{	AgentID				LLUUID	}
		{	SessionID			LLUUID	}

		{	AgentPos			LLVector3	}
		{	AgentVel			LLVector3	}
		{	Center				LLVector3	}
		{ 	Size 				LLVector3 	}
		{	AtAxis				LLVector3	}
		{	LeftAxis			LLVector3	}
		{	UpAxis				LLVector3	}
		{	ChangedGrid			BOOL	}
	}
}


// Obituary for child agents - make sure the parent know the child is dead
// This way, children can be reliably restarted
{
	ChildAgentDying Low 240 Trusted Zerocoded
	{
		AgentData				Single
		{	AgentID				LLUUID	}
		{	SessionID			LLUUID	}
	}
}


// This is sent if a full child agent hasn't been accepted yet
{
	ChildAgentUnknown Low 241 Trusted Unencoded
	{
		AgentData				Single
		{	AgentID				LLUUID	}
		{	SessionID			LLUUID	}
	}
}


// This message is sent how objects get passed between regions.
{
    AtomicPassObject High 28 Trusted Unencoded
    {
		TaskData		Single
		{	TaskID				LLUUID	}
		{	AttachmentNeedsSave	BOOL	}	// true iff is attachment and needs asset saved
	}
}


// KillChildAgents - A new agent has connected to the simulator . . . make sure that any old child cameras are blitzed
{
	KillChildAgents Low 242 Trusted Unencoded
	{
		IDBlock				Single
		{	AgentID				LLUUID	}
	}
}


// GetScriptRunning - asks if a script is running or not. the simulator
// responds with ScriptRunningReply
{
	GetScriptRunning	Low	243 		NotTrusted Unencoded
	{
		Script			Single
		{	ObjectID	LLUUID	}
		{	ItemID	LLUUID	}
	}
}

// ScriptRunningReply - response from simulator to message above
{
	ScriptRunningReply		Low	244 		NotTrusted Unencoded UDPDeprecated
	{
		Script				Single
		{	ObjectID		LLUUID	}
		{	ItemID			LLUUID	}
		{	Running			BOOL	}
//		{	Mono			BOOL	} Added to LLSD message
	}
}


// SetScriptRunning - makes a script active or inactive (Enable may be
// true or false)
{
	SetScriptRunning	Low	245  	NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Script			Single
		{	ObjectID	LLUUID	}
		{	ItemID		LLUUID	}
		{	Running		BOOL	}
	}
}

// ScriptReset - causes a script to reset
{
	ScriptReset Low 246 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Script				Single
		{	ObjectID		LLUUID	}
		{	ItemID			LLUUID	}
	}
}

// ScriptSensorRequest - causes the receiving sim to run a script sensor and return the results
{
	ScriptSensorRequest Low 247 Trusted Zerocoded
	{
		Requester			Single
		{	SourceID		LLUUID	}
		{	RequestID		LLUUID	}
		{	SearchID		LLUUID	}
		{	SearchPos		LLVector3	}
		{	SearchDir		LLQuaternion	}
		{	SearchName		Variable	1	}
		{	Type			S32	}
		{	Range			F32	}
		{	Arc				F32	}
		{	RegionHandle	U64	}
		{	SearchRegions	U8	}
	}
}

// ScriptSensorReply - returns the request script search information back to the requester
{
	ScriptSensorReply Low 248 Trusted Zerocoded
	{
		Requester			Single
		{	SourceID		LLUUID	}
	}
	{
		SensedData			Variable
		{	ObjectID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	GroupID			LLUUID	}
		{	Position		LLVector3	}
		{	Velocity		LLVector3	}
		{	Rotation		LLQuaternion	}
		{	Name			Variable	1	}
		{	Type			S32	}
		{	Range			F32	}
	}
}

//-----------------------------------------------------------------------------
// Login and Agent Motion
//-----------------------------------------------------------------------------

// viewer -> sim
// agent is coming into the region. The region should be expecting the
// agent.
{
	CompleteAgentMovement Low 249 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	CircuitCode		U32	}
	}
}

// sim -> viewer
{
	AgentMovementComplete Low 250 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		Data			Single
		{	Position		LLVector3 }
		{	LookAt			LLVector3 }
		{	RegionHandle	U64		}
		{	Timestamp		U32	}
	}
	{
		SimData			Single
		{	ChannelVersion	Variable 2      }
	}
}


//-----------------------------------------------------------------------------
// Logout
//-----------------------------------------------------------------------------

// userserver -> dataserver
{
	DataServerLogout Low 251 Trusted Unencoded
	{
		UserData			Single
		{	AgentID			LLUUID	}
		{	ViewerIP		IPADDR	}
		{	Disconnect		BOOL	}
		{	SessionID		LLUUID	}
	}
}

// LogoutRequest
// viewer -> sim
// reliable
{
	LogoutRequest Low 252 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}


// LogoutReply
// it's ok for the viewer to quit.
// sim -> viewer
// reliable
// Includes inventory items to update with new asset ids
{
	LogoutReply Low 253 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}  // null if list is actually empty (but has one entry 'cause it can't have none)
	}
}


//-----------------------------------------------------------------------------
// Instant Message
//-----------------------------------------------------------------------------

// ImprovedInstantMessage
// This message can potentially route all over the place
// ParentEstateID: parent estate id of the source estate
// RegionID: region id of the source of the IM.
// Position: position of the sender in region local coordinates
// Dialog	see llinstantmessage.h for values
// ID		May be used by dialog. Interpretation depends on context.
// BinaryBucket May be used by some dialog types
// reliable
{
	ImprovedInstantMessage Low 254 NotTrusted Zerocoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  }
		{	SessionID	LLUUID	}
	}
	{
		MessageBlock		Single
		{	FromGroup		BOOL	}
		{	ToAgentID		LLUUID	}
		{	ParentEstateID	U32	}
		{   RegionID		LLUUID	}
		{	Position		LLVector3	}
		{	Offline			U8	}
		{	Dialog			U8	}	// U8 - IM type
		{	ID				LLUUID	}
		{	Timestamp		U32	}
		{	FromAgentName	Variable	1	}
		{	Message			Variable	2	}
		{	BinaryBucket	Variable	2	}
	}
}

// RetrieveInstantMessages - used to get instant messages that
// were persisted out to the database while the user was offline
{
	RetrieveInstantMessages Low 255 NotTrusted Unencoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  }
		{	SessionID	LLUUID	}
	}
}

// FindAgent - used to find an agent's global position. I used a
// variable sized LocationBlock so that the message can be recycled with
// minimum new messages and handlers.
{
	FindAgent Low 256 NotTrusted Unencoded
	{
		AgentBlock		Single
		{	Hunter		LLUUID	}
		{	Prey		LLUUID	}
		{	SpaceIP		IPADDR	}
	}
	{
		LocationBlock	Variable
		{	GlobalX		F64	}
		{	GlobalY		F64	}
	}
}

// Set godlike to 1 if you want to become godlike.
// Set godlike to 0 if you want to relinquish god powers.
// viewer -> simulator -> dataserver
// reliable
{
	RequestGodlikePowers Low 257 NotTrusted Unencoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  }
		{	SessionID	LLUUID	}
	}
	{
		RequestBlock	Single
		{	Godlike		BOOL	}
		{	Token		LLUUID	} // viewer packs a null, sim packs token
	}
}

// At the simulator, turn the godlike bit on.
// At the viewer, show the god menu.
// dataserver -> simulator -> viewer
// reliable
{
	GrantGodlikePowers	Low	258 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		GrantData		Single
		{	GodLevel	U8		}
		{	Token		LLUUID	} // checked on sim, ignored on viewer
	}
}

// GodlikeMessage - generalized construct for Gods to send messages
// around the system. Each Request has it's own internal protocol.
{
    GodlikeMessage Low 259 NotTrusted Zerocoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  }
		{	SessionID	LLUUID		}
		{	TransactionID	LLUUID	}
	}
	{
		MethodData 	Single
		{	Method		Variable 1 }
		{	Invoice		LLUUID	}
	}
	{
		ParamList		Variable
		{	Parameter	Variable 1 }
	}
}

// EstateOwnerMessage
// format must be identical to above
{
    EstateOwnerMessage	Low 260 NotTrusted Zerocoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  	}
		{	SessionID	LLUUID		}
		{	TransactionID	LLUUID	}
	}
	{
		MethodData 	Single
		{	Method		Variable 1 }
		{	Invoice		LLUUID	}
	}
	{
		ParamList		Variable
		{	Parameter	Variable 1 }
	}
}

// GenericMessage
// format must be identical to above
// As above, but don't have to be god or estate owner to send.
{
    GenericMessage	Low 261 NotTrusted Zerocoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  	}
		{	SessionID	LLUUID		}
		{	TransactionID	LLUUID	}
	}
	{
		MethodData 	Single
		{	Method		Variable 1 }
		{	Invoice		LLUUID	}
	}
	{
		ParamList		Variable
		{	Parameter	Variable 1 }
	}
}

// ***************************************************************************
// Requests for possessions, acquisition, money, etc
// ***************************************************************************

// request for mute list
{
	MuteListRequest Low 262 NotTrusted Unencoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  	}
		{	SessionID	LLUUID		}
	}
	{
		MuteData	Single
		{	MuteCRC		U32		}
	}
}

// update/add someone in the mute list
{
	UpdateMuteListEntry Low 263 NotTrusted Unencoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  	}
		{	SessionID	LLUUID		}
	}
	{
		MuteData	Single
		{	MuteID		LLUUID		}
		{	MuteName	Variable 1	}
		{	MuteType	S32			}
		{	MuteFlags	U32			}
	}
}

// Remove a mute list entry.
{
	RemoveMuteListEntry Low 264 NotTrusted Unencoded
	{
		AgentData 		Single
		{   AgentID     LLUUID  	}
		{	SessionID	LLUUID		}
	}
	{
		MuteData	Single
		{	MuteID		LLUUID		}
		{	MuteName	Variable 1	}
	}
}


// 
// Inventory update messages 
// UDP DEPRECATED - Now a viewer capability.

{
	CopyInventoryFromNotecard Low 265 NotTrusted Zerocoded UDPDeprecated
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		NotecardData		Single
		{	NotecardItemID	LLUUID	}
		{	ObjectID		LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
	}
}

//
// This is used bi-directionally between sim, dataserver, and viewer.
// THIS MESSAGE CAN NOT CREATE NEW INVENTORY ITEMS.
//
{
	UpdateInventoryItem Low 266 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	TransactionID	LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	CallbackID		U32		} // Async Response
		
		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions

		{	TransactionID	LLUUID	} // TransactionID: new assets only
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

//
// For sim to request update/create.
// DO NOT ALLOW THIS FROM THE VIEWER.
//
{
	UpdateCreateInventoryItem Low 267 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SimApproved		BOOL	}
		{	TransactionID	LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	CallbackID		U32		} // Async Response

		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions

		{	AssetID			LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

{
	MoveInventoryItem	Low	268 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Stamp		BOOL	} // should the server re-timestamp?
	}
	{
		InventoryData	Variable
		{	ItemID		LLUUID	}
		{	FolderID	LLUUID	}
		{	NewName			Variable	1	}
	}
}

// copy inventory item by item id to specified destination folder, 
// send out bulk inventory update when done.
//
// Inventory items are only unique for {agent, inv_id} pairs;
// the OldItemID needs to be paired with the OldAgentID to
// produce a unique inventory item.
{
	CopyInventoryItem	Low	269 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData		Variable
		{	CallbackID	U32			} // Async response
		{	OldAgentID		LLUUID	}
		{	OldItemID		LLUUID	}
		{	NewFolderID		LLUUID	}
		{	NewName			Variable	1	}
	}
}

{
	RemoveInventoryItem Low 270 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData	Variable
		{	ItemID		LLUUID	}
	}
}

{
	ChangeInventoryItemFlags Low 271 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData	Variable
		{	ItemID		LLUUID	}
		{	Flags		U32	}
	}
}

//
// Sim outgoing only (to dataserver, to viewer)
// NOT viewer to sim, sim should not have handler, ever
// This message is currently only uses objects, so the viewer ignores
// the asset id.
{
	SaveAssetIntoInventory Low 272 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
	}
	{
		InventoryData		Single
		{	ItemID			LLUUID	}
		{	NewAssetID		LLUUID	}
	}
}

{
	CreateInventoryFolder	Low 273 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		FolderData			Single
		{	FolderID		LLUUID	}
		{	ParentID		LLUUID	}
		{	Type			S8	}
		{	Name			Variable	1	}
	}
}

{
	UpdateInventoryFolder Low 274 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		FolderData			Variable
		{	FolderID		LLUUID	}
		{	ParentID		LLUUID	}
		{	Type			S8	}
		{	Name			Variable	1	}
	}
}

{
	MoveInventoryFolder	Low	275 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Stamp		BOOL	} // should the server re-timestamp children
	}
	{
		InventoryData	Variable
		{	FolderID	LLUUID	}
		{	ParentID	LLUUID	}
	}
}

{
	RemoveInventoryFolder Low 276 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		FolderData			Variable
		{	FolderID		LLUUID	}
	}
}

// Get inventory segment.
{
	FetchInventoryDescendents Low 277 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData	Single
		{	FolderID	LLUUID	}
		{	OwnerID		LLUUID	}
		{	SortOrder	S32		} // 0 = name, 1 = time
		{	FetchFolders	BOOL	} // false will omit folders in query
		{	FetchItems		BOOL	} // false will omit items in query
	}
}

// return inventory segment. 
// *NOTE: This could be compressed more since we already know the
// parent_id for folders and the folder_id for items, but this is
// reasonable until we heve server side inventory.
{
	InventoryDescendents Low 278 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	FolderID		LLUUID	}
		{	OwnerID			LLUUID	} // owner of the folders creatd.
		{	Version			S32		} // version of the folder for caching
		{	Descendents		S32		} // count to help with caching
	}
	{
		FolderData		Variable
		{	FolderID		LLUUID	}
		{	ParentID		LLUUID	}
		{	Type			S8	}
		{	Name			Variable	1	}
	}
	{
		ItemData		Variable
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions
		{	AssetID			LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

// Get inventory item(s) - response comes through FetchInventoryReply
{
	FetchInventory	Low	279 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData	Variable
		{	OwnerID		LLUUID	}
		{	ItemID		LLUUID	}
	}
}

// response to fetch inventory
{
	FetchInventoryReply	Low	280 Trusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}

		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions

		{	AssetID			LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

// Can only fit around 7 items per packet - that's the way it goes. At
// least many bulk updates can be packed.
// Only from dataserver->sim->viewer
{
	BulkUpdateInventory Low 281 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	TransactionID	LLUUID	}
	}
	{
		FolderData		Variable
		{	FolderID		LLUUID	}
		{	ParentID		LLUUID	}
		{	Type			S8	}
		{	Name			Variable	1	}
	}
	{
		ItemData		Variable
		{	ItemID			LLUUID	}
		{	CallbackID		U32		}	// Async Response
		{	FolderID		LLUUID	}
		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions
		{	AssetID			LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}



// request permissions for agent id to get the asset for owner_id's
// item_id.
{
	RequestInventoryAsset	Low	282 Trusted Unencoded
	{
		QueryData	Single
		{ QueryID		LLUUID	}
		{ AgentID		LLUUID	}
		{ OwnerID		LLUUID	}
		{ ItemID		LLUUID	}
	}
}

// response to RequestInventoryAsset
// lluuid will be null if agentid in the request above cannot read asset
{
	InventoryAssetResponse	Low	283 Trusted Unencoded
	{
		QueryData	Single
		{ QueryID		LLUUID	}
		{ AssetID		LLUUID	}
		{ IsReadable	BOOL	}
	}
}

// This is the new improved way to remove inventory items.  It is
// currently only supported in viewer->userserver->dataserver
// messages typically initiated by an empty trash method.
{
	RemoveInventoryObjects	Low	284 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		FolderData			Variable
		{	FolderID		LLUUID	}
	}
	{
		ItemData			Variable
		{	ItemID			LLUUID	}
	}
}

// This is how you remove inventory when you're not even sure what it
// is - only it's parenting.
{
	PurgeInventoryDescendents Low 285 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryData	Single
		{	FolderID	LLUUID	}
	}
}

// These messages are viewer->simulator requests to update a task's
// inventory.
// if Key == 0, itemid is the key. if Key == 1, assetid is the key.
{
	UpdateTaskInventory Low 286 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID  }
	}
	{
		UpdateData			Single
		{	LocalID			U32	}
		{	Key				U8	}
	}
	{
		InventoryData		Single
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions
		{	TransactionID	LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

{
	RemoveTaskInventory Low 287 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID  }
	}
	{
		InventoryData		Single
		{	LocalID			U32	}
		{	ItemID			LLUUID	}
	}
}

{
	MoveTaskInventory	Low	288 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID  }
		{	FolderID	LLUUID	}
	}
	{
		InventoryData	Single
		{	LocalID		U32		}
		{	ItemID		LLUUID	}
	}
}

{
	RequestTaskInventory Low 289 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID  }
	}
	{
		InventoryData		Single
		{	LocalID		U32	}
	}
}

{
	ReplyTaskInventory Low 290 Trusted Zerocoded
	{
		InventoryData		Single
		{	TaskID			LLUUID	}
		{	Serial			S16	}	// S16
		{	Filename		Variable	1	}
	}
}

// These messages are viewer->simulator requests regarding objects
// which are currently being simulated. The viewer will get an 
// UpdateInventoryItem response if a DeRez succeeds, and the object
// will appear if a RezObject succeeds.
// The Destination field tells where the derez should wind up, and the
// meaning of DestinationID depends on it. For example, if the 
// destination is a category, then the destination is the category id. If
// the destination is a task inventory, then the destination id is the
// task id.
// The transaction id is generated by the viewer on derez, and then
// the packets are counted and numbered. The rest of the information is
// just duplicated (it's not that much, and derezzes that span multiple
// packets will be rare.)
{
	DeRezObject Low 291 NotTrusted Zerocoded
	{
		AgentData		Single
		{  AgentID			LLUUID  }
		{  SessionID		LLUUID  }
	}
	{
		AgentBlock		Single
		{  GroupID			LLUUID  }
		{  Destination		U8	}
		{  DestinationID	LLUUID	}	// see above
		{  TransactionID	LLUUID	}
		{  PacketCount		U8	}
		{  PacketNumber		U8	}
	}
	{
		ObjectData		Variable
		{  ObjectLocalID	U32	}   // object id in world 
	}
}

// This message is sent when a derez succeeds, but there's no way to
// know, since no inventory is created on the viewer. For example, when
// saving into task inventory.
{
	DeRezAck Low 292 Trusted Unencoded
	{
		TransactionData			Single
		{	TransactionID	LLUUID	}
		{	Success			BOOL	}
	}
}

// This message is sent from viewer -> simulator when the viewer wants
// to rez an object out of inventory.
{
	RezObject Low 293 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
		{	GroupID		LLUUID	}
	}
	{
		RezData			Single
		{	FromTaskID				LLUUID	}
		{	BypassRaycast			U8	}
		{	RayStart				LLVector3	}
		{	RayEnd					LLVector3	}
		{	RayTargetID				LLUUID	}
		{	RayEndIsIntersection	BOOL }
		{   RezSelected				BOOL }
		{	RemoveItem				BOOL }
		{	ItemFlags				U32 }
		{	GroupMask				U32 }
		{	EveryoneMask			U32 }
		{	NextOwnerMask			U32	}
	}
	{
		InventoryData			Single
		{	ItemID				LLUUID	}
		{	FolderID			LLUUID	}
		{	CreatorID			LLUUID	}	// permissions
		{	OwnerID				LLUUID	}	// permissions
		{	GroupID				LLUUID	}	// permissions
		{	BaseMask			U32	}	// permissions
		{	OwnerMask			U32	}	// permissions
		{	GroupMask			U32	}	// permissions
		{	EveryoneMask		U32	}	// permissions
		{	NextOwnerMask		U32	}	// permissions
		{	GroupOwned			BOOL	}	// permissions
		{	TransactionID		LLUUID	}
		{	Type				S8	}
		{	InvType				S8	}
		{	Flags				U32	}
		{	SaleType			U8	}
		{	SalePrice			S32	}
		{	Name				Variable	1	}
		{	Description			Variable	1	}
		{	CreationDate		S32	}
		{	CRC				U32	}
	}
}

// This message is sent from viewer -> simulator when the viewer wants
// to rez an object from a notecard.
{
	RezObjectFromNotecard Low 294 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
		{	GroupID		LLUUID	}
	}
	{
		RezData			Single
		{	FromTaskID				LLUUID	}
		{	BypassRaycast			U8	}
		{	RayStart				LLVector3	}
		{	RayEnd					LLVector3	}
		{	RayTargetID				LLUUID	}
		{	RayEndIsIntersection	BOOL }
		{   RezSelected				BOOL }
		{	RemoveItem				BOOL }
		{	ItemFlags				U32 }
		{	GroupMask				U32 }
		{	EveryoneMask			U32 }
		{	NextOwnerMask			U32	}
	}
	{
		NotecardData		Single
		{	NotecardItemID	LLUUID	}
		{	ObjectID		LLUUID	}
	}
	{
		InventoryData		Variable
		{	ItemID			LLUUID	}
	}
}


// sim -> dataserver
// sent during agent to agent inventory transfers
{
	TransferInventory Low 295 Trusted Zerocoded
	{
		InfoBlock			Single
		{	SourceID		LLUUID	}
		{	DestID			LLUUID	}
		{	TransactionID	LLUUID	}
	}
	{
		InventoryBlock		Variable
		{	InventoryID		LLUUID	}
		{	Type			S8	}
	}
}

// dataserver -> sim
// InventoryID is the id of the inventory object that the end user
// should discard if they deny the transfer.
{
	TransferInventoryAck	Low	296 Trusted	Zerocoded
	{
		InfoBlock			Single
		{	TransactionID	LLUUID	}
		{	InventoryID		LLUUID	}
	}
}


{
	AcceptFriendship Low 297 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
	}
	{
		TransactionBlock	Single
		{	TransactionID	LLUUID	}
	}
	{
		FolderData			Variable
		{	FolderID		LLUUID	} // place to put calling card.
	}
}

{
	DeclineFriendship Low 298 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
	}
	{
		TransactionBlock	Single
		{	TransactionID	LLUUID	}
	}
}

{
	FormFriendship	Low	299 Trusted	Unencoded
	{
		AgentBlock		Single
		{ SourceID		LLUUID	}
		{ DestID		LLUUID	}
	}
}

// Cancels user relationship
// Updates inventory for both users.
// Stops agent tracking in userserver.
// viewer -> userserver -> dataserver
// reliable
{
	TerminateFriendship Low 300 NotTrusted Unencoded
	{
		AgentData		Single
		{ AgentID		LLUUID	}
		{ SessionID		LLUUID	}
	}
	{
		ExBlock			Single
		{ OtherID		LLUUID	}
	}
}

// used to give someone a calling card.
{
	OfferCallingCard Low 301 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
	}
	{
		AgentBlock			Single
		{	DestID			LLUUID	}
		{	TransactionID	LLUUID	}
	}
}

{
	AcceptCallingCard Low 302 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
	}
	{
		TransactionBlock	Single
		{	TransactionID	LLUUID	}
	}
	{
		FolderData			Variable
		{	FolderID		LLUUID	} // place to put calling card.
	}
}

{
	DeclineCallingCard Low 303 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID  }
	}
	{
		TransactionBlock	Single
		{	TransactionID	LLUUID	}
	}
}


// Rez a script onto an object
{
	RezScript Low 304 NotTrusted Zerocoded
	{
		AgentData		Single
		{ AgentID		LLUUID	}
		{ SessionID		LLUUID	}
		{  GroupID			LLUUID	}
	}
	{
		UpdateBlock			Single
		{  ObjectLocalID	U32	}   // object id in world 
		{  Enabled			BOOL	}	// is script rezzed in enabled?
	}
	{
		InventoryBlock		Single
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}
		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions
		{	TransactionID	LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

// Create inventory
{
	CreateInventoryItem Low 305 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{	
		InventoryBlock		Single
		{	CallbackID	U32			} // Async Response
		{	FolderID		LLUUID	}
		{	TransactionID			LLUUID	} // Going to become TransactionID
		{	NextOwnerMask	U32	}
		{	Type			S8	}
		{	InvType			S8	}
		{	WearableType	U8	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
	}
}

// give agent a landmark for an event.
{
	CreateLandmarkForEvent	Low	306 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		EventData		Single
		{	EventID		U32		}
	}
	{	
		InventoryBlock		Single
		{	FolderID		LLUUID	}
		{	Name			Variable	1	}
	}
}

{
	EventLocationRequest	Low	307 Trusted	Zerocoded
	{
		QueryData		Single
		{	QueryID		LLUUID			}
	}
	{
		EventData		Single
		{	EventID		U32		}
	}
}

{
	EventLocationReply	Low	308 Trusted	Zerocoded
	{
		QueryData		Single
		{	QueryID		LLUUID	}
	}
	{
		EventData		Single
		{	Success		BOOL	}
		{	RegionID	LLUUID	}
		{	RegionPos	LLVector3	}
	}
}

// get information about landmarks. Used by viewers for determining
// the location of a landmark, and by simulators for teleport
{
	RegionHandleRequest Low 309 NotTrusted Unencoded
	{
		RequestBlock		Single
		{	RegionID		LLUUID	}
	}
}

{
	RegionIDAndHandleReply Low 310 Trusted Unencoded
	{
		ReplyBlock			Single
		{	RegionID		LLUUID	}
		{	RegionHandle	U64		}
	}
}

// Move money from one agent to another. Validation will happen at the
// simulator, the dataserver will actually do the work. Dataserver
// generates a MoneyBalance message in reply.  The simulator
// will generate a MoneyTransferBackend in response to this.
// viewer -> simulator -> dataserver 
{
	MoneyTransferRequest Low 311 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		MoneyData			Single
		{	SourceID		LLUUID  }
		{	DestID			LLUUID	}	// destination of the transfer
		{	Flags			U8		}
		{	Amount			S32		}
		{	AggregatePermNextOwner	U8	}
		{	AggregatePermInventory	U8	}
		{	TransactionType	S32		}	// see lltransactiontypes.h
		{	Description		Variable 1	}	// string, name of item for purchases
	}
}

// And, the money transfer
// *NOTE: Unused as of 2010-04-06, because all back-end money transactions
// are done with web services via L$ API.  JC
{
	MoneyTransferBackend Low 312 Trusted Zerocoded
	{
		MoneyData			Single
		{	TransactionID	LLUUID	}
		{ 	TransactionTime	U32	} // utc seconds since epoch
		{	SourceID		LLUUID  }
		{	DestID			LLUUID	}	// destination of the transfer
		{	Flags			U8		}
		{	Amount			S32		}
		{	AggregatePermNextOwner	U8	}
		{	AggregatePermInventory	U8	}
		{	TransactionType	S32		}	// see lltransactiontypes.h
		{	RegionID		LLUUID		}	// region sending the request, for logging
		{	GridX			U32			}	// *HACK: database doesn't have region_id in schema
		{	GridY			U32			}	// *HACK: database doesn't have region_id in schema
		{	Description		Variable 1	}	// string, name of item for purchases
	}
}


// viewer -> userserver -> dataserver
// Reliable
{
	MoneyBalanceRequest Low 313 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		MoneyData			Single
		{	TransactionID	LLUUID	}
	}
}


// dataserver -> simulator -> viewer
{
	MoneyBalanceReply Low 314 Trusted Zerocoded
	{
		MoneyData			Single
		{	AgentID				LLUUID  }
		{	TransactionID		LLUUID	}
		{	TransactionSuccess	BOOL	}	// BOOL
		{	MoneyBalance		S32	}
		{	SquareMetersCredit	S32	}
		{	SquareMetersCommitted	S32	}
		{	Description			Variable 1	}	// string
	}
	// For replies that are part of a transaction (buying something) provide
	// metadata for localization.  If TransactionType is 0, the message is
	// purely a balance update.  Added for server 1.40 and viewer 2.1.  JC
	{
		TransactionInfo		Single
		{	TransactionType			S32			}	// lltransactiontype.h
		{	SourceID				LLUUID		}
		{	IsSourceGroup			BOOL		}
		{	DestID					LLUUID		}
		{	IsDestGroup				BOOL		}
		{	Amount					S32			}
		{	ItemDescription			Variable 1	}	// string
	}
}


// RoutedMoneyBalanceReply
// This message is used when a dataserver needs to send updated
// money balance information to a simulator other than the one it
// is connected to.  It uses the standard TransferBlock format.
// dataserver -> simulator -> spaceserver -> simulator -> viewer
// reliable
{
	RoutedMoneyBalanceReply Low 315 Trusted Zerocoded
	{
		TargetBlock			Single
		{	TargetIP		IPADDR	}	// U32 encoded IP
		{	TargetPort		IPPORT	}
	}
	{
		MoneyData			Single
		{	AgentID				LLUUID  }
		{	TransactionID		LLUUID	}
		{	TransactionSuccess	BOOL	}	// BOOL
		{	MoneyBalance		S32	}
		{	SquareMetersCredit	S32	}
		{	SquareMetersCommitted	S32	}
		{	Description			Variable 1	}	// string
	}
	// See MoneyBalanceReply above.
	{
		TransactionInfo		Single
		{	TransactionType			S32			}	// lltransactiontype.h
		{	SourceID				LLUUID		}
		{	IsSourceGroup			BOOL		}
		{	DestID					LLUUID		}
		{	IsDestGroup				BOOL		}
		{	Amount					S32			}
		{	ItemDescription			Variable 1	}	// string
	}
}



//---------------------------------------------------------------------------
// Gesture saves/loads
//---------------------------------------------------------------------------


// Tell the database that some gestures are now active
// viewer -> sim -> data
{
	ActivateGestures	Low	316 	NotTrusted	Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	Flags			U32		}
	}
	{
		Data			Variable
		{	ItemID			LLUUID	}
		{	AssetID			LLUUID	}
		{	GestureFlags	U32		}
	}
}

// Tell the database some gestures are no longer active
// viewer -> sim -> data
{
	DeactivateGestures	Low	317 	NotTrusted	Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	Flags			U32		}
	}
	{
		Data			Variable
		{	ItemID			LLUUID	}
		{	GestureFlags	U32		}
	}
}

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------

// userserver -> viewer, up-to-date inventory is here
// could be sent as a result of spam
// as well as in response to InventoryRequest
//{
//	InventoryUpdate Low Trusted Unencoded
//	{
//		AgentData		Single
//		{	AgentID		LLUUID	}
//	}
//	{
//		InventoryData	Single
//		{	IsComplete	U8	}
//		{	Filename	Variable	1	}
//	}
//}

// dataserver-> userserver -> viewer to move around the mute list
{
	MuteListUpdate Low 318 Trusted Unencoded
	{
		MuteData	Single
		{	AgentID		LLUUID	}
		{	Filename	Variable	1	}
	}
}

// tell viewer to use the local mute cache
{
	UseCachedMuteList Low 319 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
}

// Sent from viewer to simulator to set user rights. This message will be
// relayed up to the dataserver through a PUT. If that
// succeeds, an UpdateUserRights will be relayed to the originating
// viewer, and a presence lookup will be performed to find
// agent-related and the same PUT will be issued to the sim host if
// they are online.
{
	GrantUserRights Low 320 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Rights			Variable
		{	AgentRelated		LLUUID		}
		{	RelatedRights		S32		}
	}
}

// This message is sent from the simulator to the viewer to indicate a
// targets granted rights. This is only sent to the originator of the
// request and the target agent if it is a modify or map
// right. Adding/removing online status rights will show up as an
// online/offline notification.
{
	ChangeUserRights Low 321 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		Rights			Variable
		{	AgentRelated		LLUUID		}
		{	RelatedRights		S32		}
	}
}

// notification for login and logout. 
// source_sim -> dest_viewer
{
	OnlineNotification Low 322 Trusted Unencoded
	{
		AgentBlock		Variable
		{	AgentID		LLUUID	}
	}
}
{
	OfflineNotification Low 323 Trusted Unencoded
	{
		AgentBlock		Variable
		{	AgentID		LLUUID	}
	}
}


// SetStartLocationRequest
// viewer -> sim 
// failure checked at sim and triggers ImprovedInstantMessage 
// success triggers SetStartLocation 
{
	SetStartLocationRequest Low 324 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		StartLocationData	Single
		{	SimName			Variable	1	}	// string
		{	LocationID		U32	}
		{	LocationPos		LLVector3	}		// region coords
		{	LocationLookAt	LLVector3	}
	}
}

// SetStartLocation
// sim -> dataserver
{
	SetStartLocation Low 325 Trusted Zerocoded
	{
		StartLocationData	Single
		{	AgentID			LLUUID	}
		{	RegionID		LLUUID	}
		{	LocationID		U32	}
		{	RegionHandle	U64			}
		{	LocationPos		LLVector3	}		// region coords
		{	LocationLookAt	LLVector3	}
	}
}


// ***************************************************************************
// Launcher messages
// ***************************************************************************


// NetTest - This goes back and forth to the space server because of
// problems determining the port
{
	NetTest Low 326 NotTrusted Unencoded
	{
		NetBlock			Single
		{	Port		IPPORT	}
	}
}

// SetChildCount - Sent to launcher to adjust nominal child count
// Simulator sends this increase the sim/cpu ratio on startup
{
	SetCPURatio Low 327 NotTrusted Unencoded
	{
		Data	Single
		{	Ratio		U8	}
	}
}



// SimCrashed - Sent to dataserver when the sim goes down.
// Maybe we should notify the spaceserver as well?
{
	SimCrashed	Low 328 NotTrusted Unencoded
	{
		Data	Single
		{	RegionX			U32 }
		{	RegionY			U32 }
	}
	{
		Users	Variable
		{	AgentID			LLUUID	}
	}
}

// ***************************************************************************
// Name Value Pair messages
// ***************************************************************************

// NameValuePair - if the specific task exists on simulator, add or replace this name value pair
{
	NameValuePair Low 329 Trusted Unencoded
	{
		TaskData			Single
		{	ID				LLUUID	}
	}
	{
		NameValueData		Variable
		{	NVPair			Variable	2	}
	}
}

// NameValuePair - if the specific task exists on simulator or dataserver, remove the name value pair (value is ignored)
{
	RemoveNameValuePair Low 330 Trusted Unencoded
	{
		TaskData			Single
		{	ID				LLUUID	}
	}
	{
		NameValueData		Variable
		{	NVPair			Variable	2	}
	}
}


// ***************************************************************************
// Add/Remove Attachment messages
// ***************************************************************************

//
// Simulator informs Dataserver of new attachment or attachment asset update 
// DO NOT ALLOW THIS FROM THE VIEWER
//
{
	UpdateAttachment Low 331 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		AttachmentBlock		Single
		{	AttachmentPoint	U8	}
	}
	{
		OperationData		Single
		{	AddItem			BOOL	}
		{	UseExistingAsset BOOL	}
	}
	{
		InventoryData		Single			// Standard inventory item block
		{	ItemID			LLUUID	}
		{	FolderID		LLUUID	}

		{	CreatorID		LLUUID	}	// permissions
		{	OwnerID			LLUUID	}	// permissions
		{	GroupID			LLUUID	}	// permissions
		{	BaseMask		U32	}	// permissions
		{	OwnerMask		U32	}	// permissions
		{	GroupMask		U32	}	// permissions
		{	EveryoneMask	U32	}	// permissions
		{	NextOwnerMask	U32	}	// permissions
		{	GroupOwned		BOOL	}	// permissions

		{	AssetID			LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Flags			U32	}
		{	SaleType		U8	}
		{	SalePrice		S32	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
		{	CreationDate	S32	}
		{	CRC				U32	}
	}
}

// Simulator informs Dataserver that attachment has been taken off
{
	RemoveAttachment Low 332 NotTrusted Unencoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		AttachmentBlock		Single
		{	AttachmentPoint	U8	}
		{	ItemID			LLUUID	}
	}
}


// ***************************************************************************
// GUIDed Sound messages
// ***************************************************************************

// SoundTrigger - Sent by simulator to viewer to trigger sound outside current region
{
	SoundTrigger High 29 NotTrusted Unencoded
	{
		SoundData			Single
		{	SoundID			LLUUID		}
		{	OwnerID			LLUUID		}
		{	ObjectID		LLUUID		}
		{	ParentID		LLUUID		}	// null if this object is the parent
		{   Handle			U64			}	// region handle
		{	Position		LLVector3	}	// region local
		{	Gain			F32			}
	}
}

// AttachedSound - Sent by simulator to viewer to play sound attached with an object
{
	AttachedSound Medium 13 Trusted Unencoded
	{
		DataBlock			Single
		{	SoundID			LLUUID	}
		{	ObjectID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	Gain			F32	}
		{	Flags			U8	}
	}
}

// AttachedSoundGainChange - Sent by simulator to viewer to change an attached sounds' volume

{
	AttachedSoundGainChange Medium 14 Trusted Unencoded
	{
		DataBlock			Single
		{	ObjectID		LLUUID	}
		{	Gain			F32	}
	}
}


// PreloadSound - Sent by simulator to viewer to preload sound for an object

{
	PreloadSound Medium 15 Trusted Unencoded
	{
		DataBlock			Variable
		{	ObjectID		LLUUID	}
		{	OwnerID			LLUUID	}
		{	SoundID			LLUUID	}
	}
}


// *************************************************************************
// Asset storage messages
// *************************************************************************

// current assumes an existing UUID, need to enhance for new assets
{
	AssetUploadRequest Low 333 NotTrusted Unencoded
	{
		AssetBlock		Single
		{	TransactionID LLUUID	}
		{	Type		S8	}
		{	Tempfile	BOOL	}
		{	StoreLocal	BOOL	}
		{	AssetData	Variable	2	}  // Optional: the actual asset data if the whole thing will fit it this packet
	}
}

{
	AssetUploadComplete Low 334 NotTrusted Unencoded
	{
		AssetBlock		Single
		{	UUID		LLUUID	}
		{	Type		S8	}
		{	Success		BOOL	}
	}
}


// Script on simulator asks dataserver if there are any email messages
// waiting.
{
	EmailMessageRequest Low 335 Trusted Unencoded
	{
		DataBlock			Single
		{	ObjectID		LLUUID	}
		{	FromAddress		Variable	1	}
		{	Subject			Variable	1	}
	}
}

// Dataserver gives simulator the oldest email message in the queue, along with
// how many messages are left in the queue.  And passes back the filter used to request emails.
{
	EmailMessageReply Low 336 Trusted Unencoded
	{
		DataBlock			Single
		{	ObjectID		LLUUID	}
		{	More			U32	} //U32
		{	Time			U32	} //U32
		{	FromAddress		Variable	1	}
		{	Subject			Variable	1	}
		{	Data			Variable	2	}
		{	MailFilter		Variable	1	}
	}
}

// Script on simulator sends mail to another script
{
	InternalScriptMail Medium 16 Trusted Unencoded
	{
		DataBlock			Single
		{	From			Variable 	1	}
		{	To				LLUUID			}
		{	Subject			Variable 	1	}
		{	Body			Variable 	2	}
	}
}

// Script on simulator asks dataserver for information 
{
	ScriptDataRequest Low 337 Trusted Unencoded
	{
		DataBlock			Variable
		{	Hash			U64				}
		{	RequestType		S8				}
		{	Request			Variable	2	}
	}
}

// Data server responds with data
{
	ScriptDataReply Low 338 Trusted Unencoded
	{
		DataBlock			Variable
		{	Hash			U64				}		
		{	Reply			Variable	2	}
	}
}


//-----------------------------------------------------------------------------
// Group messages
//-----------------------------------------------------------------------------

// CreateGroupRequest
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	CreateGroupRequest Low 339 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		GroupData		Single
		{	Name			Variable	1	}	// string
		{	Charter			Variable	2	}	// string
		{	ShowInList		BOOL	}
		{	InsigniaID		LLUUID	}
		{	MembershipFee	S32				}	// S32		
		{	OpenEnrollment	BOOL			}   // BOOL (U8)
		{	AllowPublish	BOOL		}	// whether profile is externally visible or not
		{	MaturePublish	BOOL		}	// profile is "mature"
	}
}

// CreateGroupReply
// dataserver -> simulator
// simulator -> viewer
// reliable
{
	CreateGroupReply Low 340 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
	}
	{
		ReplyData		Single
		{	GroupID			LLUUID	}
		{	Success			BOOL	}
		{	Message			Variable	1	}	// string
	}
}

// UpdateGroupInfo
// viewer -> simulator
// simulator -> dataserver
// reliable
{
	UpdateGroupInfo Low 341 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID			LLUUID	}
		{	Charter			Variable	2	}	// string
		{	ShowInList		BOOL			}
		{	InsigniaID		LLUUID	}
		{	MembershipFee	S32				}
		{	OpenEnrollment	BOOL			}
		{	AllowPublish	BOOL	}
		{	MaturePublish	BOOL	}
	}
}

// GroupRoleChanges
// viewer -> simulator -> dataserver
// reliable
{
	GroupRoleChanges	Low	342 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID	LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		RoleChange	Variable
		{	RoleID		LLUUID	}
		{	MemberID	LLUUID	}
		{	Change		U32		}
	}
}

// JoinGroupRequest
// viewer -> simulator -> dataserver
// reliable
{
	JoinGroupRequest Low 343 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID		LLUUID	}
	}
}

// JoinGroupReply
// dataserver -> simulator -> viewer
{
	JoinGroupReply Low 344 Trusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID		LLUUID	}
		{	Success		BOOL	}
	}
}


// EjectGroupMemberRequest
// viewer -> simulator -> dataserver
// reliable
{
	EjectGroupMemberRequest Low 345 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
	}
	{
		EjectData		Variable
		{	EjecteeID	LLUUID	}
	}
}

// EjectGroupMemberReply
// dataserver -> simulator -> viewer
// reliable
{
	EjectGroupMemberReply Low 346 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
	}
	{
		EjectData		Single
		{	Success		BOOL	}
	}
}

// LeaveGroupRequest
// viewer -> simulator -> dataserver
// reliable
{
	LeaveGroupRequest Low 347 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
	}
}

// LeaveGroupReply
// dataserver -> simulator -> viewer
{
	LeaveGroupReply Low 348 Trusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID		LLUUID	}
		{	Success		BOOL	}
	}
}

// InviteGroupRequest
// viewer -> simulator -> dataserver
// reliable
{
	InviteGroupRequest Low 349 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}	// UUID of inviting agent
		{	SessionID	LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID		LLUUID	}
	}
	{
		InviteData	Variable
		{	InviteeID	LLUUID	}
		{	RoleID		LLUUID	}
	}
}

// InviteGroupResponse
// simulator -> dataserver
// reliable
{
	InviteGroupResponse	Low	350 Trusted	Unencoded
	{
		InviteData	Single
		{	AgentID		LLUUID	}
		{	InviteeID	LLUUID	}
		{	GroupID			LLUUID	}
		{	RoleID		LLUUID	}
		{	MembershipFee S32	}
	}
}

// GroupProfileRequest
// viewer-> simulator -> dataserver
// reliable
{
	GroupProfileRequest Low 351 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID	LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID			LLUUID			}
	}
}

// GroupProfileReply
// dataserver -> simulator -> viewer
// reliable
{
	GroupProfileReply Low 352 Trusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
	}
	{
		GroupData		Single
		{	GroupID			LLUUID			}
		{	Name			Variable	1	}	// string
		{	Charter			Variable	2	}	// string
		{	ShowInList		BOOL	}
		{	MemberTitle		Variable	1	}	// string
		{	PowersMask		U64	}	// U32 mask
		{	InsigniaID		LLUUID			}
		{	FounderID		LLUUID			}
		{	MembershipFee	S32				}
		{	OpenEnrollment	BOOL			}   // BOOL (U8)
		{	Money			S32	}
		{	GroupMembershipCount	S32	}
		{	GroupRolesCount			S32	}
		{	AllowPublish	BOOL	}
		{	MaturePublish	BOOL	}
		{	OwnerRole		LLUUID	}
	}
}

// CurrentInterval = 0  =>  this period (week, day, etc.)
// CurrentInterval = 1  =>  last period
// viewer -> simulator -> dataserver
// reliable
{
	GroupAccountSummaryRequest Low 353 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID	}
		{	GroupID			LLUUID	}
	}
	{
		MoneyData			Single
		{	RequestID		LLUUID	}
		{	IntervalDays	S32	}
		{	CurrentInterval	S32	}
	}
}


// dataserver -> simulator -> viewer
// Reliable
{
	GroupAccountSummaryReply Low 354 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	GroupID			LLUUID		}
	}
	{
		MoneyData			Single
		{	RequestID			LLUUID	}
		{	IntervalDays		S32	}
		{	CurrentInterval		S32	}
		{	StartDate			Variable	1	}	// string
		{	Balance				S32	}
		{	TotalCredits		S32	}
		{	TotalDebits			S32	}
		{	ObjectTaxCurrent	S32	}
		{	LightTaxCurrent		S32	}
		{	LandTaxCurrent		S32	}
		{	GroupTaxCurrent		S32	}
		{	ParcelDirFeeCurrent	S32	}
		{	ObjectTaxEstimate	S32	}
		{	LightTaxEstimate	S32	}
		{	LandTaxEstimate		S32	}
		{	GroupTaxEstimate	S32	}
		{	ParcelDirFeeEstimate	S32	}
		{	NonExemptMembers	S32	}
		{	LastTaxDate			Variable	1	}	// string
		{	TaxDate				Variable	1	}	// string
	}
}


// Reliable
{
	GroupAccountDetailsRequest Low 355 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	GroupID			LLUUID	}
	}
	{
		MoneyData			Single
		{	RequestID		LLUUID	}
		{	IntervalDays	S32	}
		{	CurrentInterval	S32	}
	}
}

// Reliable
{
	GroupAccountDetailsReply Low 356 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	GroupID			LLUUID		}
	}
	{
		MoneyData			Single
		{	RequestID		LLUUID	}
		{	IntervalDays	S32	}
		{	CurrentInterval	S32	}
		{	StartDate		Variable	1	}	// string
	}
	{
		HistoryData			Variable
		{	Description		Variable	1	}	// string
		{	Amount			S32	}
	}
}


// Reliable
{
	GroupAccountTransactionsRequest Low 357 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
		{	GroupID			LLUUID		}
	}
	{
		MoneyData			Single
		{	RequestID		LLUUID	}
		{	IntervalDays	S32	}
		{	CurrentInterval	S32	}
	}
}

// Reliable
{
	GroupAccountTransactionsReply Low 358 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID			LLUUID	}
		{	GroupID			LLUUID		}
	}
	{
		MoneyData			Single
		{	RequestID		LLUUID	}
		{	IntervalDays	S32	}
		{	CurrentInterval	S32	}
		{	StartDate		Variable	1	}	// string
	}
	{
		HistoryData			Variable
		{	Time			Variable	1	}	// string
		{	User			Variable	1	}	// string
		{	Type			S32	}
		{	Item			Variable	1	}	// string
		{	Amount			S32	}
	}
}

// GroupActiveProposalsRequest
// viewer -> simulator -> dataserver
//reliable
{
	GroupActiveProposalsRequest Low 359 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		GroupData	Single
		{	GroupID			LLUUID			}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
	}
}

// GroupActiveProposalItemReply
// dataserver -> simulator -> viewer
// reliable
{
	GroupActiveProposalItemReply Low 360 Trusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	GroupID			LLUUID			}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
		{	TotalNumItems	U32		}
	}
	{
		ProposalData	Variable
		{	VoteID			LLUUID			}
		{	VoteInitiator	LLUUID			}
		{	TerseDateID		Variable	1	} // string
		{	StartDateTime	Variable	1	}	// string
		{	EndDateTime		Variable	1	}	// string
		{	AlreadyVoted	BOOL			}
		{	VoteCast		Variable	1	}	// string
		{	Majority	F32		}
		{	Quorum		S32		}
		{	ProposalText	Variable	1	}	// string
	}
}

// GroupVoteHistoryRequest
// viewer -> simulator -> dataserver
//reliable
{
	GroupVoteHistoryRequest Low 361 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID		LLUUID			}
	}
	{
		GroupData	Single
		{	GroupID			LLUUID			}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
	}
}

// GroupVoteHistoryItemReply
// dataserver -> simulator -> viewer
// reliable
{
	GroupVoteHistoryItemReply Low 362 Trusted Zerocoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	GroupID			LLUUID			}
	}
	{
		TransactionData Single
		{	TransactionID	LLUUID	}
		{	TotalNumItems	U32		}
	}
	{
		HistoryItemData	Single
		{	VoteID			LLUUID			}
		{	TerseDateID		Variable	1	} // string
		{	StartDateTime	Variable	1	}	// string
		{	EndDateTime		Variable	1	}	// string
		{	VoteInitiator	LLUUID			}
		{	VoteType		Variable	1	}	// string
		{	VoteResult		Variable	1	}	// string
		{	Majority	F32		}
		{	Quorum		S32		}
		{	ProposalText	Variable	2	}	// string
	}
	{
		VoteItem	Variable
		{	CandidateID		LLUUID		}
		{	VoteCast		Variable	1	}	// string
		{	NumVotes		S32		}
	}
}

// StartGroupProposal
// viewer -> simulator -> dataserver
// reliable
{
	StartGroupProposal Low 363 NotTrusted Zerocoded UDPDeprecated
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ProposalData		Single
		{	GroupID			LLUUID			}
		{	Quorum			S32				}
		{	Majority		F32				}	// F32
		{	Duration		S32				}	// S32, seconds
		{	ProposalText	Variable	1	}	// string
	}
}

// GroupProposalBallot
// viewer -> simulator -> dataserver
// reliable
{
	GroupProposalBallot Low 364 NotTrusted Unencoded UDPDeprecated
	{
		AgentData		Single
		{	AgentID		LLUUID			}
		{	SessionID	LLUUID	}
	}
	{
		ProposalData		Single
		{	ProposalID		LLUUID			}
		{	GroupID			LLUUID			}
		{	VoteCast		Variable	1	}	// string
	}
}

// TallyVotes userserver -> dataserver
// reliable
{
	TallyVotes	Low	365 Trusted Unencoded
}



// GroupMembersRequest
// get the group members
// simulator -> dataserver
// reliable
{
	GroupMembersRequest Low 366 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
		{   RequestID	LLUUID	}
	}
}

// GroupMembersReply
// list of uuids for the group members
// dataserver -> simulator
// reliable
{
	GroupMembersReply Low 367 Trusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
		{   RequestID	LLUUID	}
		{	MemberCount	S32		}
	}
	{
		MemberData		Variable
		{	AgentID		LLUUID	}
		{	Contribution	S32	}
		{	OnlineStatus	Variable	1	}	// string
		{	AgentPowers		U64	}
		{	Title			Variable	1	}	// string
		{	IsOwner			BOOL	}
	}
}

// used to switch an agent's currently active group.
// viewer -> simulator -> dataserver -> AgentDataUpdate...
{
	ActivateGroup	Low	368 NotTrusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
}

// viewer -> simulator -> dataserver
{
	SetGroupContribution Low 369 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data	Single
		{	GroupID		LLUUID	}
		{	Contribution	S32	}
	}
}

// viewer -> simulator -> dataserver
{
	SetGroupAcceptNotices Low 370 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Data	Single
		{	GroupID		LLUUID	}
		{	AcceptNotices	BOOL	}
	}
	{
		NewData				Single
		{	ListInProfile	BOOL	}
	}
}

// GroupRoleDataRequest
// viewer -> simulator -> dataserver
{
	GroupRoleDataRequest Low	371 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		GroupData	Single
		{	GroupID		LLUUID	}
		{	RequestID	LLUUID	}
	}
}


// GroupRoleDataReply
// All role data for this group
// dataserver -> simulator -> agent
{
	GroupRoleDataReply Low	372 Trusted	Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
	}
	{
		GroupData		Single
		{	GroupID			LLUUID	}
		{	RequestID	LLUUID	}
		{	RoleCount	S32		}
	}
	{
		RoleData	Variable
		{	RoleID		LLUUID	}
		{	Name		Variable	1	}
		{	Title		Variable	1	}
		{	Description	Variable	1	}
		{	Powers		U64		}
		{	Members		U32		}
	}
}

// GroupRoleMembersRequest
// viewer -> simulator -> dataserver
{
	GroupRoleMembersRequest Low	373 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID			LLUUID			}
		{	SessionID	LLUUID	}
	}
	{
		GroupData		Single
		{	GroupID		LLUUID	}
		{	RequestID	LLUUID	}
	}
}

// GroupRoleMembersReply
// All role::member pairs for this group.
// dataserver -> simulator -> agent
{
	GroupRoleMembersReply Low	374 Trusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	GroupID		LLUUID	}
		{	RequestID	LLUUID	}
		{	TotalPairs	U32		}
	}
	{
		MemberData		Variable
		{	RoleID		LLUUID	}
		{	MemberID	LLUUID	}
	}
}

// GroupTitlesRequest
// viewer -> simulator -> dataserver
{
	GroupTitlesRequest Low	375 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
		{	RequestID	LLUUID	}
	}
}


// GroupTitlesReply
// dataserver -> simulator -> viewer
{
	GroupTitlesReply Low 376 Trusted	Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	GroupID		LLUUID	}
		{	RequestID	LLUUID	}
	}
	{
		GroupData	Variable
		{	Title		Variable	1	} // string
		{	RoleID		LLUUID			}
		{	Selected	BOOL			}
	}
}

// GroupTitleUpdate
// viewer -> simulator -> dataserver
{
	GroupTitleUpdate	Low	377 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
		{	TitleRoleID	LLUUID	}
	}
}

// GroupRoleUpdate
// viewer -> simulator -> dataserver
{
	GroupRoleUpdate		Low	378 NotTrusted	Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	GroupID		LLUUID	}
	}
	{
		RoleData	Variable
		{	RoleID		LLUUID	}
		{	Name		Variable	1	}
		{	Description	Variable	1	}
		{	Title		Variable	1	}
		{	Powers		U64		}
		{	UpdateType	U8		}
	}
}
			


// Request the members of the live help group needed for requesting agent.
// userserver -> dataserver
{
	LiveHelpGroupRequest Low 379 Trusted Unencoded
	{
		RequestData 	Single
		{	RequestID	LLUUID	}
		{	AgentID		LLUUID	}
	}
}

// Send down the group
// dataserver -> userserver
{
	LiveHelpGroupReply Low 380 Trusted Unencoded
	{
		ReplyData	 	Single
		{	RequestID	LLUUID	}
		{	GroupID		LLUUID	}
		{	Selection	Variable 	1	} // selection criteria all or active
	}
}

//-----------------------------------------------------------------------------
// Wearable messages
//-----------------------------------------------------------------------------

// AgentWearablesRequest
// (a.k.a. "Tell me what the avatar is wearing.")
// viewer -> simulator -> dataserver
// reliable
{
	AgentWearablesRequest Low 381 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}

// AgentWearablesUpdate
// (a.k.a. "Here's what your avatar should be wearing now.")
// dataserver -> userserver -> viewer
// reliable
// NEVER from viewer to sim
{
	AgentWearablesUpdate Low 382 Trusted Zerocoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	SerialNum	U32	}	// U32, Increases every time the wearables change for a given agent.  Used to avoid processing out of order packets.
	}
	{
		WearableData	Variable
		{	ItemID		LLUUID	}
		{	AssetID		LLUUID	}
		{	WearableType U8	}	// U8, LLWearable::EWearType
	}
}

//
// AgentIsNowWearing
// (a.k.a. "Here's what I'm wearing now.")
// viewer->sim->dataserver
// reliable
{
	AgentIsNowWearing	Low	383 NotTrusted	Zerocoded
	{
		AgentData Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		WearableData	Variable
		{	ItemID		LLUUID	}
		{	WearableType	U8	}
	}
}
		

// AgentCachedTexture
// viewer queries for cached textures on dataserver (via simulator)
// viewer -> simulator -> dataserver
// reliable
{
	AgentCachedTexture Low 384 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	SerialNum	S32			}
	}
	{
		WearableData	Variable
		{	ID				LLUUID	}
		{	TextureIndex	U8 }
	}
}

// AgentCachedTextureResponse
// response to viewer queries for cached textures on dataserver (via simulator)
// dataserver -> simulator -> viewer
// reliable
{
	AgentCachedTextureResponse Low 385 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	SerialNum	S32			}
	}
	{
		WearableData	Variable
		{	TextureID		LLUUID	}
		{	TextureIndex	U8 }
		{	HostName		Variable	1 }
	}
}

// Request an AgentDataUpdate without changing any agent data.
{
	AgentDataUpdateRequest Low 386 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}

// AgentDataUpdate
// Updates a viewer or simulator's impression of agent-specific information.
// Used, for example, when an agent's group changes.
// dataserver -> simulator -> viewer
// reliable
{
	AgentDataUpdate Low 387 Trusted Zerocoded
	{
		AgentData			Single
		{	AgentID				LLUUID	}
		{	FirstName	        Variable	1	}   // string
		{	LastName	        Variable	1	}   // string
		{	GroupTitle			Variable	1	}	// string
		{	ActiveGroupID		LLUUID	}	// active group
		{	GroupPowers			U64	}
		{	GroupName			Variable	1	}	// string
	}
}


// GroupDataUpdate
// This is a bunch of group data that needs to be appropriatly routed based on presence info.
// dataserver -> simulator
{
	GroupDataUpdate	Low	388 Trusted	Zerocoded
	{
		AgentGroupData		Variable
		{	AgentID			LLUUID	}
		{	GroupID			LLUUID	}
		{	AgentPowers		U64		}
		{	GroupTitle		Variable	1	}
	}
}

// AgentGroupDataUpdate
// Updates a viewer or simulator's impression of the groups an agent is in. 
// dataserver -> simulator -> viewer
// reliable
{
	AgentGroupDataUpdate Low 389 Trusted Zerocoded UDPDeprecated
	{
		AgentData			Single
		{	AgentID				LLUUID	}
	}
	{
		GroupData			Variable
		{	GroupID				LLUUID	}
		{	GroupPowers			U64		}	
		{	AcceptNotices		BOOL	}
		{	GroupInsigniaID		LLUUID	}
		{	Contribution		S32		}
		{	GroupName			Variable	1	}	// string
	}
}

// AgentDropGroup
// Updates the viewer / simulator that an agent is no longer part of a group
// dataserver -> simulator -> viewer
// dataserver -> userserver
// reliable
{
	AgentDropGroup Low 390 Trusted Zerocoded UDPDeprecated
	{
		AgentData			Single
		{	AgentID				LLUUID	}
		{	GroupID				LLUUID	}
	}
}

// LogTextMessage
// Asks the dataserver to log the contents of this message in the
// chat and IM log table.
// Sent from userserver (IM logging) and simulator (chat logging).
{
	LogTextMessage Low 391 Trusted Zerocoded
	{
		DataBlock	Variable
		{ FromAgentId	LLUUID }
		{ ToAgentId	LLUUID }
		{ GlobalX	F64 }
		{ GlobalY	F64 }
		{ Time		U32	} // utc seconds since epoch
		{ Message	Variable	2 } // string
	}
}

// ViewerEffect
// Viewer side effect that's sent from one viewer, and broadcast to other agents nearby
// viewer-->sim (single effect created by viewer)
// sim-->viewer (multiple effects that can be seen by viewer)
// the AgentData block used for authentication for viewer-->sim messages
{
	ViewerEffect Medium 17 NotTrusted Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		Effect Variable
		{	ID			LLUUID	} // unique UUID of the effect
		{	AgentID		LLUUID	} // yes, pack AgentID again (note this block is variable)
		{	Type		U8	} // Type of the effect
		{	Duration	F32	} // F32 time (seconds)
		{	Color		Fixed		4	} // Color4U
		{	TypeData	Variable	1	} // Type specific data
	}
}


// CreateTrustedCircuit
// Sent to establish a trust relationship between two components.
// Only sent in response to a DenyTrustedCircuit message.
{
	CreateTrustedCircuit Low 392 NotTrusted Unencoded
	{
		DataBlock Single
		{ EndPointID	LLUUID }
		{ Digest        Fixed 32 } // 32 hex digits == 1 MD5 Digest
	}
}

// DenyTrustedCircuit
// Sent :
// - in response to failed CreateTrustedCircuit
// - to force the remote end-point to try to establish a trusted circuit
// - the reception of a trusted message on a non-trusted circuit
// This allows us to re-auth a circuit if it gets closed due to timeouts or network failures.
{
	DenyTrustedCircuit Low 393 NotTrusted Unencoded
	{
		DataBlock Single
		{ EndPointID	LLUUID }
	}
}

// RequestTrustedCircuit
// If the destination does not trust the sender, a Deny is sent back.
{
	RequestTrustedCircuit Low 394 Trusted Unencoded
}


{
	RezSingleAttachmentFromInv Low 395 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}
	{
		ObjectData			Single
		{	ItemID			LLUUID	}
		{	OwnerID			LLUUID	}
		{	AttachmentPt	U8		}	// 0 for default
		{	ItemFlags		U32		}
		{	GroupMask		U32		}
		{	EveryoneMask	U32		}
		{	NextOwnerMask	U32		}
		{	Name			Variable	1	}
		{	Description		Variable	1	}
	}
}

{
	RezMultipleAttachmentsFromInv Low 396 NotTrusted Zerocoded
	{
		AgentData			Single
		{	AgentID			LLUUID	}
		{	SessionID		LLUUID	}
	}	
	{
		HeaderData			Single
		{	CompoundMsgID			LLUUID  }	// All messages a single "compound msg" must have the same id
		{	TotalObjects			U8	}
		{	FirstDetachAll			BOOL	}
	}
	{
		ObjectData			Variable		// 1 to 4 of these per packet
		{	ItemID					LLUUID	}
		{	OwnerID					LLUUID	}
		{	AttachmentPt			U8	}	// 0 for default
		{	ItemFlags				U32 }
		{	GroupMask				U32 }
		{	EveryoneMask			U32 }
		{	NextOwnerMask			U32	}
		{	Name					Variable	1	}
		{	Description				Variable	1	}
	}
}


{
	DetachAttachmentIntoInv Low 397 NotTrusted Unencoded
	{
		ObjectData			Single
		{	AgentID					LLUUID  }
		{	ItemID					LLUUID	}
	}
}


// Viewer -> Sim
// Used in "Make New Outfit"
{
	CreateNewOutfitAttachments  Low 398 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		HeaderData			Single
		{	NewFolderID			LLUUID	}
	}
	{
		ObjectData			Variable
		{	OldItemID			LLUUID	}
		{	OldFolderID			LLUUID	}
	}
}

//-----------------------------------------------------------------------------
// Personal information messages
//-----------------------------------------------------------------------------

{
	UserInfoRequest	Low	399 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
}

{
	UserInfoReply	Low	400 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID	LLUUID	}
	}
	{
		UserData	Single
		{	IMViaEMail	BOOL	}
		{	DirectoryVisibility	Variable 1	}
		{	EMail		Variable	2	}
	}
}

{
	UpdateUserInfo	Low	401 NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		UserData	Single
		{	IMViaEMail	BOOL	}
		{	DirectoryVisibility	Variable 1	}
	}
}


//-----------------------------------------------------------------------------
// System operations and maintenance
//-----------------------------------------------------------------------------


// spaceserver -> sim
// tell a particular simulator to rename a parcel
{
	ParcelRename	Low	402 Trusted Unencoded
	{
		ParcelData 		Variable
		{	ParcelID		LLUUID	}
		{	NewName			Variable	1	}	// string
	}
}


// sim -> viewer
// initiate upload. primarily used for uploading raw files.
{
	InitiateDownload	Low	403 	NotTrusted Unencoded
	{
		AgentData	Single
		{	AgentID		LLUUID	}
	}
	{
		FileData	Single
		{	SimFilename		Variable	1	}	// string
		{	ViewerFilename	Variable	1	}	// string
	}
}

// Generalized system message. Each Requst has its own protocol for
// the StringData block format and contents.
{
    SystemMessage Low 404 Trusted Zerocoded
	{
		MethodData	 	Single
		{	Method		Variable 1	}
		{	Invoice		LLUUID		}
		{	Digest		Fixed 32	} // 32 hex digits == 1 MD5 Digest
	}
	{
		ParamList		Variable
		{	Parameter	Variable 1 }
	}
}


//-----------------------------------------------------------------------------
// map messages
//-----------------------------------------------------------------------------

// viewer -> sim
// reliable
// This message is sent up from the viewer to (eventually) get a list
// of all map layers and NULL-layer sims.
// Returns: MapLayerReply and MapBlockReply
{
	MapLayerRequest	Low	405 	NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Flags		U32		}
		{	EstateID	U32		}	// filled in on sim
		{	Godlike		BOOL	}	// filled in on sim
	}
}

// sim -> viewer
{
	MapLayerReply Low	406 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	Flags		U32		}
	}
	{
		LayerData		Variable
		{	Left		U32		}
		{	Right		U32		}
		{	Top			U32		}
		{	Bottom		U32		}
		{	ImageID		LLUUID	}
	}
}

// viewer -> sim
// This message is sent up from the viewer to get a list
// of the sims in a specified region.
// Returns: MapBlockReply
{
	MapBlockRequest Low	407 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Flags		U32		}
		{	EstateID	U32		}	// filled in on sim
		{	Godlike		BOOL	}	// filled in on sim
	}
	{
		PositionData	Single
		{	MinX		U16		}	// in region-widths
		{	MaxX		U16		}	// in region-widths
		{	MinY		U16		}	// in region-widths
		{	MaxY		U16		}	// in region-widths
	}
}

// viewer -> sim
// This message is sent up from the viewer to get a list
// of the sims with a given name.
// Returns: MapBlockReply
{
	MapNameRequest Low	408 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Flags		U32		}
		{	EstateID	U32		}	// filled in on sim
		{	Godlike		BOOL	}	// filled in on sim
	}
	{
		NameData		Single
		{	Name		Variable	1	}	// string
	}
}

// sim -> viewer
{
	MapBlockReply Low	409 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	Flags		U32		}
	}
	{	
		Data			Variable
		{	X			U16				}	// in region-widths
		{	Y			U16				}	// in region-widths
		{	Name		Variable	1	}	// string
		{	Access		U8				}	// PG, mature, etc.
		{	RegionFlags	U32				}
		{	WaterHeight	U8				}	// meters
		{	Agents		U8				}
		{	MapImageID	LLUUID			}
	}
}

// viewer -> sim
// This message is sent up from the viewer to get a list
// of the items of a particular type on the map.
// Used for Telehubs, Agents, Events, Popular Places, etc.
// Returns: MapBlockReply
{
	MapItemRequest Low	410 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
		{	Flags		U32		}
		{	EstateID	U32		}	// filled in on sim
		{	Godlike		BOOL	}	// filled in on sim
	}
	{
		RequestData		Single
		{	ItemType		U32		}
		{	RegionHandle	U64		} // filled in on sim
	}
}

// sim -> viewer
{
	MapItemReply Low	411 Trusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	Flags		U32		}
	}
	{
		RequestData		Single
		{	ItemType	U32		}
	}
	{	
		Data			Variable
		{	X			U32				}	// global position
		{	Y			U32				}	// global position
		{	ID			LLUUID			}	// identifier id
		{	Extra		S32				}	// extra information
		{	Extra2		S32				}	// extra information
		{	Name		Variable	1	}	// identifier string
	}
}

//-----------------------------------------------------------------------------
// Postcard messages
//-----------------------------------------------------------------------------
// reliable
{
	SendPostcard   	Low	412 	NotTrusted Unencoded
	{
		AgentData	Single
		{   AgentID		LLUUID		}
		{	SessionID	LLUUID		}
		{	AssetID		LLUUID		}
		{	PosGlobal	LLVector3d	}	// Where snapshot was taken
		{	To			Variable 1	}   // dest email address(es)
		{	From		Variable 1	}   // src email address(es)
		{	Name		Variable 1	}   // src name
		{	Subject		Variable 1	}   // mail subject
		{	Msg			Variable 2	}   // message text
		{	AllowPublish	BOOL		}	// Allow publishing on the web.
		{	MaturePublish	BOOL		}	// profile is "mature"
	}
}

// RPC messages
// Script on simulator requests rpc channel from rpcserver
// simulator -> dataserver -> MySQL
{
    RpcChannelRequest Low 413 Trusted Unencoded
    {
		DataBlock		Single
		{	GridX		U32 }
		{	GridY		U32 }
        {   TaskID      LLUUID }
        {   ItemID		LLUUID }
    }
}

// RpcServer allocated a session for the script
// ChannelID will be the NULL UUID if unable to register
// dataserver -> simulator
{
    RpcChannelReply Low 414 Trusted Unencoded
    {
        DataBlock   Single
        {   TaskID      LLUUID }
        {   ItemID		LLUUID }
        {   ChannelID   LLUUID }
    }
}

// Inbound RPC requests follow this path:
// RpcScriptRequestInbound: rpcserver -> spaceserver
// RpcScriptRequestInboundForward: spaceserver -> simulator
// reply: simulator -> rpcserver
{
	RpcScriptRequestInbound Low 415 NotTrusted Unencoded
	{
		TargetBlock Single
		{	GridX		U32 }
		{	GridY		U32	}
	}
	{
		DataBlock Single
		{	TaskID		LLUUID }
		{	ItemID		LLUUID }
		{	ChannelID	LLUUID }
		{	IntValue	U32 }
		{	StringValue Variable 2 } // string
	}
}

// spaceserver -> simulator
{
	RpcScriptRequestInboundForward Low 416 Trusted Unencoded UDPDeprecated
	{
		DataBlock Single
		{	RPCServerIP	IPADDR }
		{	RPCServerPort	IPPORT }
		{	TaskID		LLUUID }
		{	ItemID		LLUUID }
		{	ChannelID	LLUUID }
		{	IntValue	U32 }
		{	StringValue Variable 2 } // string
	}	
}

// simulator -> rpcserver
// Not trusted because trust establishment doesn't work here.
{
	RpcScriptReplyInbound Low 417 NotTrusted Unencoded
	{
		DataBlock Single
		{	TaskID		LLUUID }
		{	ItemID		LLUUID }
		{	ChannelID	LLUUID }
		{	IntValue	U32 }
		{	StringValue Variable 2 } // string
	}
}


// ScriptMailRegistration
// Simulator -> dataserver
{
	ScriptMailRegistration Low 418 Trusted Unencoded
	{
		DataBlock Single
		{	TargetIP		Variable 1	}	// String IP
		{	TargetPort		IPPORT	}
		{	TaskID			LLUUID	}
		{	Flags			U32	}
	}
}

// ParcelMediaCommandMessage
// Sends a parcel media command
{
	ParcelMediaCommandMessage Low 419 Trusted Unencoded
	{
		CommandBlock Single
		{	Flags		U32	}
		{	Command		U32	}
		{	Time		F32	}
	}
}

// ParcelMediaUpdate
// Sends a parcel media update to a single user
// For global updates use the parcel manager.
{
	ParcelMediaUpdate Low 420 Trusted Unencoded
	{
		DataBlock Single
		{	MediaURL		Variable	1	}	// string
		{	MediaID			LLUUID			}
		{	MediaAutoScale	U8				}
	}
	{
		DataBlockExtended Single
		{   MediaType       Variable    1   }
		{   MediaDesc       Variable    1   }
		{   MediaWidth      S32             }
		{   MediaHeight     S32             }
		{   MediaLoop       U8              }
	}
}

// LandStatRequest
// Sent by the viewer to request collider/script information for a parcel
{
	LandStatRequest Low 421 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID		}
		{	SessionID	LLUUID		}
	}
	{
		RequestData Single
		{	ReportType		U32				}
		{	RequestFlags	U32				}
		{	Filter			Variable	1	}
		{	ParcelLocalID	S32	}
	}
}

// LandStatReply
// Sent by the simulator in response to LandStatRequest
{
	LandStatReply Low 422 Trusted Unencoded UDPDeprecated
	{
		RequestData	Single
		{	ReportType			U32				}
		{	RequestFlags		U32				}
		{	TotalObjectCount	U32				}
	}
	{	
		ReportData	Variable
		{	TaskLocalID			U32				}
		{	TaskID				LLUUID			}
		{	LocationX			F32				}
		{	LocationY			F32				}
		{	LocationZ			F32				}
		{	Score				F32				}
		{	TaskName			Variable	1	}
		{	OwnerName			Variable	1	}
	}
}

// Generic Error -- this is used for sending an error message
// to a UDP recipient. The lowest common denominator is to at least
// log the message. More sophisticated receivers can do something
// smarter, for example, a money transaction failure can put up a 
// more user visible UI widget.
{
	Error Low 423 NotTrusted Zerocoded
	{
		AgentData Single
		{	AgentID		LLUUID	}				// will forward to agentid if coming from trusted circuit
	}
	{
		Data Single
		{	 Code		S32		}				// matches http status codes
		{	 Token		Variable	1	}		// some specific short string based message
		{	 ID			LLUUID	}				// the transactionid/uniqueid/sessionid whatever.
		{	 System		Variable	1	} 		// The hierarchical path to the system, eg, "message/handler"
		{	 Message	Variable	2	}		// Human readable message
		{	 Data		Variable	2	}		// Binary serialized LLSD for extra info.
	}
}

// ObjectIncludeInSearch
// viewer -> simulator
{
	ObjectIncludeInSearch Low 424 NotTrusted Unencoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		ObjectData		Variable
		{	ObjectLocalID	U32		}
		{	IncludeInSearch	BOOL	}
	}
}


// This message is sent from viewer -> simulator when the viewer wants
// to rez an object out of inventory back to its position before it
// last moved into the inventory
{
	RezRestoreToWorld Low 425 NotTrusted Unencoded UDPDeprecated
	{
		AgentData		Single
		{	AgentID		LLUUID  }
		{	SessionID	LLUUID	}
	}
	{
		InventoryData			Single
		{	ItemID				LLUUID	}
		{	FolderID			LLUUID	}
		{	CreatorID			LLUUID	}	// permissions
		{	OwnerID				LLUUID	}	// permissions
		{	GroupID				LLUUID	}	// permissions
		{	BaseMask			U32	}	// permissions
		{	OwnerMask			U32	}	// permissions
		{	GroupMask			U32	}	// permissions
		{	EveryoneMask		U32	}	// permissions
		{	NextOwnerMask		U32	}	// permissions
		{	GroupOwned			BOOL	}	// permissions
		{	TransactionID		LLUUID	}
		{	Type				S8	}
		{	InvType				S8	}
		{	Flags				U32	}
		{	SaleType			U8	}
		{	SalePrice			S32	}
		{	Name				Variable	1	}
		{	Description			Variable	1	}
		{	CreationDate		S32	}
		{	CRC				U32	}
	}
}

// Link inventory
{
	LinkInventoryItem	Low	426 NotTrusted	Zerocoded
	{
		AgentData		Single
		{	AgentID		LLUUID	}
		{	SessionID	LLUUID	}
	}
	{
		InventoryBlock		Single
		{	CallbackID	U32			} // Async Response
		{	FolderID		LLUUID	}
		{	TransactionID			LLUUID	} // Going to become TransactionID
		{	OldItemID		LLUUID	}
		{	Type			S8	}
		{	InvType			S8	}
		{	Name			Variable	1	}
		{	Description		Variable	1	}

	}
}

