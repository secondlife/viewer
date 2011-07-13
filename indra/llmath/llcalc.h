/*
 *  LLCalc.h
 *  SecondLife
 *
 *  Created by Aimee Walton on 28/09/2008.
 *  Copyright 2008 Aimee Walton.
 *
 */

#ifndef LL_CALC_H
#define LL_CALC_H

#include <map>
#include <string>

class LLCalc
{
public:
	LLCalc();
	~LLCalc();

	// Variable name constants
	static const char* X_POS;
	static const char* Y_POS;
	static const char* Z_POS;
	static const char* X_SCALE;
	static const char* Y_SCALE;
	static const char* Z_SCALE;
	static const char* X_ROT;
	static const char* Y_ROT;
	static const char* Z_ROT;
	static const char* HOLLOW;
	static const char* CUT_BEGIN;
	static const char* CUT_END;
	static const char* PATH_BEGIN;
	static const char* PATH_END;
	static const char* TWIST_BEGIN;
	static const char* TWIST_END;
	static const char* X_SHEAR;
	static const char* Y_SHEAR;
	static const char* X_TAPER;
	static const char* Y_TAPER;
	static const char* RADIUS_OFFSET;
	static const char* REVOLUTIONS;
	static const char* SKEW;
	static const char* X_HOLE;
	static const char* Y_HOLE;
	static const char* TEX_U_SCALE;
	static const char* TEX_V_SCALE;
	static const char* TEX_U_OFFSET;
	static const char* TEX_V_OFFSET;
	static const char* TEX_ROTATION;
	static const char* TEX_TRANSPARENCY;
	static const char* TEX_GLOW;

	void	setVar(const std::string& name, const F32& value);
	void	clearVar(const std::string& name);
	void	clearAllVariables();
//	void	updateVariables(LLSD& vars);

	bool	evalString(const std::string& expression, F32& result);
	std::string::size_type	getLastErrorPos()	{ return mLastErrorPos; }
	
	static LLCalc* getInstance();
	static void cleanUp();

	typedef	std::map<std::string, F32> calc_map_t;
	
private:
	std::string::size_type	mLastErrorPos;
	
	calc_map_t	mConstants;
	calc_map_t	mVariables;
	
	// *TODO: Add support for storing user defined variables, and stored functions.
	//	Will need UI work, and a means to save them between sessions.
//	calc_map_t mUserVariables;
	
	// "There shall be only one"
	static LLCalc*	sInstance;
};

#endif // LL_CALC_H
