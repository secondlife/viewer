/** 
 * @file lllivefile.h
 * @brief Automatically reloads a file whenever it changes or is removed.
 *
 * Copyright (c) 2006-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#ifndef LL_LLLIVEFILE_H
#define LL_LLLIVEFILE_H


class LLLiveFile
{
public:
	LLLiveFile(const std::string &filename, const F32 refresh_period = 5.f);
	virtual ~LLLiveFile();

	bool checkAndReload();
		// Returns true if the file changed in any way
		// Call this before using anything that was read & cached from the file

	std::string filename() const;

	void addToEventTimer();
		// Normally, just calling checkAndReload() is enough.  In some cases
		// though, you may need to let the live file periodically check itself.

protected:
	virtual void loadFile() = 0; // Implement this to load your file if it changed

private:
	class Impl;
	Impl& impl;
};

#endif //LL_LLLIVEFILE_H
