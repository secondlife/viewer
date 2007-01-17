/** 
 * @file lltransfersourcefile.h
 * @brief Transfer system for sending a file.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLTRANSFERSOURCEFILE_H
#define LL_LLTRANSFERSOURCEFILE_H

#include "lltransfermanager.h"

#include <stdio.h>

class LLTransferSourceParamsFile : public LLTransferSourceParams
{
public:
	LLTransferSourceParamsFile();
	virtual ~LLTransferSourceParamsFile() {}
	/*virtual*/ void packParams(LLDataPacker &dp) const;
	/*virtual*/ BOOL unpackParams(LLDataPacker &dp);

	void setFilename(const LLString &filename)		{ mFilename = filename; }
	std::string getFilename() const					{ return mFilename; }

	void setDeleteOnCompletion(BOOL enabled)		{ mDeleteOnCompletion = enabled; }
	BOOL getDeleteOnCompletion()					{ return mDeleteOnCompletion; }
protected:
	std::string mFilename;
	// ONLY DELETE THINGS OFF THE SIM IF THE FILENAME BEGINS IN 'TEMP'
	BOOL mDeleteOnCompletion;
};

class LLTransferSourceFile : public LLTransferSource
{
public:
	LLTransferSourceFile(const LLUUID &transfer_id, const F32 priority);
	virtual ~LLTransferSourceFile();

protected:
	/*virtual*/ void initTransfer();
	/*virtual*/ F32 updatePriority();
	/*virtual*/ LLTSCode dataCallback(const S32 packet_id,
									  const S32 max_bytes,
									  U8 **datap,
									  S32 &returned_bytes,
									  BOOL &delete_returned);
	/*virtual*/ void completionCallback(const LLTSCode status);

	virtual void packParams(LLDataPacker& dp) const;
	/*virtual*/ BOOL unpackParams(LLDataPacker &dp);

protected:
	LLTransferSourceParamsFile mParams;
	FILE *mFP;
};

#endif // LL_LLTRANSFERSOURCEFILE_H
