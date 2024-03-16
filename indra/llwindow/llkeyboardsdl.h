
#ifndef LL_LLKEYBOARDSDL2_H
#define LL_LLKEYBOARDSDL2_H

#include "llkeyboard.h"
#include "SDL2/SDL.h"

class LLKeyboardSDL : public LLKeyboard
{
public:
    LLKeyboardSDL();
    /*virtual*/ ~LLKeyboardSDL() {};

    /*virtual*/ BOOL	handleKeyUp(const U32 key, MASK mask);
    /*virtual*/ BOOL	handleKeyDown(const U32 key, MASK mask);
    /*virtual*/ void	resetMaskKeys();
    /*virtual*/ MASK	currentMask(BOOL for_mouse_event);
    /*virtual*/ void	scanKeyboard();

protected:
    MASK	updateModifiers(const U32 mask);
    void	setModifierKeyLevel( KEY key, BOOL new_state );
    BOOL	translateNumpadKey( const U32 os_key, KEY *translated_key );
    U16	inverseTranslateNumpadKey(const KEY translated_key);
private:
    std::map<U32, KEY> mTranslateNumpadMap;  // special map for translating OS keys to numpad keys
    std::map<KEY, U32> mInvTranslateNumpadMap; // inverse of the above

public:
    static U32 mapSDL2toWin( U32 );
};

#endif
