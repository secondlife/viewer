#include "llmessagereader.h"

static BOOL sTimeDecodes = FALSE;

static F32 sTimeDecodesSpamThreshold = 0.05f;

//virtual
LLMessageReader::~LLMessageReader()
{
	// even abstract base classes need a concrete destructor
}

//static 
void LLMessageReader::setTimeDecodes(BOOL b)
{
	sTimeDecodes = b;
}

//static 
void LLMessageReader::setTimeDecodesSpamThreshold(F32 seconds)
{
	sTimeDecodesSpamThreshold = seconds;
}

//static 
BOOL LLMessageReader::getTimeDecodes()
{
	return sTimeDecodes;
}

//static 
F32 LLMessageReader::getTimeDecodesSpamThreshold()
{
	return sTimeDecodesSpamThreshold;
}
