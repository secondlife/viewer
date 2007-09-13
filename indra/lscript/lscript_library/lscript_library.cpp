/** 
 * @file lscript_library.cpp
 * @brief external library interface
 *
 * Copyright (c) 2002-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

// *WARNING*
// When adding functions, they <b>MUST</b> be appended to the end of
// the init() method. The init() associates the name with a number,
// which is then serialized into the bytecode. Inserting a new
// function in the middle will lead to many sim crashes. Phoenix 2006-04-10.

#include "linden_common.h"

#include "lscript_library.h"

LLScriptLibrary::LLScriptLibrary()
: mNextNumber(0), mFunctions(NULL)
{
	init();
}

LLScriptLibrary::~LLScriptLibrary()
{
	S32 i;
	for (i = 0; i < mNextNumber; i++)
	{
		delete mFunctions[i];
		mFunctions[i] = NULL;
	}
	delete [] mFunctions;
}

void dummy_func(LLScriptLibData *retval, LLScriptLibData *args, const LLUUID &id)
{
}

void LLScriptLibrary::init()
{
	// IF YOU ADD NEW SCRIPT CALLS, YOU MUST PUT THEM AT THE END OF THIS LIST.
	// Otherwise the bytecode numbers for each call will be wrong, and all
	// existing scripts will crash.

	// energy, sleep, dummy_func, name, return type, parameters, help text, gods-only
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSin", "f", "f", "float llSin(float theta)\ntheta in radians"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCos", "f", "f", "float llCos(float theta)\ntheta in radians"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTan", "f", "f", "float llTan(float theta)\ntheta radians"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAtan2", "f", "ff", "float llAtan2(float y, float x)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSqrt", "f", "f", "float llSqrt(float val)\nreturns 0 and triggers a Math Error for imaginary results"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPow", "f", "ff", "float llPow(float base, float exponent)\nreturns 0 and triggers Math Error for imaginary results"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAbs", "i", "i", "integer llAbs(integer val)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llFabs", "f", "f", "float llFabs(float val)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llFrand", "f", "f", "float llFrand(float mag)\nreturns random number in range [0,mag)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llFloor", "i", "f", "integer llFloor(float val)\nreturns largest integer value <= val"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCeil", "i", "f", "integer llCeil(float val)\nreturns smallest integer value >= val"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRound", "i", "f", "integer llRound(float val)\nreturns val rounded to the nearest integer"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llVecMag", "f", "v", "float llVecMag(vector v)\nreturns the magnitude of v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llVecNorm", "v", "v", "vector llVecNorm(vector v)\nreturns the v normalized"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llVecDist", "f", "vv", "float llVecDist(vector v1, vector v2)\nreturns the 3D distance between v1 and v2"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Euler", "v", "q", "vector llRot2Euler(rotation q)\nreturns the Euler representation (roll, pitch, yaw) of q"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llEuler2Rot", "q", "v", "rotation llEuler2Rot(vector v)\nreturns the rotation representation of Euler Angles v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAxes2Rot", "q", "vvv", "rotation llAxes2Rot(vector fwd, vector left, vector up)\nreturns the rotation defined by the coordinate axes"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Fwd", "v", "q", "vector llRot2Fwd(rotation q)\nreturns the forward vector defined by q"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Left", "v", "q", "vector llRot2Left(rotation q)\nreturns the left vector defined by q"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Up", "v", "q", "vector llRot2Up(rotation q)\nreturns the up vector defined by q"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRotBetween", "q", "vv", "rotation llRotBetween(vector v1, vector v2)\nreturns the rotation to rotate v1 to v2"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llWhisper", NULL, "is", "llWhisper(integer channel, string msg)\nwhispers msg on channel"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSay", NULL, "is", "llSay(integer channel, string msg)\nsays msg on channel"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llShout", NULL, "is", "llShout(integer channel, string msg)\nshouts msg on channel"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListen", "i", "isks", "integer llListen(integer channel, string name, key id, string msg)\nsets a callback for msg on channel from name and id (name, id, and/or msg can be empty) and returns an identifier that can be used to deactivate or remove the listen"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListenControl", NULL, "ii", "llListenControl(integer number, integer active)\nmakes a listen event callback active or inactive"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListenRemove", NULL, "i", "llListenRemove(integer number)\nremoves listen event callback number"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSensor", NULL, "skiff", "llSensor(string name, key id, integer type, float range, float arc)\nPerforms a single scan for name and id with type (AGENT, ACTIVE, PASSIVE, and/or SCRIPTED) within range meters and arc radians of forward vector (name, id, and/or keytype can be empty or 0)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSensorRepeat", NULL, "skifff", "llSensorRepeat(string name, key id, integer type, float range, float arc, float rate)\nsets a callback for name and id with type (AGENT, ACTIVE, PASSIVE, and/or SCRIPTED) within range meters and arc radians of forward vector (name, id, and/or keytype can be empty or 0) and repeats every rate seconds"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSensorRemove", NULL, NULL, "llSensorRemove()\nremoves sensor"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedName", "s", "i", "string llDetectedName(integer number)\nreturns the name of detected object number (returns empty string if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedKey", "k", "i", "key llDetectedKey(integer number)\nreturns the key of detected object number (returns empty key if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedOwner", "k", "i", "key llDetectedOwner(integer number)\nreturns the key of detected object's owner (returns empty key if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedType", "i", "i", "integer llDetectedType(integer number)\nreturns the type (AGENT, ACTIVE, PASSIVE, SCRIPTED) of detected object (returns 0 if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedPos", "v", "i", "vector llDetectedPos(integer number)\nreturns the position of detected object number (returns <0,0,0> if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedVel", "v", "i", "vector llDetectedVel(integer number)\nreturns the velocity of detected object number (returns <0,0,0> if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedGrab", "v", "i", "vector llDetectedGrab(integer number)\nreturns the grab offset of the user touching object (returns <0,0,0> if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedRot", "q", "i", "rotation llDetectedRot(integer number)\nreturns the rotation of detected object number (returns <0,0,0,1> if number is not valid sensed object)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedGroup", "i", "i", "integer llDetectedGroup(integer number)\nReturns TRUE if detected object is part of same group as owner"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetectedLinkNumber", "i", "i", "integer llDetectedLinkNumber(integer number)\nreturns the link position of the triggered event for touches and collisions only"));
	addFunction(new LLScriptLibraryFunction(0.f, 0.f, dummy_func, "llDie", NULL, NULL, "llDie()\ndeletes the object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGround", "f", "v", "float llGround(vector v)\nreturns the ground height below the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCloud", "f", "v", "float llCloud(vector v)\nreturns the cloud density at the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llWind", "v", "v", "vector llWind(vector v)\nreturns the wind velocity at the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetStatus", NULL, "ii", "llSetStatus(integer status, integer value)\nsets status (STATUS_PHYSICS, STATUS_PHANTOM, STATUS_BLOCK_GRAB,\nSTATUS_ROTATE_X, STATUS_ROTATE_Y, and/or STATUS_ROTATE_Z) to value"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetStatus", "i", "i", "integer llGetStatus(integer status)\ngets value of status (STATUS_PHYSICS, STATUS_PHANTOM, STATUS_BLOCK_GRAB,\nSTATUS_ROTATE_X, STATUS_ROTATE_Y, and/or STATUS_ROTATE_Z)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetScale", NULL, "v", "llSetScale(vector scale)\nsets the scale"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetScale", "v", NULL, "vector llGetScale()\ngets the scale"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetColor", NULL, "vi", "llSetColor(vector color, integer face)\nsets the color"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAlpha", "f", "i", "float llGetAlpha(integer face)\ngets the alpha"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetAlpha", NULL, "fi", "llSetAlpha(float alpha, integer face)\nsets the alpha"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetColor", "v", "i", "vector llGetColor(integer face)\ngets the color"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetTexture", NULL, "si", "llSetTexture(string texture, integer face)\nsets the texture of face"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llScaleTexture", NULL, "ffi", "llScaleTexture(float scales, float scalet, integer face)\nsets the texture s, t scales for the chosen face"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llOffsetTexture", NULL, "ffi", "llOffsetTexture(float offsets, float offsett, integer face)\nsets the texture s, t offsets for the chosen face"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llRotateTexture", NULL, "fi", "llRotateTexture(float rotation, integer face)\nsets the texture rotation for the chosen face"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTexture", "s", "i", "string llGetTexture(integer face)\ngets the texture of face (if it's a texture in the object inventory, otherwise the key in a string)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetPos", NULL, "v", "llSetPos(vector pos)\nsets the position (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetPos", "v", NULL, "vector llGetPos()\ngets the position (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLocalPos", "v", NULL, "vector llGetLocalPos()\ngets the position relative to the root (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetRot", NULL, "q", "llSetRot(rotation rot)\nsets the rotation (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRot", "q", NULL, "rotation llGetRot()\ngets the rotation (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLocalRot", "q", NULL, "rotation llGetLocalRot()\ngets the rotation local to the root (if the script isn't physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetForce", NULL, "vi", "llSetForce(vector force, integer local)\nsets force on object, in local coords if local == TRUE (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetForce", "v", NULL, "vector llGetForce()\ngets the force (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTarget", "i", "vf", "integer llTarget(vector position, float range)\nset positions within range of position as a target and return an ID for the target"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTargetRemove", NULL, "i", "llTargetRemove(integer number)\nremoves target number"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRotTarget", "i", "qf", "integer llRotTarget(rotation rot, float error)\nset rotations with error of rot as a rotational target and return an ID for the rotational target"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRotTargetRemove", NULL, "i", "llRotTargetRemove(integer number)\nremoves rotational target number"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llMoveToTarget", NULL, "vf", "llMoveToTarget(vector target, float tau)\ncritically damp to target in tau seconds (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopMoveToTarget", NULL, NULL, "llStopMoveToTarget()\nStops critically damped motion"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llApplyImpulse", NULL, "vi", "llApplyImpulse(vector force, integer local)\napplies impulse to object, in local coords if local == TRUE (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llApplyRotationalImpulse", NULL, "vi", "llApplyRotationalImpulse(vector force, integer local)\napplies rotational impulse to object, in local coords if local == TRUE (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetTorque", NULL, "vi", "llSetTorque(vector torque, integer local)\nsets the torque of object, in local coords if local == TRUE (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTorque", "v", NULL, "vector llGetTorque()\ngets the torque (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetForceAndTorque", NULL, "vvi", "llSetForceAndTorque(vector force, vector torque, integer local)\nsets the force and torque of object, in local coords if local == TRUE (if the script is physical)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetVel", "v", NULL, "vector llGetVel()\ngets the velocity"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAccel", "v", NULL, "vector llGetAccel()\ngets the acceleration"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetOmega", "v", NULL, "vector llGetOmega()\ngets the omega"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTimeOfDay", "f", "", "float llGetTimeOfDay()\ngets the time in seconds since Second Life server midnight (or since server up-time; whichever is smaller)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetWallclock", "f", "", "float llGetWallclock()\ngets the time in seconds since midnight"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTime", "f", NULL, "float llGetTime()\ngets the time in seconds since creation"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llResetTime", NULL, NULL, "llResetTime()\nsets the time to zero"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAndResetTime", "f", NULL, "float llGetAndResetTime()\ngets the time in seconds since creation and sets the time to zero"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSound", NULL, "sfii", "llSound(string sound, float volume, integer queue, integer loop)\nplays sound at volume and whether it should loop or not"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPlaySound", NULL, "sf", "llPlaySound(string sound, float volume)\nplays attached sound once at volume (0.0 - 1.0)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLoopSound", NULL, "sf", "llLoopSound(string sound, float volume)\nplays attached sound looping indefinitely at volume (0.0 - 1.0)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLoopSoundMaster", NULL, "sf", "llLoopSoundMaster(string sound, float volume)\nplays attached sound looping at volume (0.0 - 1.0), declares it a sync master"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLoopSoundSlave", NULL, "sf", "llLoopSoundSlave(string sound, float volume)\nplays attached sound looping at volume (0.0 - 1.0), synced to most audible sync master"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPlaySoundSlave", NULL, "sf", "llPlaySoundSlave(string sound, float volume)\nplays attached sound once at volume (0.0 - 1.0), synced to next loop of most audible sync master"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTriggerSound", NULL, "sf", "llTriggerSound(string sound, float volume)\nplays sound at volume (0.0 - 1.0), centered at but not attached to object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopSound", NULL, "", "llStopSound()\nStops currently attached sound"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llPreloadSound", NULL, "s", "llPreloadSound(string sound)\npreloads a sound on viewers within range"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetSubString", "s", "sii", "string llGetSubString(string src, integer start, integer end)\nreturns the indicated substring"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDeleteSubString", "s", "sii", "string llDeleteSubString(string src, integer start, integer end)\nremoves the indicated substring and returns the result"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llInsertString", "s", "sis", "string llInsertString(string dst, integer position, string src)\ninserts src into dst at position and returns the result"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llToUpper", "s", "s", "string llToUpper(string src)\nconvert src to all upper case and returns the result"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llToLower", "s", "s", "string llToLower(string src)\nconvert src to all lower case and returns the result"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGiveMoney", "i", "ki", "llGiveMoney(key destination, integer amount)\ntransfer amount of money from script owner to destination"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llMakeExplosion", NULL, "iffffsv", "llMakeExplosion(integer particles, float scale, float vel, float lifetime, float arc, string texture, vector offset)\nMake a round explosion of particles"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llMakeFountain", NULL, "iffffisvf", "llMakeFountain(integer particles, float scale, float vel, float lifetime, float arc, integer bounce, string texture, vector offset, float bounce_offset)\nMake a fountain of particles"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llMakeSmoke", NULL, "iffffsv", "llMakeSmoke(integer particles, float scale, float vel, float lifetime, float arc, string texture, vector offset)\nMake smoke like particles"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llMakeFire", NULL, "iffffsv", "llMakeFire(integer particles, float scale, float vel, float lifetime, float arc, string texture, vector offset)\nMake fire like particles"));
	addFunction(new LLScriptLibraryFunction(200.f, 0.1f, dummy_func, "llRezObject", NULL, "svvqi", "llRezObject(string inventory, vector pos, vector vel, rotation rot, integer param)\nInstanciate owners inventory object at pos with velocity vel and rotation rot with start parameter param"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLookAt", NULL, "vff", "llLookAt(vector target, F32 strength, F32 damping)\nCause object name to point it's forward axis towards target"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopLookAt", NULL, NULL, "llStopLookAt()\nStop causing object name to point at a target"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetTimerEvent", NULL, "f", "llSetTimerEvent(float sec)\nCause the timer event to be triggered every sec seconds"));
	addFunction(new LLScriptLibraryFunction(0.f, 0.f, dummy_func, "llSleep", NULL, "f", "llSleep(float sec)\nPut script to sleep for sec seconds"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetMass", "f", NULL, "float llGetMass()\nGet the mass of task name that script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCollisionFilter", NULL, "ski", "llCollisionFilter(string name, key id, integer accept)\nif accept == TRUE, only accept collisions with objects name and id (either is optional), otherwise with objects not name or id"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTakeControls", NULL, "iii", "llTakeControls(integer controls, integer accept, integer pass_on)\nTake controls from agent task has permissions for.  If (accept == (controls & input)), send input to task.  If pass_on send to agent also."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llReleaseControls", NULL, NULL, "llReleaseControls()\nStop taking inputs"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAttachToAvatar", NULL, "i", "llAttachToAvatar(integer attachment)\nAttach to avatar task has permissions for at point attachment"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDetachFromAvatar", NULL, NULL, "llDetachFromAvatar()\nDrop off of avatar"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTakeCamera", NULL, "k", "llTakeCamera(key avatar)\nMove avatar's viewpoint to task"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llReleaseCamera", NULL, "k", "llReleaseCamera(key avatar)\nReturn camera to agent"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetOwner", "k", NULL, "key llGetOwner()\nReturns the owner of the task"));
	addFunction(new LLScriptLibraryFunction(10.f, 2.f, dummy_func, "llInstantMessage", NULL, "ks", "llInstantMessage(key user, string message)\nIMs message to the user"));
	addFunction(new LLScriptLibraryFunction(10.f, 20.f, dummy_func, "llEmail", NULL, "sss", "llEmail(string address, string subject, string message)\nSends email to address with subject and message"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetNextEmail", NULL, "ss", "llGetNextEmail(string address, string subject)\nGet the next waiting email with appropriate address and/or subject (if blank they are ignored)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetKey", "k", NULL, "key llGetKey()\nGet the key for the task the script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetBuoyancy", NULL, "f", "llSetBuoyancy(float buoyancy)\nSet the tasks buoyancy (0 is none, < 1.0 sinks, 1.0 floats, > 1.0 rises)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetHoverHeight", NULL, "fif", "llSetHoverHeight(float height, integer water, float tau)\nCritically damps to a height (either above ground level or above the higher of land and water if water == TRUE)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopHover", NULL, NULL, "llStopHover()\nStop hovering to a height"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llMinEventDelay", NULL, "f", "llMinEventDelay(float delay)\nSet the minimum time between events being handled"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSoundPreload", NULL, "s", "llSoundPreload(string sound)\npreloads a sound on viewers within range"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRotLookAt", NULL, "qff", "llRotLookAt(rotation target, F32 strength, F32 damping)\nCause object name to point it's forward axis towards target"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStringLength", "i", "s", "integer llStringLength(string str)\nReturns the length of string"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStartAnimation", NULL, "s", "llStartAnimation(string anim)\nStart animation anim for agent that owns object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopAnimation", NULL, "s", "llStopAnimation(string anim)\nStop animation anim for agent that owns object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPointAt", NULL, "v", "llPointAt(vector pos)\nMake agent that owns object point at pos"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStopPointAt", NULL, NULL, "llStopPointAt()\nStop agent that owns object pointing"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTargetOmega", NULL, "vff", "llTargetOmega(vector axis, float spinrate, float gain)\nAttempt to spin at spinrate with strength gain"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetStartParameter", "i", NULL, "integer llGetStartParameter()\nGet's the start paramter passed to llRezObject"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGodLikeRezObject", NULL, "kv", "llGodLikeRezObject(key inventory, vector pos)\nrez directly off of a UUID if owner has dog-bit set", TRUE));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRequestPermissions", NULL, "ki", "llRequestPermissions(key agent, integer perm)\nask agent to allow the script to do perm (NB: Debit, ownership, link, joint, and permission requests can only go to the task's owner)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetPermissionsKey", "k", NULL, "key llGetPermissionsKey()\nReturn agent that permissions are enabled for.  NULL_KEY if not enabled"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetPermissions", "i", NULL, "integer llGetPermissions()\nreturn what permissions have been enabled"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLinkNumber", "i", NULL, "integer llGetLinkNumber()\nReturns what number in a link set the script is attached to (0 means no link, 1 the root, 2 for first child, &c)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetLinkColor", NULL, "ivi", "llSetLinkColor(integer linknumber, vector color, integer face)\nIf a task exists in the link chain at linknumber, set face to color"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llCreateLink", NULL, "ki", "llCreateLink(key target, integer parent)\nAttempt to link task script is attached to and target (requires permission PERMISSION_CHANGE_LINKS be set). If parent == TRUE, task script is attached to is the root"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llBreakLink", NULL, "i", "llBreakLink(integer linknum)\nDelinks the task with the given link number (requires permission PERMISSION_CHANGE_LINKS be set)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llBreakAllLinks", NULL, NULL, "llBreakAllLinks()\nDelinks all tasks in the link set (requires permission PERMISSION_CHANGE_LINKS be set)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLinkKey", "k", "i", "key llGetLinkKey(integer linknum)\nGet the key of linknumber in link set"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLinkName", "s", "i", "string llGetLinkName(integer linknum)\nGet the name of linknumber in link set"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryNumber", "i", "i", "integer llGetInventoryNumber(integer type)\nGet the number of items of a given type in the task's inventory.\nValid types: INVENTORY_TEXTURE, INVENTORY_SOUND, INVENTORY_OBJECT, INVENTORY_SCRIPT, INVENTORY_CLOTHING, INVENTORY_BODYPART, INVENTORY_NOTECARD, INVENTORY_LANDMARK, INVENTORY_ALL"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryName", "s", "ii", "string llGetInventoryName(integer type, integer number)\nGet the name of the inventory item number of type"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetScriptState", NULL, "si", "llSetScriptState(string name, integer run)\nControl the state of a script name."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetEnergy", "f", NULL, "float llGetEnergy()\nReturns how much energy is in the object as a percentage of maximum"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGiveInventory", NULL, "ks", "llGiveInventory(key destination, string inventory)\nGive inventory to destination"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRemoveInventory", NULL, "s", "llRemoveInventory(string inventory)\nRemove the named inventory item"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetText", NULL, "svf", "llSetText(string text, vector color, float alpha)\nSet text floating over object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llWater", "f", "v", "float llWater(vector v)\nreturns the water height below the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPassTouches", NULL, "i", "llPassTouches(integer pass)\nif pass == TRUE, touches are passed from children on to parents (default is FALSE)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llRequestAgentData", "k", "ki", "key llRequestAgentData(key id, integer data)\nRequests data about agent id.  When data is available the dataserver event will be raised"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llRequestInventoryData", "k", "s", "key llRequestInventoryData(string name)\nRequests data from object's inventory object.  When data is available the dataserver event will be raised"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetDamage", NULL, "f", "llSetDamage(float damage)\nSets the amount of damage that will be done to an object that this task hits.  Task will be killed."));
	addFunction(new LLScriptLibraryFunction(100.f, 5.f, dummy_func, "llTeleportAgentHome", NULL, "k", "llTeleportAgentHome(key id)\nTeleports agent on owner's land to agent's home location"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llModifyLand", NULL, "ii", "llModifyLand(integer action, integer size)\nModify land with action (LAND_LEVEL, LAND_RAISE, LAND_LOWER, LAND_SMOOTH, LAND_NOISE, LAND_REVERT)\non size (LAND_SMALL_BRUSH, LAND_MEDIUM_BRUSH, LAND_LARGE_BRUSH)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCollisionSound", NULL, "sf", "llCollisionSound(string impact_sound, float impact_volume)\nSuppress default collision sounds, replace default impact sounds with impact_sound (empty string to just suppress)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCollisionSprite", NULL, "s", "llCollisionSprite(string impact_sprite)\nSuppress default collision sprites, replace default impact sprite with impact_sprite (empty string to just suppress)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAnimation", "s", "k", "string llGetAnimation(key id)\nGet the currently playing locomotion animation for avatar id"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llResetScript", NULL, NULL, "llResetScript()\nResets the script"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llMessageLinked", NULL, "iisk", "llMessageLinked(integer linknum, integer num, string str, key id)\nSends num, str, and id to members of the link set (LINK_ROOT sends to root task in a linked set,\nLINK_SET sends to all tasks,\nLINK_ALL_OTHERS to all other tasks,\nLINK_ALL_CHILDREN to all children,\nLINK_THIS to the task the script it is in)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPushObject", NULL, "kvvi", "llPushObject(key id, vector impulse, vector ang_impulse, integer local)\nApplies impulse and ang_impulse to object id"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llPassCollisions", NULL, "i", "llPassCollisions(integer pass)\nif pass == TRUE, collisions are passed from children on to parents (default is FALSE)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetScriptName", "s", NULL, "llGetScriptName()\nReturns the script name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetNumberOfSides", "i", NULL, "integer llGetNumberOfSides()\nReturns the number of sides"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAxisAngle2Rot", "q", "vf", "rotation llAxisAngle2Rot(vector axis, float angle)\nReturns the rotation generated angle about axis"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Axis", "v", "q", "vector llRot2Axis(rotation rot)\nReturns the rotation axis represented by rot"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRot2Angle", "f", "q", "float llRot2Angle(rotation rot)\nReturns the rotation angle represented by rot"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAcos", "f", "f", "float llAcos(float val)\nReturns the arccosine in radians of val"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAsin", "f", "f", "float llAsin(float val)\nReturns the arcsine in radians of val"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAngleBetween", "f", "qq", "float llAngleBetween(rotation a, rotation b)\nReturns angle between rotation a and b"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryKey", "k", "s", "key llGetInventoryKey(string name)\nReturns the key of the inventory name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAllowInventoryDrop", NULL, "i", "llAllowInventoryDrop(integer add)\nIf add == TRUE, users without permissions can still drop inventory items onto task"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetSunDirection", "v", NULL, "vector llGetSunDirection()\nReturns the sun direction on the simulator"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTextureOffset", "v", "i", "vector llGetTextureOffset(integer side)\nReturns the texture offset of side in the x and y components of a vector"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTextureScale", "v", "i", "vector llGetTextureScale(integer side)\nReturns the texture scale of side in the x and y components of a vector"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTextureRot", "f", "i", "float llGetTextureRot(integer side)\nReturns the texture rotation of side"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSubStringIndex", "i", "ss", "integer llSubStringIndex(string source, string pattern)\nFinds index in source where pattern first appears (returns -1 if not found)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetOwnerKey", "k", "k", "key llGetOwnerKey(key id)\nFind the owner of id"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetCenterOfMass", "v", NULL, "vector llGetCenterOfMass()\nGet the object's center of mass"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListSort", "l", "lii", "list llListSort(list src, integer stride, integer ascending)\nSort the list into blocks of stride in ascending order if ascending == TRUE.  Note that sort only works between same types."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetListLength", "i", "l", "integer llGetListLength(list src)\nGet the number of elements in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2Integer", "i", "li", "integer llList2Integer(list src, integer index)\nCopy the integer at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2Float", "f", "li", "float llList2Float(list src, integer index)\nCopy the float at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2String", "s", "li", "string llList2String(list src, integer index)\nCopy the string at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2Key", "k", "li", "key llList2Key(list src, integer index)\nCopy the key at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2Vector", "v", "li", "vector llList2Vector(list src, integer index)\nCopy the vector at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2Rot", "q", "li", "rotation llList2Rot(list src, integer index)\nCopy the rotation at index in the list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2List", "l", "lii", "list llList2List(list src, integer start, integer end)\nCopy the slice of the list from start to end"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDeleteSubList", "l", "lii", "list llDeleteSubList(list src, integer start, integer end)\nRemove the slice from the list and return the remainder"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetListEntryType", "i", "li", "integer llGetListEntryType(list src, integer index)\nReturns the type of the index entry in the list\n(TYPE_INTEGER, TYPE_FLOAT, TYPE_STRING, TYPE_KEY, TYPE_VECTOR, TYPE_ROTATION, or TYPE_INVALID if index is off list)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2CSV", "s", "l", "string llList2CSV(list src)\nCreate a string of comma separated values from list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llCSV2List", "l", "s", "list llCSV2List(string src)\nCreate a list from a string of comma separated values"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListRandomize", "l", "li", "list llListRandomize(list src, integer stride)\nReturns a randomized list of blocks of size stride"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llList2ListStrided", "l", "liii", "list llList2ListStrided(list src, integer start, integer end, integer stride)\nCopy the strided slice of the list from start to end"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRegionCorner", "v", NULL, "vector llGetRegionCorner()\nReturns a vector with the south west corner x,y position of the region the object is in"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListInsertList", "l", "lli", "list llListInsertList(list dest, list src, integer start)\nInserts src into dest at position start"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListFindList", "i", "ll", "integer llListFindList(list src, list test)\nReturns the start of the first instance of test in src, -1 if not found"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectName", "s", NULL, "string llGetObjectName()\nReturns the name of the object script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetObjectName", NULL, "s", "llSetObjectName(string name)\nSets the objects name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetDate", "s", NULL, "string llGetDate()\nGets the date as YYYY-MM-DD"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llEdgeOfWorld", "i", "vv", "integer llEdgeOfWorld(vector pos, vector dir)\nChecks to see whether the border hit by dir from pos is the edge of the world (has no neighboring simulator)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAgentInfo", "i", "k", "integer llGetAgentInfo(key id)\nGets information about agent ID.\nReturns AGENT_FLYING, AGENT_ATTACHMENTS, AGENT_SCRIPTED, AGENT_SITTING, AGENT_ON_OBJECT, AGENT_MOUSELOOK, AGENT_AWAY, AGENT_BUSY, AGENT_TYPING, AGENT_CROUCHING, AGENT_ALWAYS_RUN, AGENT_WALKING and/or AGENT_IN_AIR."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llAdjustSoundVolume", NULL, "f", "llAdjustSoundVolume(float volume)\nadjusts volume of attached sound (0.0 - 1.0)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetSoundQueueing", NULL, "i", "llSetSoundQueueing(integer queue)\ndetermines whether attached sound calls wait for the current sound to finish (0 = no [default], nonzero = yes)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetSoundRadius", NULL, "f", "llSetSoundRadius(float radius)\nestablishes a hard cut-off radius for audibility of scripted sounds (both attached and triggered)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llKey2Name", "s", "k", "string llKey2Name(key id)\nReturns the name of the object key, iff the object is in the current simulator, otherwise the empty string"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetTextureAnim", NULL, "iiiifff", "llSetTextureAnim(integer mode, integer face, integer sizex, integer sizey, float start, float length, float rate)\nAnimate the texture on the specified face/faces"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llTriggerSoundLimited", NULL, "sfvv", "llTriggerSoundLimited(string sound, float volume, vector tne, vector bsw)\nplays sound at volume (0.0 - 1.0), centered at but not attached to object, limited to AABB defined by vectors top-north-east and bottom-south-west"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llEjectFromLand", NULL, "k", "llEjectFromLand(key pest)\nEjects pest from land that you own"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llParseString2List", "l", "sll", "list llParseString2List(string src, list separators, list spacers)\nBreaks src into a list, discarding separators, keeping spacers (separators and spacers must be lists of strings, maximum of 8 each)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llOverMyLand", "i", "k", "integer llOverMyLand(key id)\nReturns TRUE if id is over land owner of object owns, FALSE otherwise"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetLandOwnerAt", "k", "v", "key llGetLandOwnerAt(vector pos)\nReturns the key of the land owner, NULL_KEY if public"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llGetNotecardLine", "k", "si", "key llGetNotecardLine(string name, integer line)\nReturns line line of notecard name via the dataserver event"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAgentSize", "v", "k", "vector llGetAgentSize(key id)\nIf the agent is in the same sim as the object, returns the size of the avatar"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSameGroup", "i", "k", "integer llSameGroup(key id)\nReturns TRUE if ID is in the same sim and has the same active group, otherwise FALSE"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llUnSit", NULL, "k", "key llUnSit(key id)\nIf agent identified by id is sitting on the object the script is attached to or is over land owned by the objects owner, the agent is forced to stand up"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGroundSlope", "v", "v", "vector llGroundSlope(vector v)\nreturns the ground slope below the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGroundNormal", "v", "v", "vector llGroundNormal(vector v)\nreturns the ground normal below the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGroundContour", "v", "v", "vector llGroundCountour(vector v)\nreturns the ground contour below the object position + v"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAttached", "i", NULL, "integer llGetAttached()\nreturns the object attachment point or 0 if not attached"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetFreeMemory", "i", NULL, "integer llGetFreeMemory()\nreturns the available heap space for the current script"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRegionName", "s", NULL, "string llGetRegionName()\nreturns the current region name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRegionTimeDilation", "f", NULL, "float llGetRegionTimeDilation()\nreturns the current time dilation as a float between 0 and 1"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRegionFPS", "f", NULL, "float llGetRegionFPS()\nreturns the mean region frames per second"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llParticleSystem", NULL, "l", "llParticleSystem(list rules)\nCreates a particle system based on rules.  Empty list removes particle system from object.\nList format is [ rule1, data1, rule2, data2 . . . rulen, datan ]"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGroundRepel", NULL, "fif", "llGroundRepel(float height, integer water, float tau)\nCritically damps to height if within height*0.5 of level (either above ground level or above the higher of land and water if water == TRUE)"));
	addFunction(new LLScriptLibraryFunction(10.f, 3.f, dummy_func, "llGiveInventoryList", NULL, "ksl", "llGiveInventoryList(key destination, string category, list inventory)\nGive inventory to destination in a new category"));

// script calls for vehicle action
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetVehicleType", NULL, "i", "llSetVehicleType(integer type)\nsets vehicle to one of the default types"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetVehicleFloatParam", NULL, "if", "llSetVehicleFloatParam(integer param, float value)\nsets the specified vehicle float parameter"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetVehicleVectorParam", NULL, "iv", "llSetVehicleVectorParam(integer param, vector vec)\nsets the specified vehicle vector parameter"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetVehicleRotationParam", NULL, "iq", "llSetVehicleVectorParam(integer param, rotation rot)\nsets the specified vehicle rotation parameter"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetVehicleFlags", NULL, "i", "llSetVehicleFlags(integer flags)\nsets the enabled bits in 'flags'"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRemoveVehicleFlags", NULL, "i", "llRemoveVehicleFlags(integer flags)\nremoves the enabled bits in 'flags'"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSitTarget", NULL, "vq", "llSitTarget(vector offset, rotation rot)\nSet the sit location for this object (if offset == <0,0,0> clear it)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llAvatarOnSitTarget", "k", NULL, "key llAvatarOnSitTarget()\nIf an avatar is sitting on the sit target, return the avatar's key, NULL_KEY otherwise"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llAddToLandPassList", NULL, "kf", "llAddToLandPassList(key avatar, float hours)\nAdd avatar to the land pass list for hours"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetTouchText", NULL, "s", "llSetTouchText(string text)\nDisplays text in pie menu that acts as a touch"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetSitText", NULL, "s", "llSetSitText(string text)\nDisplays text rather than sit in pie menu"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCameraEyeOffset", NULL, "v", "llSetCameraEyeOffset(vector offset)\nSets the camera eye offset used in this object if an avatar sits on it"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCameraAtOffset", NULL, "v", "llSetCameraAtOffset(vector offset)\nSets the camera at offset used in this object if an avatar sits on it"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llDumpList2String", "s", "ls", "string llDumpList2String(list src, string separator)\nWrite the list out in a single string using separator between values"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llScriptDanger", "i", "v", "integer llScriptDanger(vector pos)\nReturns true if pos is over public land, sandbox land, land that doesn't allow everyone to edit and build, or land that doesn't allow outside scripts"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llDialog", NULL, "ksli", "llDialog(key avatar, string message, list buttons, integer chat_channel\nShows a dialog box on the avatar's screen with the message.\nUp to 12 strings in the list form buttons.\nIf a button is clicked, the name is chatted on chat_channel."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llVolumeDetect", NULL, "i", "llVolumeDetect(integer detect)\nIf detect = TRUE, object becomes phantom but triggers collision_start and collision_end events\nwhen other objects start and stop interpenetrating.\nMust be applied to the root object."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llResetOtherScript", NULL, "s", "llResetOtherScript(string name)\nResets script name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetScriptState", "i", "s", "integer llGetScriptState(string name)\nResets TRUE if script name is running"));
	addFunction(new LLScriptLibraryFunction(10.f, 3.f, dummy_func, "llRemoteLoadScript", NULL, "ksii", "Deprecated.  Please use llRemoteLoadScriptPin instead."));

	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetRemoteScriptAccessPin", NULL, "i", "llSetRemoteScriptAccessPin(integer pin)\nIf pin is set to a non-zero number, the task will accept remote script\nloads via llRemoteLoadScriptPin if it passes in the correct pin.\nOthersise, llRemoteLoadScriptPin is ignored."));
	addFunction(new LLScriptLibraryFunction(10.f, 3.f, dummy_func, "llRemoteLoadScriptPin", NULL, "ksiii", "llRemoteLoadScriptPin(key target, string name, integer pin, integer running, integer start_param)\nIf the owner of the object this script is attached can modify target,\nthey are in the same region,\nand the matching pin is used,\ncopy script name onto target,\nif running == TRUE, start the script with param."));
	
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llOpenRemoteDataChannel", NULL, NULL, "llOpenRemoteDataChannel()\nCreates a channel to listen for XML-RPC calls.  Will trigger a remote_data event with channel id once it is available."));
	addFunction(new LLScriptLibraryFunction(10.f, 3.f, dummy_func, "llSendRemoteData", "k", "ksis", "key llSendRemoteData(key channel, string dest, integer idata, string sdata)\nSend an XML-RPC request to dest through channel with payload of channel (in a string), integer idata and string sdata.\nA message identifier key is returned.\nAn XML-RPC reply will trigger a remote_data event and reference the message id.\nThe message_id is returned."));
	addFunction(new LLScriptLibraryFunction(10.f, 3.f, dummy_func, "llRemoteDataReply", NULL, "kksi", "llRemoteDataReply(key channel, key message_id, string sdata, integer idata)\nSend an XML-RPC reply to message_id on channel with payload of string sdata and integer idata"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llCloseRemoteDataChannel", NULL, "k", "llCloseRemoteDataChannel(key channel)\nCloses XML-RPC channel."));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llMD5String", "s", "si", "string llMD5String(string src, integer nonce)\nPerforms a RSA Data Security, Inc. MD5 Message-Digest Algorithm on string with nonce.  Returns a 32 character hex string."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetPrimitiveParams", NULL, "l", "llSetPrimitiveParams(list rules)\nSet primitive parameters based on rules."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStringToBase64", "s", "s", "string llStringToBase64(string str)\nConverts a string to the Base 64 representation of the string."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llBase64ToString", "s", "s", "string llBase64ToString(string str)\nConverts a Base 64 string to a conventional string.  If the conversion creates any unprintable characters, they are converted to spaces."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.3f, dummy_func, "llXorBase64Strings", "s", "ss", "string llXorBase64Strings(string s1, string s2)\nDEPRECATED!  Please use llXorBase64StringsCorrect instead!!  Incorrectly performs an exclusive or on two Base 64 strings and returns a Base 64 string.  s2 repeats if it is shorter than s1.  Retained for backwards compatability."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRemoteDataSetRegion", NULL, NULL, "llRemoteDataSetRegion()\nIf an object using remote data channels changes regions, you must call this function to reregister the remote data channels.\nYou do not need to make this call if you don't change regions."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLog10", "f", "f", "float llLog10(float val)\nReturns the base 10 log of val if val > 0, otherwise returns 0."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llLog", "f", "f", "float llLog(float val)\nReturns the base e log of val if val > 0, otherwise returns 0."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetAnimationList", "l", "k", "list llGetAnimationList(key id)\nGets a list of all playing animations for avatar id"));
	addFunction(new LLScriptLibraryFunction(10.f, 2.f, dummy_func, "llSetParcelMusicURL", NULL, "s", "llSetParcelMusicURL(string url)\nSets the streaming audio URL for the parcel object is on"));
	
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRootPosition", "v", NULL, "vector llGetRootPosition()\nGets the global position of the root object of the object script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRootRotation", "q", NULL, "rotation llGetRootRotation()\nGets the global rotation of the root object of the object script is attached to"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectDesc", "s", NULL, "string llGetObjectDesc()\nReturns the description of the object the script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetObjectDesc", NULL, "s", "llSetObjectDesc(string name)\nSets the object's description"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetCreator", "k", NULL, "key llGetCreator()\nReturns the creator of the object"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetTimestamp", "s", NULL, "string llGetTimestamp()\nGets the timestamp in the format: YYYY-MM-DDThh:mm:ss.ff..fZ"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetLinkAlpha", NULL, "ifi", "llSetLinkAlpha(integer linknumber, float alpha, integer face)\nIf a prim exists in the link chain at linknumber, set face to alpha"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetNumberOfPrims", "i", NULL, "integer llGetNumberOfPrims()\nReturns the number of prims in a link set the script is attached to"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llGetNumberOfNotecardLines", "k", "s", "key llGetNumberOfNotecardLines(string name)\nReturns number of lines in notecard 'name' via the dataserver event (cast return value to integer)"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetBoundingBox", "l", "k", "list llGetBoundingBox(key object)\nReturns the bounding box around an object (including any linked prims) relative to the root prim, in a list:  [ (vector) min_corner, (vector) max_corner ]"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetGeometricCenter", "v", NULL, "vector llGetGeometricCenter()\nReturns the geometric center of the linked set the script is attached to."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llGetPrimitiveParams", "l", "l", "list llGetPrimitiveParams(list params)\nGets primitive parameters specified in the params list."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.0f, dummy_func, "llIntegerToBase64", "s", "i", "string llIntegerToBase64(integer number)\nBig endian encode of of integer as a Base64 string."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.0f, dummy_func, "llBase64ToInteger", "i", "s", "integer llBase64ToInteger(string str)\nBig endian decode of a Base64 string into an integer."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetGMTclock", "f", "", "float llGetGMTclock()\nGets the time in seconds since midnight in GMT"));
	addFunction(new LLScriptLibraryFunction(10.f, 10.f, dummy_func, "llGetSimulatorHostname", "s", "", "string llGetSimulatorHostname()\nGets the hostname of the machine script is running on (same as string in viewer Help dialog)"));
	
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetLocalRot", NULL, "q", "llSetLocalRot(rotation rot)\nsets the rotation of a child prim relative to the root prim"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llParseStringKeepNulls", "l", "sll", "list llParseStringKeepNulls(string src, list separators, list spacers)\nBreaks src into a list, discarding separators, keeping spacers (separators and spacers must be lists of strings, maximum of 8 each), keeping any null values generated."));
	addFunction(new LLScriptLibraryFunction(200.f, 0.1f, dummy_func, "llRezAtRoot", NULL, "svvqi", "llRezAtRoot(string inventory, vector pos, vector vel, rotation rot, integer param)\nInstantiate owner's inventory object at pos with velocity vel and rotation rot with start parameter param.\nThe last selected root object's location will be set to pos"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectPermMask", "i", "i", "integer llGetObjectPermMask(integer mask)\nReturns the requested permission mask for the root object the task is attached to.", FALSE));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetObjectPermMask", NULL, "ii", "llSetObjectPermMask(integer mask, integer value)\nSets the given permission mask to the new value on the root object the task is attached to.", TRUE));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryPermMask", "i", "si", "integer llGetInventoryPermMask(string item, integer mask)\nReturns the requested permission mask for the inventory item.", FALSE));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetInventoryPermMask", NULL, "sii", "llSetInventoryPermMask(string item, integer mask, integer value)\nSets the given permission mask to the new value on the inventory item.", TRUE));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryCreator", "k", "s", "key llGetInventoryCreator(string item)\nReturns the key for the creator of the inventory item.", FALSE));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llOwnerSay", NULL, "s", "llOwnerSay(string msg)\nsays msg to owner only (if owner in sim)"));
	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llRequestSimulatorData", "k", "si", "key llRequestSimulatorData(string simulator, integer data)\nRequests data about simulator.  When data is available the dataserver event will be raised"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llForceMouselook", NULL, "i", "llForceMouselook(integer mouselook)\nIf mouselook is TRUE any avatar that sits on this object is forced into mouselook mode"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectMass", "f", "k", "float llGetObjectMass(key id)\nGet the mass of the object with key id"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListReplaceList", "l", "llii", "list llListReplaceList(list dest, list src, integer start, integer end)\nReplaces start through end of dest with src."));
	addFunction(new LLScriptLibraryFunction(10.f, 10.f, dummy_func, "llLoadURL", NULL, "kss", "llLoadURL(key avatar_id, string message, string url)\nShows dialog to avatar avatar_id offering to load web page at URL.  If user clicks yes, launches their web browser."));

	addFunction(new LLScriptLibraryFunction(10.f, 2.f, dummy_func, "llParcelMediaCommandList", NULL, "l", "llParcelMediaCommandList(list command)\nSends a list of commands, some with arguments, to a parcel."));
	addFunction(new LLScriptLibraryFunction(10.f, 2.f, dummy_func, "llParcelMediaQuery", "l", "l", "list llParcelMediaQuery(list query)\nSends a list of queries, returns a list of results."));

	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llModPow", "i", "iii", "integer llModPow(integer a, integer b, integer c)\nReturns a raised to the b power, mod c. ( (a**b)%c ).  b is capped at 0xFFFF (16 bits)."));
	
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetInventoryType", "i", "s", "integer llGetInventoryType(string name)\nReturns the type of the inventory name"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetPayPrice", NULL, "il", "llSetPayPrice(integer price, list quick_pay_buttons)\nSets the default amount when someone chooses to pay this object."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetCameraPos", "v", "", "vector llGetCameraPos()\nGets current camera position for agent task has permissions for."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetCameraRot", "q", "", "rotation llGetCameraRot()\nGets current camera orientation for agent task has permissions for."));
	
	addFunction(new LLScriptLibraryFunction(10.f, 2.f, dummy_func, "llSetPrimURL", NULL, "s", "llSetPrimURL(string url)\nUpdates the URL for the web page shown on the sides of the object."));
	addFunction(new LLScriptLibraryFunction(10.f, 20.f, dummy_func, "llRefreshPrimURL", NULL, "", "llRefreshPrimURL()\nReloads the web page shown on the sides of the object."));
	
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llEscapeURL", "s", "s", "string llEscapeURL(string url)\nReturns and escaped/encoded version of url, replacing spaces with %20 etc."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llUnescapeURL", "s", "s", "string llUnescapeURL(string url)\nReturns and unescaped/unencoded version of url, replacing %20 with spaces etc."));

	addFunction(new LLScriptLibraryFunction(10.f, 1.f, dummy_func, "llMapDestination", NULL, "svv", "llMapDestination(string simname, vector pos, vector look_at)\nOpens world map centered on region with pos highlighted.\nOnly works for scripts attached to avatar, or during touch events.\n(NOTE: look_at currently does nothing)"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llAddToLandBanList", NULL, "kf", "llAddToLandBanList(key avatar, float hours)\nAdd avatar to the land ban list for hours"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llRemoveFromLandPassList", NULL, "k", "llRemoveFromLandPassList(key avatar)\nRemove avatar from the land pass list"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llRemoveFromLandBanList", NULL, "k", "llRemoveFromLandBanList(key avatar)\nRemove avatar from the land ban list"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCameraParams",			 NULL, "l", "llSetCameraParams(list rules)\nSets multiple camera parameters at once.\nList format is [ rule1, data1, rule2, data2 . . . rulen, datan ]"));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llClearCameraParams",		 NULL, NULL, "llClearCameraParams()\nResets all camera parameters to default values and turns off scripted camera control."));
	
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llListStatistics", "f", "il", "float llListStatistics(integer operation, list l)\nPerform statistical aggregate functions on list l using LIST_STAT_* operations."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetUnixTime", "i", NULL, "integer llGetUnixTime()\nGet the number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC from the system clock."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetParcelFlags", "i", "v", "integer llGetParcelFlags(vector pos)\nGet the parcel flags (PARCEL_FLAG_*) for the parcel including the point pos."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetRegionFlags", "i", NULL, "integer llGetRegionFlags()\nGet the region flags (REGION_FLAG_*) for the region the object is in."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llXorBase64StringsCorrect", "s", "ss", "string llXorBase64StringsCorrect(string s1, string s2)\nCorrectly performs an exclusive or on two Base 64 strings and returns a Base 64 string.  s2 repeats if it is shorter than s1."));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llHTTPRequest", "k", "sls", "llHTTPRequest(string url, list parameters, string body)\nSend an HTTP request."));

	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llResetLandBanList", NULL, NULL, "llResetLandBanList()\nRemoves all residents from the land ban list."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.1f, dummy_func, "llResetLandPassList", NULL, NULL, "llResetLandPassList()\nRemoves all residents from the land access/pass list."));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectPrimCount", "i", "k", "integer llGetObjectPrimCount(key object_id)\nReturns the total number of prims for an object."));
	addFunction(new LLScriptLibraryFunction(10.f, 2.0f, dummy_func, "llGetParcelPrimOwners", "l", "v", "list llGetParcelPrimOwners(vector pos)\nReturns a list of all residents who own objects on the parcel and the number of objects they own.\nRequires owner-like permissions for the parcel."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetParcelPrimCount", "i", "vii","integer llGetParcelPrimCount(vector pos, integer category, integer sim_wide)\nGets the number of prims on the parcel of the given category.\nCategories: PARCEL_COUNT_TOTAL, _OWNER, _GROUP, _OTHER, _SELECTED, _TEMP."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetParcelMaxPrims", "i", "vi","integer llGetParcelMaxPrims(vector pos, integer sim_wide)\nGets the maximum number of prims allowed on the parcel at pos."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetParcelDetails", "l", "vl","list llGetParcelDetails(vector pos, list params)\nGets the parcel details specified in params for the parcel at pos.\nParams is one or more of: PARCEL_DETAILS_NAME, _DESC, _OWNER, _GROUP, _AREA"));


	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetLinkPrimitiveParams", NULL, "il", "llSetLinkPrimitiveParams(integer linknumber, list rules)\nSet primitive parameters for linknumber based on rules."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.2f, dummy_func, "llSetLinkTexture", NULL, "isi", "llSetLinkTexture(integer link_pos, string texture, integer face)\nSets the texture of face for link_pos"));

	
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llStringTrim", "s", "si", "string llStringTrim(string src, integer trim_type)\nTrim leading and/or trailing spaces from a string.\nUses trim_type of STRING_TRIM, STRING_TRIM_HEAD or STRING_TRIM_TAIL."));
	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llRegionSay", NULL, "is", "llRegionSay(integer channel, string msg)\nbroadcasts msg to entire region on channel (not 0.)"));

	addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llGetObjectDetails", "l", "kl", "list llGetObjectDetails(key id, list params)\nGets the object details specified in params for the object with key id.\nDetails are OBJECT_NAME, _DESC, _POS, _ROT, _VELOCITY, _OWNER, _GROUP, _CREATOR."));

	// energy, sleep, dummy_func, name, return type, parameters, help text, gods-only

	// IF YOU ADD NEW SCRIPT CALLS, YOU MUST PUT THEM AT THE END OF THIS LIST.
	// Otherwise the bytecode numbers for each call will be wrong, and all
	// existing scripts will crash.
}

	//Ventrella Follow Cam Script Stuff
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamPitch",				NULL, "f", "llSetCamPitch(-45 to 80)\n(Adjusts the angular amount that the camera aims straight ahead vs. straight down, maintaining the same distance. Analogous to 'incidence'."));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamVerticalOffset",		NULL, "f", "llSetCamVerticalOffset(-2 to 2)\nAdjusts the vertical position of the camera focus position relative to the subject"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamPositionLag",			NULL, "f", "llSetCamPositionLag(0 to 3) \nHow much the camera lags as it tries to move towards its 'ideal' position"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamFocusLag",			NULL, "f", "llSetCamFocusLag(0 to 3)\nHow much the camera lags as it tries to aim towards the subject"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamDistance",			NULL, "f", "llSetCamDistance(0.5 to 10)\nSets how far away the camera wants to be from its subject"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamBehindnessAngle",		NULL, "f", "llSetCamBehindnessAngle(0 to 180)\nSets the angle in degrees within which the camera is not constrained by changes in subject rotation"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamBehindnessLag",		NULL, "f", "llSetCamBehindnessLag(0 to 3)\nSets how strongly the camera is forced to stay behind the target if outside of behindness angle"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamPositionThreshold",	NULL, "f", "llSetCamPositionThreshold(0 to 4)\nSets the radius of a sphere around the camera's ideal position within which it is not affected by subject motion"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamFocusThreshold",		NULL, "f", "llSetCamFocusThreshold(0 to 4)\nSets the radius of a sphere around the camera's subject position within which its focus is not affected by subject motion"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamScriptControl",				NULL, "i", "llSetCamScriptControl(TRUE or FALSE)\nTurns on or off scripted control of the camera"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamPosition",			NULL, "v", "llSetCamPosition(vector)\nSets the position of the camera"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamFocus",				NULL, "v", "llSetCamFocus(vector focus)\nSets the focus (target position) of the camera"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamPositionLocked",		NULL, "i", "llSetCamPositionLocked(TRUE or FALSE)\nLocks the camera position so it will not move"));
	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetCamFocusLocked",			NULL, "i", "llSetCamFocusLocked(TRUE or FALSE)\nLocks the camera focus so it will not move"));

	//addFunction(new LLScriptLibraryFunction(10.f, 0.f, dummy_func, "llSetForSale", "i", "ii", "integer llSetForSale(integer selltype, integer price)\nSets this object for sale in mode selltype for price.  Returns TRUE if successfully set for sale."));

LLScriptLibraryFunction::LLScriptLibraryFunction(F32 eu, F32 st, void (*exec_func)(LLScriptLibData *, LLScriptLibData *, const LLUUID &), char *name, char *ret_type, char *args, char *desc, BOOL god_only)
		: mEnergyUse(eu), mSleepTime(st), mExecFunc(exec_func), mName(name), mReturnType(ret_type), mArgs(args), mGodOnly(god_only)
{
	mDesc = new char[512];
	if (mSleepTime)
	{
		snprintf(	/* Flawfinder: ignore */
			mDesc,
			512,
			"%s\nSleeps script for %.1f seconds.",
			desc,
			mSleepTime);
	}
	else
	{
		strncpy(mDesc, desc, 512);	/* Flawfinder: ignore */
		mDesc[511] = '\0'; // just in case.
	}
}

LLScriptLibraryFunction::~LLScriptLibraryFunction()
{
	delete [] mDesc;
}

void LLScriptLibrary::addFunction(LLScriptLibraryFunction *func)
{
	LLScriptLibraryFunction **temp = new LLScriptLibraryFunction*[mNextNumber + 1];
	if (mNextNumber)
	{
		memcpy(	/* Flawfinder: ignore */
			temp,
			mFunctions,
			sizeof(LLScriptLibraryFunction*)*mNextNumber);
		delete [] mFunctions;
	}
	mFunctions = temp;
	mFunctions[mNextNumber] = func;
	mNextNumber++;
}

void LLScriptLibrary::assignExec(char *name, void (*exec_func)(LLScriptLibData *, LLScriptLibData *, const LLUUID &))
{
	S32 i;
	for (i = 0; i < mNextNumber; i++)
	{
		if (!strcmp(name, mFunctions[i]->mName))
		{
			mFunctions[i]->mExecFunc = exec_func;
		}
	}
}

void LLScriptLibData::print(std::ostream &s, BOOL b_prepend_comma)
{
	char tmp[1024];	/*Flawfinder: ignore*/
	if (b_prepend_comma)
	{
	        s << ", ";
	}
	switch (mType)
	{
	case LST_INTEGER:
	     s << mInteger;
	     break;
	case LST_FLOATINGPOINT:
	     snprintf(tmp, 1024, "%f", mFP);	/* Flawfinder: ignore */
	     s << tmp;
	     break;
	case LST_KEY:
	     s << mKey;
	     break;
	case LST_STRING:
	     s << mString;
	     break;
	case LST_VECTOR:
	     snprintf(tmp, 1024, "<%f, %f, %f>", mVec.mV[VX], /* Flawfinder: ignore */
		      mVec.mV[VY], mVec.mV[VZ]);
	     s << tmp;
	     break;
	case LST_QUATERNION:
	     snprintf(tmp, 1024, "<%f, %f, %f, %f>", mQuat.mQ[VX], mQuat.mQ[VY], /* Flawfinder: ignore */
		      mQuat.mQ[VZ], mQuat.mQ[VS]);
	     s << tmp;
	     break;
	default:
	     break;
	}
}

void LLScriptLibData::print_separator(std::ostream& ostr, BOOL b_prepend_sep, char* sep)
{
	if (b_prepend_sep)
	{
		ostr << sep;
	}
	//print(ostr, FALSE);
	{
		char tmp[1024];	/* Flawfinder: ignore */
		switch (mType)
		{
		case LST_INTEGER:
		     ostr << mInteger;
		     break;
		case LST_FLOATINGPOINT:
		     snprintf(tmp, 1024, "%f", mFP);	/* Flawfinder: ignore */
		     ostr << tmp;
		     break;
		case LST_KEY:
		     ostr << mKey;
		     break;
		case LST_STRING:
		     ostr << mString;
		     break;
		case LST_VECTOR:
		     snprintf(tmp, 1024, "<%f, %f, %f>", mVec.mV[VX], /* Flawfinder: ignore */
			      mVec.mV[VY], mVec.mV[VZ]);
		     ostr << tmp;
		     break;
		case LST_QUATERNION:
		     snprintf(tmp, 1024, "<%f, %f, %f, %f>", mQuat.mQ[VX], mQuat.mQ[VY], /* Flawfinder: ignore */
			      mQuat.mQ[VZ], mQuat.mQ[VS]);
		     ostr << tmp;
		     break;
		default:
		     break;
		}
	}
}


LLScriptLibrary gScriptLibrary;
