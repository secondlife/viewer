
N			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{N}+
FS			(f|F)
%e 10000
%n 4000
%p 5000

%top {
	#include "linden_common.h"
}

%{
// Deal with the fact that lex/yacc generates unreachable code
#ifdef LL_WINDOWS
#pragma warning (disable : 4018) // warning C4018: signed/unsigned mismatch
#pragma warning (disable : 4702) // warning C4702: unreachable code
#endif	//	LL_WINDOWS
#include "llmath.h"
#include "lscript_tree.h"
#include "lscript_typecheck.h"
#include "lscript_resource.h"
#include "indra.y.hpp"
#include "lltimer.h"
#include "indra_constants.h"
#include "llagentconstants.h"
#include "lllslconstants.h"
#include "lluuid.h"
#include "llassetstorage.h"
#include "llpartdata.h"
#include "llvehicleparams.h"
#include "llpermissionsflags.h"
#include "llfollowcamparams.h"
#include "llparcelflags.h"
#include "llregionflags.h"
#include "lscript_http.h"
#include "llclickaction.h"
#include "llmediaentry.h"

void count();
void line_comment();
void block_comment();
void parse_string();

#define YYLMAX 16384
#define YY_NEVER_INTERACTIVE 1 /* stops flex from calling isatty() */
#ifdef LL_WINDOWS
#define isatty(x) 0 /* hack for bug in cygwin flex 2.5.35 */
#endif

#ifdef ECHO
#undef ECHO
#endif

#define ECHO do { } while (0)

#define yyparse indra_parse
#define yyerror indra_error
#define yylval indra_lval
#define yy_create_buffer indra__create_buffer
#define yy_delete_buffer indra__delete_buffer
#define yy_flex_debug indra__flex_debug
#define yy_init_buffer indra__init_buffer
#define yy_flush_buffer indra__flush_buffer
#define yy_load_buffer_state indra__load_buffer_state
#define yy_switch_to_buffer indra__switch_to_buffer
#define yyin indra_in
#define yyleng indra_leng
#define yylex indra_lex
#define yylineno indra_lineno
#define yyout indra_out
#define yyrestart indra_restart
#define yytext indra_text
#define yywrap indra_wrap
#define yyalloc indra_alloc
#define yyrealloc indra_realloc
#define yyfree indra_free


#if defined(__cplusplus)
extern "C" { int yylex( void ); }
extern "C" { int yyparse( void ); }
extern "C" { int yyerror(const char *fmt, ...); }
#endif

%}

%%
"//"					{ gInternalLine++; gInternalColumn = 0; line_comment(); }
"/*"					{ block_comment(); }

"integer"			{ count(); return(INTEGER); }
"float"				{ count(); return(FLOAT_TYPE); }
"string"			{ count(); return(STRING); }
"key"				{ count(); return(LLKEY); }
"vector"			{ count(); return(VECTOR); }
"quaternion"		{ count(); return(QUATERNION); }
"rotation"			{ count(); return(QUATERNION); }
"list"				{ count(); return(LIST); }

"default"			{ count(); yylval.sval = new char[strlen(yytext) + 1]; strcpy(yylval.sval, yytext); return(STATE_DEFAULT); }
"state"				{ count(); return(STATE); }
"event"				{ count(); return(EVENT); }
"jump"				{ count(); return(JUMP); }
"return"			{ count(); return(RETURN); }
"if"				{ count(); return(IF); }
"else"				{ count(); return(ELSE); }
"for"				{ count(); return(FOR); }
"do"				{ count(); return(DO); }
"while"				{ count(); return(WHILE); }

"state_entry"			{ count(); return(STATE_ENTRY); }
"state_exit"			{ count(); return(STATE_EXIT); }
"touch_start"			{ count(); return(TOUCH_START); }
"touch"					{ count(); return(TOUCH); }
"touch_end"				{ count(); return(TOUCH_END); }
"collision_start"		{ count(); return(COLLISION_START); }
"collision"				{ count(); return(COLLISION); }
"collision_end"			{ count(); return(COLLISION_END); }
"land_collision_start"	{ count(); return(LAND_COLLISION_START); }
"land_collision"		{ count(); return(LAND_COLLISION); }
"land_collision_end"	{ count(); return(LAND_COLLISION_END); }
"timer"					{ count(); return(TIMER); }
"listen"				{ count(); return(CHAT); }
"sensor"				{ count(); return(SENSOR); }
"no_sensor"				{ count(); return(NO_SENSOR); }
"control"				{ count(); return(CONTROL); }
"print"					{ count(); return(PRINT); }
"at_target"				{ count(); return(AT_TARGET); }
"not_at_target"			{ count(); return(NOT_AT_TARGET); }
"at_rot_target"			{ count(); return(AT_ROT_TARGET); }
"not_at_rot_target"		{ count(); return(NOT_AT_ROT_TARGET); }
"money"					{ count(); return(MONEY); }
"email"					{ count(); return(EMAIL); }
"run_time_permissions"	{ count(); return(RUN_TIME_PERMISSIONS); }
"changed"				{ count(); return(INVENTORY); }
"attach"				{ count(); return(ATTACH); }
"dataserver"			{ count(); return(DATASERVER); }
"moving_start"			{ count(); return(MOVING_START); }
"moving_end"			{ count(); return(MOVING_END); }
"link_message"			{ count(); return(LINK_MESSAGE); }
"on_rez"				{ count(); return(REZ); }
"object_rez"			{ count(); return(OBJECT_REZ); }
"remote_data"			{ count(); return(REMOTE_DATA); }
"http_response"			{ count(); return(HTTP_RESPONSE); }
"http_request"			{ count(); return(HTTP_REQUEST); }
"."						{ count(); return(PERIOD); }


0[xX]{H}+			{ count(); yylval.ival = strtoul(yytext, NULL, 0);  return(INTEGER_CONSTANT); }
{N}+				{ count(); yylval.ival = strtoul(yytext, NULL, 10); return(INTEGER_CONSTANT); }
"TRUE"				{ count(); yylval.ival = 1; return(INTEGER_TRUE); }
"FALSE"				{ count(); yylval.ival = 0; return(INTEGER_FALSE); }
"STATUS_PHYSICS"		{ count(); yylval.ival = 0x1; return(INTEGER_CONSTANT); }
"STATUS_ROTATE_X"		{ count(); yylval.ival = 0x2; return(INTEGER_CONSTANT); }
"STATUS_ROTATE_Y"		{ count(); yylval.ival = 0x4; return(INTEGER_CONSTANT); }
"STATUS_ROTATE_Z"		{ count(); yylval.ival = 0x8; return(INTEGER_CONSTANT); }
"STATUS_PHANTOM"		{ count(); yylval.ival = 0x10; return(INTEGER_CONSTANT); }
"STATUS_SANDBOX"		{ count(); yylval.ival = 0x20; return(INTEGER_CONSTANT); }
"STATUS_BLOCK_GRAB"		{ count(); yylval.ival = 0x40; return(INTEGER_CONSTANT); }
"STATUS_DIE_AT_EDGE"	{ count(); yylval.ival = 0x80; return(INTEGER_CONSTANT); }
"STATUS_RETURN_AT_EDGE"	{ count(); yylval.ival = 0x100; return(INTEGER_CONSTANT); }
"STATUS_CAST_SHADOWS"	{ count(); yylval.ival = 0x200; return(INTEGER_CONSTANT); }

"AGENT_FLYING"			{ count(); yylval.ival = AGENT_FLYING; return(INTEGER_CONSTANT); }
"AGENT_ATTACHMENTS"		{ count(); yylval.ival = AGENT_ATTACHMENTS; return(INTEGER_CONSTANT); }
"AGENT_SCRIPTED"		{ count(); yylval.ival = AGENT_SCRIPTED; return(INTEGER_CONSTANT); }
"AGENT_MOUSELOOK"		{ count(); yylval.ival = AGENT_MOUSELOOK; return(INTEGER_CONSTANT); }
"AGENT_SITTING"			{ count(); yylval.ival = AGENT_SITTING; return(INTEGER_CONSTANT); }
"AGENT_ON_OBJECT"		{ count(); yylval.ival = AGENT_ON_OBJECT; return(INTEGER_CONSTANT); }
"AGENT_AWAY"			{ count(); yylval.ival = AGENT_AWAY; return(INTEGER_CONSTANT); }
"AGENT_WALKING"			{ count(); yylval.ival = AGENT_WALKING; return(INTEGER_CONSTANT); }
"AGENT_IN_AIR"			{ count(); yylval.ival = AGENT_IN_AIR; return(INTEGER_CONSTANT); }
"AGENT_TYPING"			{ count(); yylval.ival = AGENT_TYPING; return(INTEGER_CONSTANT); }
"AGENT_CROUCHING"		{ count(); yylval.ival = AGENT_CROUCHING; return(INTEGER_CONSTANT); }
"AGENT_BUSY"			{ count(); yylval.ival = AGENT_BUSY; return(INTEGER_CONSTANT); }
"AGENT_ALWAYS_RUN"		{ count(); yylval.ival = AGENT_ALWAYS_RUN; return(INTEGER_CONSTANT); }
"AGENT_AUTOPILOT"		{ count(); yylval.ival = AGENT_AUTOPILOT; return(INTEGER_CONSTANT); }

"CAMERA_PITCH"				{ count(); yylval.ival = FOLLOWCAM_PITCH; return(INTEGER_CONSTANT); }
"CAMERA_FOCUS_OFFSET"		{ count(); yylval.ival = FOLLOWCAM_FOCUS_OFFSET; return (INTEGER_CONSTANT); }
"CAMERA_POSITION_LAG"		{ count(); yylval.ival = FOLLOWCAM_POSITION_LAG; return (INTEGER_CONSTANT); }
"CAMERA_FOCUS_LAG"			{ count(); yylval.ival = FOLLOWCAM_FOCUS_LAG; return (INTEGER_CONSTANT); }
"CAMERA_DISTANCE"			{ count(); yylval.ival = FOLLOWCAM_DISTANCE; return (INTEGER_CONSTANT); }
"CAMERA_BEHINDNESS_ANGLE"	{ count(); yylval.ival = FOLLOWCAM_BEHINDNESS_ANGLE; return (INTEGER_CONSTANT); }
"CAMERA_BEHINDNESS_LAG"		{ count(); yylval.ival = FOLLOWCAM_BEHINDNESS_LAG; return (INTEGER_CONSTANT); }
"CAMERA_POSITION_THRESHOLD"	{ count(); yylval.ival = FOLLOWCAM_POSITION_THRESHOLD; return (INTEGER_CONSTANT); }
"CAMERA_FOCUS_THRESHOLD"	{ count(); yylval.ival = FOLLOWCAM_FOCUS_THRESHOLD; return (INTEGER_CONSTANT); }
"CAMERA_ACTIVE"				{ count(); yylval.ival = FOLLOWCAM_ACTIVE; return (INTEGER_CONSTANT); }
"CAMERA_POSITION"			{ count(); yylval.ival = FOLLOWCAM_POSITION; return (INTEGER_CONSTANT); }
"CAMERA_FOCUS"				{ count(); yylval.ival = FOLLOWCAM_FOCUS; return (INTEGER_CONSTANT); }
"CAMERA_POSITION_LOCKED"	{ count(); yylval.ival = FOLLOWCAM_POSITION_LOCKED; return (INTEGER_CONSTANT); }
"CAMERA_FOCUS_LOCKED"		{ count(); yylval.ival = FOLLOWCAM_FOCUS_LOCKED; return (INTEGER_CONSTANT); }

"ANIM_ON"				{ count(); yylval.ival = 0x1; return(INTEGER_CONSTANT); }
"LOOP"					{ count(); yylval.ival = 0x2; return(INTEGER_CONSTANT); }
"REVERSE"				{ count(); yylval.ival = 0x4; return(INTEGER_CONSTANT); }
"PING_PONG"				{ count(); yylval.ival = 0x8; return(INTEGER_CONSTANT); }
"SMOOTH"				{ count(); yylval.ival = 0x10; return(INTEGER_CONSTANT); }
"ROTATE"				{ count(); yylval.ival = 0x20; return(INTEGER_CONSTANT); }
"SCALE"					{ count(); yylval.ival = 0x40; return(INTEGER_CONSTANT); }

"ALL_SIDES"				{ count(); yylval.ival = LSL_ALL_SIDES; return(INTEGER_CONSTANT); }
"LINK_ROOT"				{ count(); yylval.ival = LSL_LINK_ROOT; return(INTEGER_CONSTANT); }
"LINK_SET"				{ count(); yylval.ival = LSL_LINK_SET; return(INTEGER_CONSTANT); }
"LINK_ALL_OTHERS"		{ count(); yylval.ival = LSL_LINK_ALL_OTHERS; return(INTEGER_CONSTANT); }
"LINK_ALL_CHILDREN"		{ count(); yylval.ival = LSL_LINK_ALL_CHILDREN; return(INTEGER_CONSTANT); }
"LINK_THIS"				{ count(); yylval.ival = LSL_LINK_THIS; return(INTEGER_CONSTANT); }

"AGENT"					{ count(); yylval.ival = 0x1; return(INTEGER_CONSTANT); }
"ACTIVE"				{ count(); yylval.ival = 0x2; return(INTEGER_CONSTANT); }
"PASSIVE"				{ count(); yylval.ival = 0x4; return(INTEGER_CONSTANT); }
"SCRIPTED"				{ count(); yylval.ival = 0x8; return(INTEGER_CONSTANT); }

"CONTROL_FWD"			{ count(); yylval.ival = AGENT_CONTROL_AT_POS; return(INTEGER_CONSTANT); }
"CONTROL_BACK"			{ count(); yylval.ival = AGENT_CONTROL_AT_NEG; return(INTEGER_CONSTANT); }
"CONTROL_LEFT"			{ count(); yylval.ival = AGENT_CONTROL_LEFT_POS; return(INTEGER_CONSTANT); }
"CONTROL_RIGHT"			{ count(); yylval.ival = AGENT_CONTROL_LEFT_NEG; return(INTEGER_CONSTANT); }
"CONTROL_ROT_LEFT"		{ count(); yylval.ival = AGENT_CONTROL_YAW_POS; return(INTEGER_CONSTANT); }
"CONTROL_ROT_RIGHT"		{ count(); yylval.ival = AGENT_CONTROL_YAW_NEG; return(INTEGER_CONSTANT); }
"CONTROL_UP"			{ count(); yylval.ival = AGENT_CONTROL_UP_POS; return(INTEGER_CONSTANT); }
"CONTROL_DOWN"			{ count(); yylval.ival = AGENT_CONTROL_UP_NEG; return(INTEGER_CONSTANT); }
"CONTROL_LBUTTON"		{ count(); yylval.ival = AGENT_CONTROL_LBUTTON_DOWN; return(INTEGER_CONSTANT); }
"CONTROL_ML_LBUTTON"	{ count(); yylval.ival = AGENT_CONTROL_ML_LBUTTON_DOWN; return(INTEGER_CONSTANT); }

"PERMISSION_DEBIT"				{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_DEBIT]; return(INTEGER_CONSTANT); }
"PERMISSION_TAKE_CONTROLS"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TAKE_CONTROLS]; return(INTEGER_CONSTANT); }
"PERMISSION_REMAP_CONTROLS"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_REMAP_CONTROLS]; return(INTEGER_CONSTANT); }
"PERMISSION_TRIGGER_ANIMATION"	{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TRIGGER_ANIMATION]; return(INTEGER_CONSTANT); }
"PERMISSION_ATTACH"				{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_ATTACH]; return(INTEGER_CONSTANT); }
"PERMISSION_RELEASE_OWNERSHIP"	{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_RELEASE_OWNERSHIP]; return(INTEGER_CONSTANT); }
"PERMISSION_CHANGE_LINKS"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_CHANGE_LINKS]; return(INTEGER_CONSTANT); }
"PERMISSION_CHANGE_JOINTS"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_CHANGE_JOINTS]; return(INTEGER_CONSTANT); }
"PERMISSION_CHANGE_PERMISSIONS"	{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_CHANGE_PERMISSIONS]; return(INTEGER_CONSTANT); }
"PERMISSION_TRACK_CAMERA"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TRACK_CAMERA]; return(INTEGER_CONSTANT); }
"PERMISSION_CONTROL_CAMERA"		{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_CONTROL_CAMERA]; return(INTEGER_CONSTANT); }
"PERMISSION_TELEPORT"			{ count(); yylval.ival = LSCRIPTRunTimePermissionBits[SCRIPT_PERMISSION_TELEPORT]; return(INTEGER_CONSTANT); }

"INVENTORY_TEXTURE"					{ count(); yylval.ival = LLAssetType::AT_TEXTURE; return(INTEGER_CONSTANT); }
"INVENTORY_SOUND"					{ count(); yylval.ival = LLAssetType::AT_SOUND; return(INTEGER_CONSTANT); }
"INVENTORY_OBJECT"					{ count(); yylval.ival = LLAssetType::AT_OBJECT; return(INTEGER_CONSTANT); }
"INVENTORY_SCRIPT"					{ count(); yylval.ival = LLAssetType::AT_LSL_TEXT; return(INTEGER_CONSTANT); }
"INVENTORY_LANDMARK"				{ count(); yylval.ival = LLAssetType::AT_LANDMARK; return(INTEGER_CONSTANT); }
"INVENTORY_CLOTHING"				{ count(); yylval.ival = LLAssetType::AT_CLOTHING; return(INTEGER_CONSTANT); }
"INVENTORY_NOTECARD"				{ count(); yylval.ival = LLAssetType::AT_NOTECARD; return(INTEGER_CONSTANT); }
"INVENTORY_BODYPART"				{ count(); yylval.ival = LLAssetType::AT_BODYPART; return(INTEGER_CONSTANT); }
"INVENTORY_ANIMATION"				{ count(); yylval.ival = LLAssetType::AT_ANIMATION; return(INTEGER_CONSTANT); }
"INVENTORY_GESTURE"					{ count(); yylval.ival = LLAssetType::AT_GESTURE; return(INTEGER_CONSTANT); }
"INVENTORY_ALL"						{ count(); yylval.ival = LLAssetType::AT_NONE; return(INTEGER_CONSTANT); }
"INVENTORY_NONE"					{ count(); yylval.ival = LLAssetType::AT_NONE; return(INTEGER_CONSTANT); }

"CHANGED_INVENTORY"		{ count(); yylval.ival = CHANGED_INVENTORY; return(INTEGER_CONSTANT); }	
"CHANGED_COLOR"			{ count(); yylval.ival = CHANGED_COLOR; return(INTEGER_CONSTANT); }	
"CHANGED_SHAPE"			{ count(); yylval.ival = CHANGED_SHAPE; return(INTEGER_CONSTANT); }	
"CHANGED_SCALE"			{ count(); yylval.ival = CHANGED_SCALE; return(INTEGER_CONSTANT); }	
"CHANGED_TEXTURE"		{ count(); yylval.ival = CHANGED_TEXTURE; return(INTEGER_CONSTANT); }	
"CHANGED_LINK"			{ count(); yylval.ival = CHANGED_LINK; return(INTEGER_CONSTANT); }	
"CHANGED_ALLOWED_DROP"	{ count(); yylval.ival = CHANGED_ALLOWED_DROP; return(INTEGER_CONSTANT); }	
"CHANGED_OWNER"			{ count(); yylval.ival = CHANGED_OWNER; return(INTEGER_CONSTANT); }	
"CHANGED_REGION"		{ count(); yylval.ival = CHANGED_REGION; return(INTEGER_CONSTANT); }	
"CHANGED_TELEPORT"		{ count(); yylval.ival = CHANGED_TELEPORT; return(INTEGER_CONSTANT); }	
"CHANGED_REGION_START"	{ count(); yylval.ival = CHANGED_REGION_START; return(INTEGER_CONSTANT); }	
"CHANGED_MEDIA"		    { count(); yylval.ival = CHANGED_MEDIA; return(INTEGER_CONSTANT); }	

"OBJECT_UNKNOWN_DETAIL"	{ count(); yylval.ival = OBJECT_UNKNOWN_DETAIL; return(INTEGER_CONSTANT); }
"OBJECT_NAME"			{ count(); yylval.ival = OBJECT_NAME; return(INTEGER_CONSTANT); }
"OBJECT_DESC"			{ count(); yylval.ival = OBJECT_DESC; return(INTEGER_CONSTANT); }
"OBJECT_POS"			{ count(); yylval.ival = OBJECT_POS; return(INTEGER_CONSTANT); }
"OBJECT_ROT"			{ count(); yylval.ival = OBJECT_ROT; return(INTEGER_CONSTANT); }
"OBJECT_VELOCITY"		{ count(); yylval.ival = OBJECT_VELOCITY; return(INTEGER_CONSTANT); }
"OBJECT_OWNER"			{ count(); yylval.ival = OBJECT_OWNER; return(INTEGER_CONSTANT); }
"OBJECT_GROUP"			{ count(); yylval.ival = OBJECT_GROUP; return(INTEGER_CONSTANT); }
"OBJECT_CREATOR"		{ count(); yylval.ival = OBJECT_CREATOR; return(INTEGER_CONSTANT); }

"TYPE_INTEGER"			{ count(); yylval.ival = LST_INTEGER; return(INTEGER_CONSTANT); }	
"TYPE_FLOAT"			{ count(); yylval.ival = LST_FLOATINGPOINT; return(INTEGER_CONSTANT); }	
"TYPE_STRING"			{ count(); yylval.ival = LST_STRING; return(INTEGER_CONSTANT); }	
"TYPE_KEY"				{ count(); yylval.ival = LST_KEY; return(INTEGER_CONSTANT); }	
"TYPE_VECTOR"			{ count(); yylval.ival = LST_VECTOR; return(INTEGER_CONSTANT); }	
"TYPE_ROTATION"			{ count(); yylval.ival = LST_QUATERNION; return(INTEGER_CONSTANT); }	
"TYPE_INVALID"			{ count(); yylval.ival = LST_NULL; return(INTEGER_CONSTANT); }	

"NULL_KEY"				{ yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "00000000-0000-0000-0000-000000000000"); return(STRING_CONSTANT); }
"EOF"					{ yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "\n\n\n"); return(STRING_CONSTANT); }
"URL_REQUEST_GRANTED"	{ yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, URL_REQUEST_GRANTED); return(STRING_CONSTANT); }
"URL_REQUEST_DENIED"	{ yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, URL_REQUEST_DENIED); return(STRING_CONSTANT); }

"PI"					{ count(); yylval.fval = F_PI; return(FP_CONSTANT); }
"TWO_PI"				{ count(); yylval.fval = F_TWO_PI; return(FP_CONSTANT); }
"PI_BY_TWO"				{ count(); yylval.fval = F_PI_BY_TWO; return(FP_CONSTANT); }
"DEG_TO_RAD"			{ count(); yylval.fval = DEG_TO_RAD; return(FP_CONSTANT); }
"RAD_TO_DEG"			{ count(); yylval.fval = RAD_TO_DEG; return(FP_CONSTANT); }
"SQRT2"					{ count(); yylval.fval = F_SQRT2; return(FP_CONSTANT); }

"DEBUG_CHANNEL"			{ count(); yylval.ival = CHAT_CHANNEL_DEBUG; return(INTEGER_CONSTANT); }	
"PUBLIC_CHANNEL"			{ count(); yylval.ival = 0; return(INTEGER_CONSTANT); }	

"ZERO_VECTOR"			{ count(); return(ZERO_VECTOR); }
"ZERO_ROTATION"			{ count(); return(ZERO_ROTATION); }

"ATTACH_CHEST"		{ count(); yylval.ival = 1; return(INTEGER_CONSTANT); }
"ATTACH_HEAD"		{ count(); yylval.ival = 2; return(INTEGER_CONSTANT); }
"ATTACH_LSHOULDER"	{ count(); yylval.ival = 3; return(INTEGER_CONSTANT); }
"ATTACH_RSHOULDER"	{ count(); yylval.ival = 4; return(INTEGER_CONSTANT); }
"ATTACH_LHAND"		{ count(); yylval.ival = 5; return(INTEGER_CONSTANT); }
"ATTACH_RHAND"		{ count(); yylval.ival = 6; return(INTEGER_CONSTANT); }
"ATTACH_LFOOT"		{ count(); yylval.ival = 7; return(INTEGER_CONSTANT); }
"ATTACH_RFOOT"		{ count(); yylval.ival = 8; return(INTEGER_CONSTANT); }
"ATTACH_BACK"		{ count(); yylval.ival = 9; return(INTEGER_CONSTANT); }
"ATTACH_PELVIS"		{ count(); yylval.ival = 10; return(INTEGER_CONSTANT); }
"ATTACH_MOUTH"		{ count(); yylval.ival = 11; return(INTEGER_CONSTANT); }
"ATTACH_CHIN"		{ count(); yylval.ival = 12; return(INTEGER_CONSTANT); }
"ATTACH_LEAR"		{ count(); yylval.ival = 13; return(INTEGER_CONSTANT); }
"ATTACH_REAR"		{ count(); yylval.ival = 14; return(INTEGER_CONSTANT); }
"ATTACH_LEYE"		{ count(); yylval.ival = 15; return(INTEGER_CONSTANT); }
"ATTACH_REYE"		{ count(); yylval.ival = 16; return(INTEGER_CONSTANT); }
"ATTACH_NOSE"		{ count(); yylval.ival = 17; return(INTEGER_CONSTANT); }
"ATTACH_RUARM"		{ count(); yylval.ival = 18; return(INTEGER_CONSTANT); }
"ATTACH_RLARM"		{ count(); yylval.ival = 19; return(INTEGER_CONSTANT); }
"ATTACH_LUARM"		{ count(); yylval.ival = 20; return(INTEGER_CONSTANT); }
"ATTACH_LLARM"		{ count(); yylval.ival = 21; return(INTEGER_CONSTANT); }
"ATTACH_RHIP"		{ count(); yylval.ival = 22; return(INTEGER_CONSTANT); }
"ATTACH_RULEG"		{ count(); yylval.ival = 23; return(INTEGER_CONSTANT); }
"ATTACH_RLLEG"		{ count(); yylval.ival = 24; return(INTEGER_CONSTANT); }
"ATTACH_LHIP"		{ count(); yylval.ival = 25; return(INTEGER_CONSTANT); }
"ATTACH_LULEG"		{ count(); yylval.ival = 26; return(INTEGER_CONSTANT); }
"ATTACH_LLLEG"		{ count(); yylval.ival = 27; return(INTEGER_CONSTANT); }
"ATTACH_BELLY"		{ count(); yylval.ival = 28; return(INTEGER_CONSTANT); }
"ATTACH_RPEC"		{ count(); yylval.ival = 29; return(INTEGER_CONSTANT); }
"ATTACH_LPEC"		{ count(); yylval.ival = 30; return(INTEGER_CONSTANT); }
"ATTACH_HUD_CENTER_2"	{ count(); yylval.ival = 31; return(INTEGER_CONSTANT); }
"ATTACH_HUD_TOP_RIGHT"	{ count(); yylval.ival = 32; return(INTEGER_CONSTANT); }
"ATTACH_HUD_TOP_CENTER"	{ count(); yylval.ival = 33; return(INTEGER_CONSTANT); }
"ATTACH_HUD_TOP_LEFT"	{ count(); yylval.ival = 34; return(INTEGER_CONSTANT); }
"ATTACH_HUD_CENTER_1"	{ count(); yylval.ival = 35; return(INTEGER_CONSTANT); }
"ATTACH_HUD_BOTTOM_LEFT" { count(); yylval.ival = 36; return(INTEGER_CONSTANT); }
"ATTACH_HUD_BOTTOM"		{ count(); yylval.ival = 37; return(INTEGER_CONSTANT); }
"ATTACH_HUD_BOTTOM_RIGHT"	{ count(); yylval.ival = 38; return(INTEGER_CONSTANT); }

"LAND_LEVEL"		{ count(); yylval.ival = E_LANDBRUSH_LEVEL; return(INTEGER_CONSTANT); }
"LAND_RAISE"		{ count(); yylval.ival = E_LANDBRUSH_RAISE; return(INTEGER_CONSTANT); }
"LAND_LOWER"		{ count(); yylval.ival = E_LANDBRUSH_LOWER; return(INTEGER_CONSTANT); }
"LAND_SMOOTH"		{ count(); yylval.ival = E_LANDBRUSH_SMOOTH; return(INTEGER_CONSTANT); }
"LAND_NOISE"		{ count(); yylval.ival = E_LANDBRUSH_NOISE; return(INTEGER_CONSTANT); }
"LAND_REVERT"		{ count(); yylval.ival = E_LANDBRUSH_REVERT; return(INTEGER_CONSTANT); }
	
"LAND_SMALL_BRUSH"	{ count(); yylval.ival = 1; return(INTEGER_CONSTANT); }
"LAND_MEDIUM_BRUSH"	{ count(); yylval.ival = 2; return(INTEGER_CONSTANT); }
"LAND_LARGE_BRUSH"	{ count(); yylval.ival = 3; return(INTEGER_CONSTANT); }
	
"DATA_ONLINE"		{ count(); yylval.ival = 1; return(INTEGER_CONSTANT); }
"DATA_NAME"			{ count(); yylval.ival = 2; return(INTEGER_CONSTANT); }
"DATA_BORN"			{ count(); yylval.ival = 3; return(INTEGER_CONSTANT); }
"DATA_RATING"		{ count(); yylval.ival = 4; return(INTEGER_CONSTANT); }
"DATA_SIM_POS"		{ count(); yylval.ival = 5; return(INTEGER_CONSTANT); }
"DATA_SIM_STATUS"	{ count(); yylval.ival = 6; return(INTEGER_CONSTANT); }
"DATA_SIM_RATING"	{ count(); yylval.ival = 7; return(INTEGER_CONSTANT); }
"DATA_PAYINFO"		{ count(); yylval.ival = 8; return(INTEGER_CONSTANT); }

"PAYMENT_INFO_ON_FILE" { count(); yylval.ival = 1; return(INTEGER_CONSTANT); }
"PAYMENT_INFO_USED"  { count(); yylval.ival = 2; return(INTEGER_CONSTANT); }

"REMOTE_DATA_CHANNEL"	{ count(); yylval.ival = LSL_REMOTE_DATA_CHANNEL; return(INTEGER_CONSTANT); }
"REMOTE_DATA_REQUEST"	{ count(); yylval.ival = LSL_REMOTE_DATA_REQUEST; return(INTEGER_CONSTANT); }
"REMOTE_DATA_REPLY"		{ count(); yylval.ival = LSL_REMOTE_DATA_REPLY; return(INTEGER_CONSTANT); }


"PSYS_PART_FLAGS"		{ count(); yylval.ival = LLPS_PART_FLAGS; return(INTEGER_CONSTANT); }
"PSYS_PART_START_COLOR"	{ count(); yylval.ival = LLPS_PART_START_COLOR; return (INTEGER_CONSTANT); }
"PSYS_PART_START_ALPHA"	{ count(); yylval.ival = LLPS_PART_START_ALPHA; return (INTEGER_CONSTANT); }
"PSYS_PART_START_SCALE"	{ count(); yylval.ival = LLPS_PART_START_SCALE; return (INTEGER_CONSTANT); }
"PSYS_PART_END_COLOR"	{ count(); yylval.ival = LLPS_PART_END_COLOR; return (INTEGER_CONSTANT); }
"PSYS_PART_END_ALPHA"	{ count(); yylval.ival = LLPS_PART_END_ALPHA; return (INTEGER_CONSTANT); }
"PSYS_PART_END_SCALE"	{ count(); yylval.ival = LLPS_PART_END_SCALE; return (INTEGER_CONSTANT); }
"PSYS_PART_MAX_AGE"		{ count(); yylval.ival = LLPS_PART_MAX_AGE; return (INTEGER_CONSTANT); }


"PSYS_PART_WIND_MASK"				{ count(); yylval.ival = LLPartData::LL_PART_WIND_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_INTERP_COLOR_MASK"		{ count(); yylval.ival = LLPartData::LL_PART_INTERP_COLOR_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_INTERP_SCALE_MASK"		{ count(); yylval.ival = LLPartData::LL_PART_INTERP_SCALE_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_BOUNCE_MASK"				{ count(); yylval.ival = LLPartData::LL_PART_BOUNCE_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_FOLLOW_SRC_MASK"			{ count(); yylval.ival = LLPartData::LL_PART_FOLLOW_SRC_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_FOLLOW_VELOCITY_MASK"	{ count(); yylval.ival = LLPartData::LL_PART_FOLLOW_VELOCITY_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_TARGET_POS_MASK"			{ count(); yylval.ival = LLPartData::LL_PART_TARGET_POS_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_EMISSIVE_MASK"			{ count(); yylval.ival = LLPartData::LL_PART_EMISSIVE_MASK; return(INTEGER_CONSTANT); }
"PSYS_PART_TARGET_LINEAR_MASK"		{ count(); yylval.ival = LLPartData::LL_PART_TARGET_LINEAR_MASK; return(INTEGER_CONSTANT); }


"PSYS_SRC_MAX_AGE"					{ count(); yylval.ival = LLPS_SRC_MAX_AGE; return(INTEGER_CONSTANT); }
"PSYS_SRC_PATTERN"					{ count(); yylval.ival = LLPS_SRC_PATTERN; return(INTEGER_CONSTANT); }
"PSYS_SRC_INNERANGLE"				{ count(); yylval.ival = LLPS_SRC_INNERANGLE; return(INTEGER_CONSTANT); }
"PSYS_SRC_OUTERANGLE"				{ count(); yylval.ival = LLPS_SRC_OUTERANGLE; return(INTEGER_CONSTANT); }
"PSYS_SRC_ANGLE_BEGIN"				{ count(); yylval.ival = LLPS_SRC_ANGLE_BEGIN; return(INTEGER_CONSTANT); }
"PSYS_SRC_ANGLE_END"				{ count(); yylval.ival = LLPS_SRC_ANGLE_END; return(INTEGER_CONSTANT); }
"PSYS_SRC_BURST_RATE"				{ count(); yylval.ival = LLPS_SRC_BURST_RATE; return(INTEGER_CONSTANT); }
"PSYS_SRC_BURST_PART_COUNT"			{ count(); yylval.ival = LLPS_SRC_BURST_PART_COUNT; return(INTEGER_CONSTANT); }
"PSYS_SRC_BURST_RADIUS"				{ count(); yylval.ival = LLPS_SRC_BURST_RADIUS; return(INTEGER_CONSTANT); }
"PSYS_SRC_BURST_SPEED_MIN"			{ count(); yylval.ival = LLPS_SRC_BURST_SPEED_MIN; return(INTEGER_CONSTANT); }
"PSYS_SRC_BURST_SPEED_MAX"			{ count(); yylval.ival = LLPS_SRC_BURST_SPEED_MAX; return(INTEGER_CONSTANT); }
"PSYS_SRC_ACCEL"					{ count(); yylval.ival = LLPS_SRC_ACCEL; return(INTEGER_CONSTANT); }
"PSYS_SRC_TEXTURE"					{ count(); yylval.ival = LLPS_SRC_TEXTURE; return(INTEGER_CONSTANT); }
"PSYS_SRC_TARGET_KEY"				{ count(); yylval.ival = LLPS_SRC_TARGET_UUID; return(INTEGER_CONSTANT); }
"PSYS_SRC_OMEGA"					{ count(); yylval.ival = LLPS_SRC_OMEGA; return(INTEGER_CONSTANT); }

"PSYS_SRC_OBJ_REL_MASK"				{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_OBJ_REL_MASK; return(INTEGER_CONSTANT); }

"PSYS_SRC_PATTERN_DROP"				{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_PATTERN_DROP; return(INTEGER_CONSTANT); }
"PSYS_SRC_PATTERN_EXPLODE"			{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_PATTERN_EXPLODE; return(INTEGER_CONSTANT); }
"PSYS_SRC_PATTERN_ANGLE"			{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE; return(INTEGER_CONSTANT); }
"PSYS_SRC_PATTERN_ANGLE_CONE"		{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE; return(INTEGER_CONSTANT); }
"PSYS_SRC_PATTERN_ANGLE_CONE_EMPTY"	{ count(); yylval.ival = LLPartSysData::LL_PART_SRC_PATTERN_ANGLE_CONE_EMPTY; return(INTEGER_CONSTANT); }


"VEHICLE_TYPE_NONE"		{ count(); yylval.ival = VEHICLE_TYPE_NONE; return(INTEGER_CONSTANT); }
"VEHICLE_TYPE_SLED"		{ count(); yylval.ival = VEHICLE_TYPE_SLED; return(INTEGER_CONSTANT); }
"VEHICLE_TYPE_CAR"		{ count(); yylval.ival = VEHICLE_TYPE_CAR; return(INTEGER_CONSTANT); }
"VEHICLE_TYPE_BOAT"		{ count(); yylval.ival = VEHICLE_TYPE_BOAT; return(INTEGER_CONSTANT); }
"VEHICLE_TYPE_AIRPLANE"	{ count(); yylval.ival = VEHICLE_TYPE_AIRPLANE; return(INTEGER_CONSTANT); }
"VEHICLE_TYPE_BALLOON"	{ count(); yylval.ival = VEHICLE_TYPE_BALLOON; return(INTEGER_CONSTANT); }

"VEHICLE_REFERENCE_FRAME"			{ count(); yylval.ival = VEHICLE_REFERENCE_FRAME; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_FRICTION_TIMESCALE"	{ count(); yylval.ival = VEHICLE_LINEAR_FRICTION_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_ANGULAR_FRICTION_TIMESCALE" { count(); yylval.ival = VEHICLE_ANGULAR_FRICTION_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_MOTOR_DIRECTION"	{ count(); yylval.ival = VEHICLE_LINEAR_MOTOR_DIRECTION; return(INTEGER_CONSTANT); }
"VEHICLE_ANGULAR_MOTOR_DIRECTION"	{ count(); yylval.ival = VEHICLE_ANGULAR_MOTOR_DIRECTION; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_MOTOR_OFFSET"	    { count(); yylval.ival = VEHICLE_LINEAR_MOTOR_OFFSET; return(INTEGER_CONSTANT); }



"VEHICLE_HOVER_HEIGHT"		{ count(); yylval.ival = VEHICLE_HOVER_HEIGHT; return(INTEGER_CONSTANT); }
"VEHICLE_HOVER_EFFICIENCY"	{ count(); yylval.ival = VEHICLE_HOVER_EFFICIENCY; return(INTEGER_CONSTANT); }
"VEHICLE_HOVER_TIMESCALE"	{ count(); yylval.ival = VEHICLE_HOVER_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_BUOYANCY"			{ count(); yylval.ival = VEHICLE_BUOYANCY; return(INTEGER_CONSTANT); }

"VEHICLE_LINEAR_DEFLECTION_EFFICIENCY"	{ count(); yylval.ival = VEHICLE_LINEAR_DEFLECTION_EFFICIENCY; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_DEFLECTION_TIMESCALE"	{ count(); yylval.ival = VEHICLE_LINEAR_DEFLECTION_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_MOTOR_TIMESCALE"		{ count(); yylval.ival = VEHICLE_LINEAR_MOTOR_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_LINEAR_MOTOR_DECAY_TIMESCALE"	{ count(); yylval.ival = VEHICLE_LINEAR_MOTOR_DECAY_TIMESCALE; return(INTEGER_CONSTANT); }

"VEHICLE_ANGULAR_DEFLECTION_EFFICIENCY" { count(); yylval.ival = VEHICLE_ANGULAR_DEFLECTION_EFFICIENCY; return(INTEGER_CONSTANT); }
"VEHICLE_ANGULAR_DEFLECTION_TIMESCALE"	{ count(); yylval.ival = VEHICLE_ANGULAR_DEFLECTION_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_ANGULAR_MOTOR_TIMESCALE"		{ count(); yylval.ival = VEHICLE_ANGULAR_MOTOR_TIMESCALE; return(INTEGER_CONSTANT); }
"VEHICLE_ANGULAR_MOTOR_DECAY_TIMESCALE"	{ count(); yylval.ival = VEHICLE_ANGULAR_MOTOR_DECAY_TIMESCALE; return(INTEGER_CONSTANT); }

"VEHICLE_VERTICAL_ATTRACTION_EFFICIENCY"	{ count(); yylval.ival = VEHICLE_VERTICAL_ATTRACTION_EFFICIENCY; return(INTEGER_CONSTANT); }
"VEHICLE_VERTICAL_ATTRACTION_TIMESCALE"		{ count(); yylval.ival = VEHICLE_VERTICAL_ATTRACTION_TIMESCALE; return(INTEGER_CONSTANT); }

"VEHICLE_BANKING_EFFICIENCY"	{ count(); yylval.ival = VEHICLE_BANKING_EFFICIENCY; return(INTEGER_CONSTANT); }
"VEHICLE_BANKING_MIX"			{ count(); yylval.ival = VEHICLE_BANKING_MIX; return(INTEGER_CONSTANT); }
"VEHICLE_BANKING_TIMESCALE"		{ count(); yylval.ival = VEHICLE_BANKING_TIMESCALE; return(INTEGER_CONSTANT); }

"VEHICLE_FLAG_NO_FLY_UP"			{ count(); yylval.ival = VEHICLE_FLAG_NO_DEFLECTION_UP; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_NO_DEFLECTION_UP"			{ count(); yylval.ival = VEHICLE_FLAG_NO_DEFLECTION_UP; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_LIMIT_ROLL_ONLY"		{ count(); yylval.ival = VEHICLE_FLAG_LIMIT_ROLL_ONLY; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_HOVER_WATER_ONLY"		{ count(); yylval.ival = VEHICLE_FLAG_HOVER_WATER_ONLY; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_HOVER_TERRAIN_ONLY"	{ count(); yylval.ival = VEHICLE_FLAG_HOVER_TERRAIN_ONLY; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_HOVER_GLOBAL_HEIGHT"	{ count(); yylval.ival = VEHICLE_FLAG_HOVER_GLOBAL_HEIGHT; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_HOVER_UP_ONLY"		{ count(); yylval.ival = VEHICLE_FLAG_HOVER_UP_ONLY; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_LIMIT_MOTOR_UP"		{ count(); yylval.ival = VEHICLE_FLAG_LIMIT_MOTOR_UP; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_MOUSELOOK_STEER"		{ count(); yylval.ival = VEHICLE_FLAG_MOUSELOOK_STEER; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_MOUSELOOK_BANK"		{ count(); yylval.ival = VEHICLE_FLAG_MOUSELOOK_BANK; return(INTEGER_CONSTANT); }
"VEHICLE_FLAG_CAMERA_DECOUPLED"		{ count(); yylval.ival = VEHICLE_FLAG_CAMERA_DECOUPLED; return(INTEGER_CONSTANT); }



"PRIM_TYPE"				{ count(); yylval.ival = LSL_PRIM_TYPE; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL"			{ count(); yylval.ival = LSL_PRIM_MATERIAL; return(INTEGER_CONSTANT); }
"PRIM_PHYSICS"			{ count(); yylval.ival = LSL_PRIM_PHYSICS; return(INTEGER_CONSTANT); }
"PRIM_FLEXIBLE"			{ count(); yylval.ival = LSL_PRIM_FLEXIBLE; return(INTEGER_CONSTANT); }
"PRIM_POINT_LIGHT"		{ count(); yylval.ival = LSL_PRIM_POINT_LIGHT; return(INTEGER_CONSTANT); }
"PRIM_TEMP_ON_REZ"		{ count(); yylval.ival = LSL_PRIM_TEMP_ON_REZ; return(INTEGER_CONSTANT); }
"PRIM_PHANTOM"			{ count(); yylval.ival = LSL_PRIM_PHANTOM; return(INTEGER_CONSTANT); }
"PRIM_CAST_SHADOWS"		{ count(); yylval.ival = LSL_PRIM_CAST_SHADOWS; return(INTEGER_CONSTANT); }
"PRIM_POSITION"			{ count(); yylval.ival = LSL_PRIM_POSITION; return(INTEGER_CONSTANT); }
"PRIM_SIZE"				{ count(); yylval.ival = LSL_PRIM_SIZE; return(INTEGER_CONSTANT); }
"PRIM_ROTATION"			{ count(); yylval.ival = LSL_PRIM_ROTATION; return(INTEGER_CONSTANT); }
"PRIM_TEXTURE"			{ count(); yylval.ival = LSL_PRIM_TEXTURE; return(INTEGER_CONSTANT); }
"PRIM_COLOR"			{ count(); yylval.ival = LSL_PRIM_COLOR; return(INTEGER_CONSTANT); }
"PRIM_BUMP_SHINY"		{ count(); yylval.ival = LSL_PRIM_BUMP_SHINY; return(INTEGER_CONSTANT); }
"PRIM_FULLBRIGHT"		{ count(); yylval.ival = LSL_PRIM_FULLBRIGHT; return(INTEGER_CONSTANT); }
"PRIM_TEXGEN"			{ count(); yylval.ival = LSL_PRIM_TEXGEN; return(INTEGER_CONSTANT); }
"PRIM_GLOW"	     		{ count(); yylval.ival = LSL_PRIM_GLOW; return(INTEGER_CONSTANT); }

"PRIM_TYPE_BOX"			{ count(); yylval.ival = LSL_PRIM_TYPE_BOX; return(INTEGER_CONSTANT); }
"PRIM_TYPE_CYLINDER"	{ count(); yylval.ival = LSL_PRIM_TYPE_CYLINDER; return(INTEGER_CONSTANT); }
"PRIM_TYPE_PRISM"		{ count(); yylval.ival = LSL_PRIM_TYPE_PRISM; return(INTEGER_CONSTANT); }
"PRIM_TYPE_SPHERE"		{ count(); yylval.ival = LSL_PRIM_TYPE_SPHERE; return(INTEGER_CONSTANT); }
"PRIM_TYPE_TORUS"		{ count(); yylval.ival = LSL_PRIM_TYPE_TORUS; return(INTEGER_CONSTANT); }
"PRIM_TYPE_TUBE"		{ count(); yylval.ival = LSL_PRIM_TYPE_TUBE; return(INTEGER_CONSTANT); }
"PRIM_TYPE_RING"		{ count(); yylval.ival = LSL_PRIM_TYPE_RING; return(INTEGER_CONSTANT); }
"PRIM_TYPE_SCULPT"		{ count(); yylval.ival = LSL_PRIM_TYPE_SCULPT; return(INTEGER_CONSTANT); }

"PRIM_HOLE_DEFAULT"		{ count(); yylval.ival = LSL_PRIM_HOLE_DEFAULT; return(INTEGER_CONSTANT); }
"PRIM_HOLE_CIRCLE"		{ count(); yylval.ival = LSL_PRIM_HOLE_CIRCLE; return(INTEGER_CONSTANT); }
"PRIM_HOLE_SQUARE"		{ count(); yylval.ival = LSL_PRIM_HOLE_SQUARE; return(INTEGER_CONSTANT); }
"PRIM_HOLE_TRIANGLE"	{ count(); yylval.ival = LSL_PRIM_HOLE_TRIANGLE; return(INTEGER_CONSTANT); }

"PRIM_MATERIAL_STONE"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_STONE; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_METAL"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_METAL; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_GLASS"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_GLASS; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_WOOD"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_WOOD; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_FLESH"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_FLESH; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_PLASTIC"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_PLASTIC; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_RUBBER"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_RUBBER; return(INTEGER_CONSTANT); }
"PRIM_MATERIAL_LIGHT"	{ count(); yylval.ival = LSL_PRIM_MATERIAL_LIGHT; return(INTEGER_CONSTANT); }

"PRIM_SHINY_NONE"		{ count(); yylval.ival = LSL_PRIM_SHINY_NONE; return(INTEGER_CONSTANT); }
"PRIM_SHINY_LOW"		{ count(); yylval.ival = LSL_PRIM_SHINY_LOW; return(INTEGER_CONSTANT); }
"PRIM_SHINY_MEDIUM"		{ count(); yylval.ival = LSL_PRIM_SHINY_MEDIUM; return(INTEGER_CONSTANT); }
"PRIM_SHINY_HIGH"		{ count(); yylval.ival = LSL_PRIM_SHINY_HIGH; return(INTEGER_CONSTANT); }

"PRIM_BUMP_NONE"		{ count(); yylval.ival = LSL_PRIM_BUMP_NONE; return(INTEGER_CONSTANT); }
"PRIM_BUMP_BRIGHT"		{ count(); yylval.ival = LSL_PRIM_BUMP_BRIGHT; return(INTEGER_CONSTANT); }
"PRIM_BUMP_DARK"		{ count(); yylval.ival = LSL_PRIM_BUMP_DARK; return(INTEGER_CONSTANT); }
"PRIM_BUMP_WOOD"		{ count(); yylval.ival = LSL_PRIM_BUMP_WOOD; return(INTEGER_CONSTANT); }
"PRIM_BUMP_BARK"		{ count(); yylval.ival = LSL_PRIM_BUMP_BARK; return(INTEGER_CONSTANT); }
"PRIM_BUMP_BRICKS"		{ count(); yylval.ival = LSL_PRIM_BUMP_BRICKS; return(INTEGER_CONSTANT); }
"PRIM_BUMP_CHECKER"		{ count(); yylval.ival = LSL_PRIM_BUMP_CHECKER; return(INTEGER_CONSTANT); }
"PRIM_BUMP_CONCRETE"	{ count(); yylval.ival = LSL_PRIM_BUMP_CONCRETE; return(INTEGER_CONSTANT); }
"PRIM_BUMP_TILE"		{ count(); yylval.ival = LSL_PRIM_BUMP_TILE; return(INTEGER_CONSTANT); }
"PRIM_BUMP_STONE"		{ count(); yylval.ival = LSL_PRIM_BUMP_STONE; return(INTEGER_CONSTANT); }
"PRIM_BUMP_DISKS"		{ count(); yylval.ival = LSL_PRIM_BUMP_DISKS; return(INTEGER_CONSTANT); }
"PRIM_BUMP_GRAVEL"		{ count(); yylval.ival = LSL_PRIM_BUMP_GRAVEL; return(INTEGER_CONSTANT); }
"PRIM_BUMP_BLOBS"		{ count(); yylval.ival = LSL_PRIM_BUMP_BLOBS; return(INTEGER_CONSTANT); }
"PRIM_BUMP_SIDING"		{ count(); yylval.ival = LSL_PRIM_BUMP_SIDING; return(INTEGER_CONSTANT); }
"PRIM_BUMP_LARGETILE"	{ count(); yylval.ival = LSL_PRIM_BUMP_LARGETILE; return(INTEGER_CONSTANT); }
"PRIM_BUMP_STUCCO"		{ count(); yylval.ival = LSL_PRIM_BUMP_STUCCO; return(INTEGER_CONSTANT); }
"PRIM_BUMP_SUCTION"		{ count(); yylval.ival = LSL_PRIM_BUMP_SUCTION; return(INTEGER_CONSTANT); }
"PRIM_BUMP_WEAVE"		{ count(); yylval.ival = LSL_PRIM_BUMP_WEAVE; return(INTEGER_CONSTANT); }

"PRIM_TEXGEN_DEFAULT"	{ count(); yylval.ival = LSL_PRIM_TEXGEN_DEFAULT; return(INTEGER_CONSTANT); }
"PRIM_TEXGEN_PLANAR"	{ count(); yylval.ival = LSL_PRIM_TEXGEN_PLANAR; return(INTEGER_CONSTANT); }

"PRIM_SCULPT_TYPE_SPHERE"	{ count(); yylval.ival = LSL_PRIM_SCULPT_TYPE_SPHERE; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_TYPE_TORUS"	{ count(); yylval.ival = LSL_PRIM_SCULPT_TYPE_TORUS; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_TYPE_PLANE"	{ count(); yylval.ival = LSL_PRIM_SCULPT_TYPE_PLANE; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_TYPE_CYLINDER"	{ count(); yylval.ival = LSL_PRIM_SCULPT_TYPE_CYLINDER; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_TYPE_MASK" 	{ count(); yylval.ival = LSL_PRIM_SCULPT_TYPE_MASK; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_FLAG_MIRROR" 	{ count(); yylval.ival = LSL_PRIM_SCULPT_FLAG_MIRROR; return(INTEGER_CONSTANT); }
"PRIM_SCULPT_FLAG_INVERT" 	{ count(); yylval.ival = LSL_PRIM_SCULPT_FLAG_INVERT; return(INTEGER_CONSTANT); }

"MASK_BASE"				{ count(); yylval.ival = 0; return(INTEGER_CONSTANT); }
"MASK_OWNER"			{ count(); yylval.ival = 1; return(INTEGER_CONSTANT); }
"MASK_GROUP"			{ count(); yylval.ival = 2; return(INTEGER_CONSTANT); }
"MASK_EVERYONE"			{ count(); yylval.ival = 3; return(INTEGER_CONSTANT); }
"MASK_NEXT"				{ count(); yylval.ival = 4; return(INTEGER_CONSTANT); }

"PERM_TRANSFER"			{ count(); yylval.ival = PERM_TRANSFER; return(INTEGER_CONSTANT); }
"PERM_MODIFY"			{ count(); yylval.ival = PERM_MODIFY; return(INTEGER_CONSTANT); }
"PERM_COPY"				{ count(); yylval.ival = PERM_COPY; return(INTEGER_CONSTANT); }
"PERM_MOVE"				{ count(); yylval.ival = PERM_MOVE; return(INTEGER_CONSTANT); }
"PERM_ALL"				{ count(); yylval.ival = PERM_ALL; return(INTEGER_CONSTANT); }

"PARCEL_MEDIA_COMMAND_STOP"		{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_STOP; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_PAUSE"	{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_PAUSE; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_PLAY"		{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_PLAY; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_LOOP"		{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_LOOP; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_TEXTURE"	{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_TEXTURE; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_URL"		{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_URL; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_TIME"		{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_TIME; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_AGENT"	{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_AGENT; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_UNLOAD"	{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_UNLOAD; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_AUTO_ALIGN"	{ count(); yylval.ival = PARCEL_MEDIA_COMMAND_AUTO_ALIGN; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_TYPE"     { count(); yylval.ival = PARCEL_MEDIA_COMMAND_TYPE; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_SIZE"     { count(); yylval.ival = PARCEL_MEDIA_COMMAND_SIZE; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_DESC"     { count(); yylval.ival = PARCEL_MEDIA_COMMAND_DESC; return(INTEGER_CONSTANT); }
"PARCEL_MEDIA_COMMAND_LOOP_SET" { count(); yylval.ival = PARCEL_MEDIA_COMMAND_LOOP_SET; return(INTEGER_CONSTANT); }

"LIST_STAT_MAX"			{ count(); yylval.ival = LIST_STAT_MAX; return(INTEGER_CONSTANT); }
"LIST_STAT_MIN"			{ count(); yylval.ival = LIST_STAT_MIN; return(INTEGER_CONSTANT); }
"LIST_STAT_MEAN"		{ count(); yylval.ival = LIST_STAT_MEAN; return(INTEGER_CONSTANT); }
"LIST_STAT_MEDIAN"		{ count(); yylval.ival = LIST_STAT_MEDIAN; return(INTEGER_CONSTANT); }
"LIST_STAT_STD_DEV"		{ count(); yylval.ival = LIST_STAT_STD_DEV; return(INTEGER_CONSTANT); }
"LIST_STAT_SUM"		{ count(); yylval.ival = LIST_STAT_SUM; return(INTEGER_CONSTANT); }
"LIST_STAT_SUM_SQUARES"		{ count(); yylval.ival = LIST_STAT_SUM_SQUARES; return(INTEGER_CONSTANT); }
"LIST_STAT_NUM_COUNT"		{ count(); yylval.ival = LIST_STAT_NUM_COUNT; return(INTEGER_CONSTANT); }
"LIST_STAT_GEOMETRIC_MEAN"		{ count(); yylval.ival = LIST_STAT_GEO_MEAN; return(INTEGER_CONSTANT); }
"LIST_STAT_RANGE"		{ count(); yylval.ival = LIST_STAT_RANGE; return(INTEGER_CONSTANT); }

"PAY_HIDE"		{ count(); yylval.ival = PAY_PRICE_HIDE; return(INTEGER_CONSTANT); }
"PAY_DEFAULT"	{ count(); yylval.ival = PAY_PRICE_DEFAULT; return(INTEGER_CONSTANT); }

"PARCEL_FLAG_ALLOW_FLY"		{ count(); yylval.ival = PF_ALLOW_FLY; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_GROUP_SCRIPTS"		{ count(); yylval.ival = PF_ALLOW_GROUP_SCRIPTS; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_SCRIPTS"		{ count(); yylval.ival = PF_ALLOW_OTHER_SCRIPTS; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_LANDMARK"		{ count(); yylval.ival = PF_ALLOW_LANDMARK; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_TERRAFORM"		{ count(); yylval.ival = PF_ALLOW_TERRAFORM; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_DAMAGE"		{ count(); yylval.ival = PF_ALLOW_DAMAGE; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_CREATE_OBJECTS"		{ count(); yylval.ival = PF_CREATE_OBJECTS; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_CREATE_GROUP_OBJECTS"		{ count(); yylval.ival = PF_CREATE_GROUP_OBJECTS; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_USE_ACCESS_GROUP"		{ count(); yylval.ival = PF_USE_ACCESS_GROUP; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_USE_ACCESS_LIST"		{ count(); yylval.ival = PF_USE_ACCESS_LIST; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_USE_BAN_LIST"		{ count(); yylval.ival = PF_USE_BAN_LIST; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_USE_LAND_PASS_LIST"		{ count(); yylval.ival = PF_USE_PASS_LIST; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_LOCAL_SOUND_ONLY"		{ count(); yylval.ival = PF_SOUND_LOCAL; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_RESTRICT_PUSHOBJECT"		{ count(); yylval.ival = PF_RESTRICT_PUSHOBJECT; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_GROUP_OBJECT_ENTRY"		{ count(); yylval.ival = PF_ALLOW_GROUP_OBJECT_ENTRY; return(INTEGER_CONSTANT); }
"PARCEL_FLAG_ALLOW_ALL_OBJECT_ENTRY"		{ count(); yylval.ival = PF_ALLOW_ALL_OBJECT_ENTRY; return(INTEGER_CONSTANT); }

"REGION_FLAG_ALLOW_DAMAGE"		{ count(); yylval.ival = REGION_FLAGS_ALLOW_DAMAGE; return(INTEGER_CONSTANT); }
"REGION_FLAG_FIXED_SUN"		{ count(); yylval.ival = REGION_FLAGS_SUN_FIXED; return(INTEGER_CONSTANT); }
"REGION_FLAG_BLOCK_TERRAFORM"		{ count(); yylval.ival = REGION_FLAGS_BLOCK_TERRAFORM; return(INTEGER_CONSTANT); }
"REGION_FLAG_SANDBOX"		{ count(); yylval.ival = REGION_FLAGS_SANDBOX; return(INTEGER_CONSTANT); }
"REGION_FLAG_DISABLE_COLLISIONS"		{ count(); yylval.ival = REGION_FLAGS_SKIP_COLLISIONS; return(INTEGER_CONSTANT); }
"REGION_FLAG_DISABLE_PHYSICS"		{ count(); yylval.ival = REGION_FLAGS_SKIP_PHYSICS; return(INTEGER_CONSTANT); }
"REGION_FLAG_BLOCK_FLY"		{ count(); yylval.ival = REGION_FLAGS_BLOCK_FLY; return(INTEGER_CONSTANT); }
"REGION_FLAG_ALLOW_DIRECT_TELEPORT"		{ count(); yylval.ival = REGION_FLAGS_ALLOW_DIRECT_TELEPORT; return(INTEGER_CONSTANT); }
"REGION_FLAG_RESTRICT_PUSHOBJECT"		{ count(); yylval.ival = REGION_FLAGS_RESTRICT_PUSHOBJECT; return(INTEGER_CONSTANT); }

"HTTP_METHOD" { count(); yylval.ival = HTTP_METHOD; return(INTEGER_CONSTANT); }
"HTTP_MIMETYPE" { count(); yylval.ival = HTTP_MIMETYPE; return(INTEGER_CONSTANT); }
"HTTP_BODY_MAXLENGTH" { count(); yylval.ival = HTTP_BODY_MAXLENGTH; return(INTEGER_CONSTANT); }
"HTTP_BODY_TRUNCATED" { count(); yylval.ival = HTTP_BODY_TRUNCATED; return(INTEGER_CONSTANT); }
"HTTP_VERIFY_CERT" { count(); yylval.ival = HTTP_VERIFY_CERT; return(INTEGER_CONSTANT); }

"PARCEL_COUNT_TOTAL"		{ count(); yylval.ival = OC_TOTAL; return(INTEGER_CONSTANT); }
"PARCEL_COUNT_OWNER"		{ count(); yylval.ival = OC_OWNER; return(INTEGER_CONSTANT); }
"PARCEL_COUNT_GROUP"		{ count(); yylval.ival = OC_GROUP; return(INTEGER_CONSTANT); }
"PARCEL_COUNT_OTHER"		{ count(); yylval.ival = OC_OTHER; return(INTEGER_CONSTANT); }
"PARCEL_COUNT_SELECTED"	{ count(); yylval.ival = OC_SELECTED; return(INTEGER_CONSTANT); }
"PARCEL_COUNT_TEMP"		{ count(); yylval.ival = OC_TEMP; return(INTEGER_CONSTANT); }

"PARCEL_DETAILS_NAME"	{ count(); yylval.ival = PARCEL_DETAILS_NAME; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_DESC"	{ count(); yylval.ival = PARCEL_DETAILS_DESC; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_OWNER"	{ count(); yylval.ival = PARCEL_DETAILS_OWNER; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_GROUP"	{ count(); yylval.ival = PARCEL_DETAILS_GROUP; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_AREA"	{ count(); yylval.ival = PARCEL_DETAILS_AREA; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_ID"		{ count(); yylval.ival = PARCEL_DETAILS_ID; return(INTEGER_CONSTANT); }
"PARCEL_DETAILS_SEE_AVATARS"	{ count(); yylval.ival = PARCEL_DETAILS_SEE_AVATARS; return(INTEGER_CONSTANT); }

"STRING_TRIM_HEAD"	{ count(); yylval.ival = STRING_TRIM_HEAD; return(INTEGER_CONSTANT); }
"STRING_TRIM_TAIL"	{ count(); yylval.ival = STRING_TRIM_TAIL; return(INTEGER_CONSTANT); }
"STRING_TRIM"	{ count(); yylval.ival = STRING_TRIM; return(INTEGER_CONSTANT); }

"CLICK_ACTION_NONE"       { count(); yylval.ival = CLICK_ACTION_NONE; return(INTEGER_CONSTANT); }
"CLICK_ACTION_TOUCH"      { count(); yylval.ival = CLICK_ACTION_TOUCH; return(INTEGER_CONSTANT); }
"CLICK_ACTION_SIT"        { count(); yylval.ival = CLICK_ACTION_SIT; return(INTEGER_CONSTANT); }
"CLICK_ACTION_BUY"        { count(); yylval.ival = CLICK_ACTION_BUY; return(INTEGER_CONSTANT); }
"CLICK_ACTION_PAY"        { count(); yylval.ival = CLICK_ACTION_PAY; return(INTEGER_CONSTANT); }
"CLICK_ACTION_OPEN"       { count(); yylval.ival = CLICK_ACTION_OPEN; return(INTEGER_CONSTANT); }
"CLICK_ACTION_PLAY"       { count(); yylval.ival = CLICK_ACTION_PLAY; return(INTEGER_CONSTANT); }
"CLICK_ACTION_OPEN_MEDIA" { count(); yylval.ival = CLICK_ACTION_OPEN_MEDIA; return(INTEGER_CONSTANT); }
"CLICK_ACTION_ZOOM"       { count(); yylval.ival = CLICK_ACTION_ZOOM; return(INTEGER_CONSTANT); }

"TEXTURE_BLANK"           { yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "5748decc-f629-461c-9a36-a35a221fe21f"); return(STRING_CONSTANT); }
"TEXTURE_DEFAULT"         { yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "89556747-24cb-43ed-920b-47caed15465f"); return(STRING_CONSTANT); }
"TEXTURE_MEDIA"           { yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "8b5fec65-8d8d-9dc5-cda8-8fdf2716e361"); return(STRING_CONSTANT); }
"TEXTURE_PLYWOOD"         { yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "89556747-24cb-43ed-920b-47caed15465f"); return(STRING_CONSTANT); }
"TEXTURE_TRANSPARENT"     { yylval.sval = new char[UUID_STR_LENGTH]; strcpy(yylval.sval, "8dcd4a48-2d37-4909-9f78-f7a9eb4ef903"); return(STRING_CONSTANT); }

"TOUCH_INVALID_FACE"	  { count(); yylval.ival = -1; return(INTEGER_CONSTANT); }
"TOUCH_INVALID_VECTOR"	  { count(); return(TOUCH_INVALID_VECTOR); }
"TOUCH_INVALID_TEXCOORD"  { count(); return(TOUCH_INVALID_TEXCOORD); }

"PRIM_MEDIA_ALT_IMAGE_ENABLE"        { count(); yylval.ival = LLMediaEntry::ALT_IMAGE_ENABLE_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_CONTROLS"                { count(); yylval.ival = LLMediaEntry::CONTROLS_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_CURRENT_URL"             { count(); yylval.ival = LLMediaEntry::CURRENT_URL_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_HOME_URL"                { count(); yylval.ival = LLMediaEntry::HOME_URL_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_AUTO_LOOP"               { count(); yylval.ival = LLMediaEntry::AUTO_LOOP_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_AUTO_PLAY"               { count(); yylval.ival = LLMediaEntry::AUTO_PLAY_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_AUTO_SCALE"              { count(); yylval.ival = LLMediaEntry::AUTO_SCALE_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_AUTO_ZOOM"               { count(); yylval.ival = LLMediaEntry::AUTO_ZOOM_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_FIRST_CLICK_INTERACT"    { count(); yylval.ival = LLMediaEntry::FIRST_CLICK_INTERACT_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_WIDTH_PIXELS"            { count(); yylval.ival = LLMediaEntry::WIDTH_PIXELS_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_HEIGHT_PIXELS"           { count(); yylval.ival = LLMediaEntry::HEIGHT_PIXELS_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_WHITELIST_ENABLE"        { count(); yylval.ival = LLMediaEntry::WHITELIST_ENABLE_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_WHITELIST"               { count(); yylval.ival = LLMediaEntry::WHITELIST_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PERMS_INTERACT"          { count(); yylval.ival = LLMediaEntry::PERMS_INTERACT_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PERMS_CONTROL"           { count(); yylval.ival = LLMediaEntry::PERMS_CONTROL_ID; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PARAM_MAX"               { count(); yylval.ival = LLMediaEntry::PARAM_MAX_ID; return(INTEGER_CONSTANT); }

"PRIM_MEDIA_CONTROLS_STANDARD"       { count(); yylval.ival = LLMediaEntry::STANDARD; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_CONTROLS_MINI"           { count(); yylval.ival = LLMediaEntry::MINI; return(INTEGER_CONSTANT); }

"PRIM_MEDIA_PERM_NONE"               { count(); yylval.ival = LLMediaEntry::PERM_NONE; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PERM_OWNER"              { count(); yylval.ival = LLMediaEntry::PERM_OWNER; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PERM_GROUP"              { count(); yylval.ival = LLMediaEntry::PERM_GROUP; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_PERM_ANYONE"             { count(); yylval.ival = LLMediaEntry::PERM_ANYONE; return(INTEGER_CONSTANT); }

"PRIM_MEDIA_MAX_URL_LENGTH"          { count(); yylval.ival = LLMediaEntry::MAX_URL_LENGTH; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_MAX_WHITELIST_SIZE"      { count(); yylval.ival = LLMediaEntry::MAX_WHITELIST_SIZE; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_MAX_WHITELIST_COUNT"     { count(); yylval.ival = LLMediaEntry::MAX_WHITELIST_COUNT; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_MAX_WIDTH_PIXELS"        { count(); yylval.ival = LLMediaEntry::MAX_WIDTH_PIXELS; return(INTEGER_CONSTANT); }
"PRIM_MEDIA_MAX_HEIGHT_PIXELS"       { count(); yylval.ival = LLMediaEntry::MAX_HEIGHT_PIXELS; return(INTEGER_CONSTANT); }

"STATUS_OK"                          { count(); yylval.ival = LSL_STATUS_OK; return(INTEGER_CONSTANT); }
"STATUS_MALFORMED_PARAMS"            { count(); yylval.ival = LSL_STATUS_MALFORMED_PARAMS; return(INTEGER_CONSTANT); }
"STATUS_TYPE_MISMATCH"               { count(); yylval.ival = LSL_STATUS_TYPE_MISMATCH; return(INTEGER_CONSTANT); }
"STATUS_BOUNDS_ERROR"                { count(); yylval.ival = LSL_STATUS_BOUNDS_ERROR; return(INTEGER_CONSTANT); }
"STATUS_NOT_FOUND"                   { count(); yylval.ival = LSL_STATUS_NOT_FOUND; return(INTEGER_CONSTANT); }
"STATUS_NOT_SUPPORTED"               { count(); yylval.ival = LSL_STATUS_NOT_SUPPORTED; return(INTEGER_CONSTANT); }
"STATUS_INTERNAL_ERROR"              { count(); yylval.ival = LSL_STATUS_INTERNAL_ERROR; return(INTEGER_CONSTANT); }
"STATUS_WHITELIST_FAILED"            { count(); yylval.ival = LSL_STATUS_WHITELIST_FAILED; return(INTEGER_CONSTANT); }

{L}({L}|{N})*		{ count(); yylval.sval = new char[strlen(yytext) + 1]; strcpy(yylval.sval, yytext); return(IDENTIFIER); }

{N}+{E}					{ count(); yylval.fval = (F32)atof(yytext); return(FP_CONSTANT); }
{N}*"."{N}+({E})?{FS}?	{ count(); yylval.fval = (F32)atof(yytext); return(FP_CONSTANT); }
{N}+"."{N}*({E})?{FS}?	{ count(); yylval.fval = (F32)atof(yytext); return(FP_CONSTANT); }

L?\"(\\.|[^\\"])*\"	{ parse_string(); count(); return(STRING_CONSTANT); }

"++"				{ count(); return(INC_OP); }
"--"				{ count(); return(DEC_OP); }
"+="				{ count(); return(ADD_ASSIGN); }
"-="				{ count(); return(SUB_ASSIGN); }
"*="				{ count(); return(MUL_ASSIGN); }
"/="				{ count(); return(DIV_ASSIGN); }
"%="				{ count(); return(MOD_ASSIGN); }
";"					{ count(); return(';'); }
"{"					{ count(); return('{'); }
"}"					{ count(); return('}'); }
","					{ count(); return(','); }
"="					{ count(); return('='); }
"("					{ count(); return('('); }
")"					{ count(); return(')'); }
"-"					{ count(); return('-'); }
"+"					{ count(); return('+'); }
"*"					{ count(); return('*'); }
"/"					{ count(); return('/'); }
"%"					{ count(); return('%'); }
"@"					{ count(); return('@'); }
":"					{ count(); return(':'); }
">"					{ count(); return('>'); }
"<"					{ count(); return('<'); }
"]"					{ count(); return(']'); }
"["					{ count(); return('['); }
"=="				{ count(); return(EQ);  }
"!="				{ count(); return(NEQ);  }
">="				{ count(); return(GEQ);  }
"<="				{ count(); return(LEQ);  }
"&"					{ count(); return('&');  }
"|"					{ count(); return('|');  }
"^"					{ count(); return('^');  }
"~"					{ count(); return('~');  }
"!"					{ count(); return('!');  }
"&&"				{ count(); return(BOOLEAN_AND);	}
"||"				{ count(); return(BOOLEAN_OR);	}
"<<"				{ count(); return(SHIFT_LEFT);  }
">>"				{ count(); return(SHIFT_RIGHT); }

[ \t\v\n\f]			{ count(); }
.					{ /* ignore bad characters */ }

%%

LLScriptAllocationManager	*gAllocationManager;
LLScriptScript				*gScriptp;

// Prototype for the yacc parser entry point
int yyparse(void);

int yyerror(const char *fmt, ...)
{
	gErrorToText.writeError(yyout, gLine, gColumn, LSERROR_SYNTAX_ERROR);
	return 0;
}

//#define EMERGENCY_DEBUG_PRINTOUTS
//#define EMIT_CIL_ASSEMBLER

BOOL lscript_compile(const char* src_filename, const char* dst_filename,
					 const char* err_filename, BOOL compile_to_mono, const char* class_name, BOOL is_god_like)
{
	BOOL			b_parse_ok = FALSE;
	BOOL			b_dummy = FALSE;
	U64				b_dummy_count = FALSE;
	LSCRIPTType		type = LST_NULL;

	gInternalColumn = 0;
	gInternalLine = 0;
	gScriptp = NULL;

	gErrorToText.init();
	init_supported_expressions();
	init_temp_jumps();
	gAllocationManager = new LLScriptAllocationManager();

	yyin = LLFile::fopen(std::string(src_filename), "r");
	if (yyin)
	{
		yyout = LLFile::fopen(std::string(err_filename), "w");

		// Reset the lexer's internal buffering.

	    yyrestart(yyin);

		b_parse_ok = !yyparse();

		if (b_parse_ok)
		{
#ifdef EMERGENCY_DEBUG_PRINTOUTS
			char compiled[256];
			sprintf(compiled, "%s.o", src_filename);
			LLFILE* compfile;
			compfile = LLFile::fopen(compiled, "w");
#endif

			if(dst_filename)
			{
				gScriptp->setBytecodeDest(dst_filename);
			}

			gScriptp->mGodLike = is_god_like;
			
			gScriptp->setClassName(class_name);

			gScopeStringTable = new LLStringTable(16384);
#ifdef EMERGENCY_DEBUG_PRINTOUTS
			gScriptp->recurse(compfile, 0, 4, LSCP_PRETTY_PRINT, LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
#endif
			gScriptp->recurse(yyout, 0, 0, LSCP_PRUNE,		 LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
			gScriptp->recurse(yyout, 0, 0, LSCP_SCOPE_PASS1, LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
			gScriptp->recurse(yyout, 0, 0, LSCP_SCOPE_PASS2, LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
			gScriptp->recurse(yyout, 0, 0, LSCP_TYPE,		 LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
			if (!gErrorToText.getErrors())
			{
				gScriptp->recurse(yyout, 0, 0, LSCP_RESOURCE, LSPRUNE_INVALID,		 b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
#ifdef EMERGENCY_DEBUG_PRINTOUTS
				gScriptp->recurse(yyout, 0, 0, LSCP_EMIT_ASSEMBLY, LSPRUNE_INVALID,  b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
#endif
				if(TRUE == compile_to_mono)
				{
					gScriptp->recurse(yyout, 0, 0, LSCP_EMIT_CIL_ASSEMBLY, LSPRUNE_INVALID,  b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
				}
				else
				{
					gScriptp->recurse(yyout, 0, 0, LSCP_EMIT_BYTE_CODE, LSPRUNE_INVALID, b_dummy, NULL, type, type, b_dummy_count, NULL, NULL, 0, NULL, 0, NULL);
				}
			}
			delete gScopeStringTable;
			gScopeStringTable = NULL;
#ifdef EMERGENCY_DEBUG_PRINTOUTS
			fclose(compfile);
#endif
		}
		fclose(yyout);
		fclose(yyin);
	}

	delete gAllocationManager;
	delete gScopeStringTable;
	
	return b_parse_ok && !gErrorToText.getErrors();
}


BOOL lscript_compile(char *filename, BOOL compile_to_mono, BOOL is_god_like = FALSE)
{
	char src_filename[MAX_STRING];
	sprintf(src_filename, "%s.lsl", filename);
	char err_filename[MAX_STRING];
	sprintf(err_filename, "%s.out", filename);
	char class_name[MAX_STRING];
	sprintf(class_name, "%s", filename);
	return lscript_compile(src_filename, NULL, err_filename, compile_to_mono, class_name, is_god_like);
}


S32 yywrap()
{
#if defined(FLEX_SCANNER) && !defined(LL_WINDOWS)
	// get gcc to stop complaining about lack of use of yyunput
	(void) yyunput;
#endif
	return(1);
}

void line_comment()
{
	char c;

	while ((c = yyinput()) != '\n' && c != 0 && c != EOF)
		;
}

void block_comment()
{
	char c1 = 0;
	char c2 = yyinput();
	while (c2 != 0 && c2 != EOF && !(c1 == '*' && c2 == '/')) {
		if (c2 == '\n')
		{
			gInternalLine++;
			gInternalColumn = 0;
		}
		else if (c2 == '\t')
			gInternalColumn += 4 - (gInternalColumn % 8);
		else
			gInternalColumn++;
		c1 = c2;
		c2 = yyinput();
	}
}

void count()
{
	S32 i;

	gColumn = gInternalColumn;
	gLine = gInternalLine;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
		{
			gInternalLine++;
			gInternalColumn = 0;
		}
		else if (yytext[i] == '\t')
			gInternalColumn += 4 - (gInternalColumn % 8);
		else
			gInternalColumn++;
}

void parse_string()
{
	S32 length = (S32)strlen(yytext);
	length = length - 2; 
	char *temp = yytext + 1;

	S32 i;
	S32 escapes = 0;
	S32 tabs = 0;
	for (i = 0; i < length; i++)
	{
		if (temp[i] == '\\')
		{
			escapes++;
			i++;
			if (temp[i] == 't')
				tabs++;
		}
	}

	S32 newlength = length - escapes + tabs*3;
	yylval.sval = new char[newlength + 1];

	char *dest = yylval.sval;

	for (i = 0; i < length; i++)
	{
		if (temp[i] == '\\')
		{
			i++;
			// linefeed
			if (temp[i] == 'n')
			{
				*dest++ = 10;
			}
			else if (temp[i] == 't')
			{
				*dest++ = ' ';
				*dest++ = ' ';
				*dest++ = ' ';
				*dest++ = ' ';
			}
			else
			{
				*dest++ = temp[i];
			}
		}
		else
		{
			*dest++ = temp[i];
		}
	}
	yylval.sval[newlength] = 0;
}
