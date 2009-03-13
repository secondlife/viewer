/** 
 * @file llTextParser.h
 * @brief GUI for user-defined highlights
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * $/LicenseInfo$
 *
 */

#ifndef LL_LLTEXTPARSER_H
#define LL_LLTEXTPARSER_H

#include <vector>
#include "linden_common.h"

#include "lltextparser.h"

class LLSD;
class LLUUID;
class LLVector3d;
class LLColor4;

class LLTextParser
{
public:
	enum ConditionType { CONTAINS, MATCHES, STARTS_WITH, ENDS_WITH };
	enum HighlightType { PART, ALL };
	enum HighlightPosition { WHOLE, START, MIDDLE, END };
	enum DialogAction  { ACTION_NONE, ACTION_CLOSE, ACTION_ADD, ACTION_COPY, ACTION_UPDATE };

	static LLTextParser* getInstance();
	LLTextParser(){};
	~LLTextParser();

	S32  findPattern(const std::string &text, LLSD highlight);
	LLSD parsePartialLineHighlights(const std::string &text,const LLColor4 &color,S32 part=WHOLE, S32 index=0);
	bool parseFullLineHighlights(const std::string &text, LLColor4 *color);
	void triggerAlerts(LLUUID agent_id, LLVector3d position, std::string text, LLWindow* viewer_window);

	std::string getFileName();
	LLSD loadFromDisk();
	bool saveToDisk(LLSD highlights);
public:
		LLSD	mHighlights;
private:
	static LLTextParser* sInstance;
};

#endif
