/** 
 * @file llfloaterauction.h
 * @author James Cook, Ian Wilkes
 * @brief llfloaterauction class header file
 *
 * Copyright (c) 2004-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLFLOATERAUCTION_H
#define LL_LLFLOATERAUCTION_H

#include "llfloater.h"
#include "lluuid.h"
#include "llmemory.h"
#include "llviewerimage.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLFloaterAuction
//
// Class which holds the functionality to start auctions.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
class LLParcelSelection;

class LLFloaterAuction : public LLFloater
{
public:
	// LLFloater interface
	/*virtual*/ void onClose(bool app_quitting) { setVisible(FALSE); }
	/*virtual*/ void draw();

	// LLFloaterAuction interface
	static void show();
	
private:
	LLFloaterAuction();
	~LLFloaterAuction();
	void initialize();

	static void onClickSnapshot(void* data);
	static void onClickOK(void* data);

	static LLFloaterAuction* sInstance;

private:
	LLTransactionID mTransactionID;
	LLAssetID mImageID;
	LLPointer<LLImageGL> mImage;
	LLHandle<LLParcelSelection> mParcelp;
	S32 mParcelID;
	LLHost mParcelHost;
};


#endif // LL_LLFLOATERAUCTION_H
