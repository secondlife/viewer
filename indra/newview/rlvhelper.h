#pragma once

#include "rlvdefines.h"

// ============================================================================
// Various helper classes/timers/functors
//

namespace Rlv
{
	struct CommandDbgOut
	{
		CommandDbgOut(const std::string& orig_cmd) : mOrigCmd(orig_cmd) {}
		void add(std::string strCmd, ECmdRet eRet);
		std::string get() const;
		static std::string getDebugVerbFromReturnCode(ECmdRet eRet);
		static std::string getReturnCodeString(ECmdRet eRet);
	private:
		std::string mOrigCmd;
		std::map<ECmdRet, std::string> mCommandResults;
	};
}

// ============================================================================
