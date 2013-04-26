%{
	#include "linden_common.h"
	#include "lscript_tree.h"

    #ifdef __cplusplus
    extern "C" {
    #endif

	int yylex(void);
	int yyparse( void );
	int yyerror(const char *fmt, ...);

    #if LL_LINUX
    // broken yacc codegen...  --ryan.
    #define getenv getenv_workaround
    #endif

    #ifdef LL_WINDOWS
	#pragma warning (disable : 4702) // warning C4702: unreachable code
	#pragma warning( disable : 4065 )	// warning: switch statement contains 'default' but no 'case' labels
	#endif

    #ifdef __cplusplus
    }
    #endif
%}

%union
{
	S32								ival;
	F32								fval;
	char							*sval;
	class LLScriptType				*type;
	class LLScriptConstant			*constant;
	class LLScriptIdentifier		*identifier;
	class LLScriptSimpleAssignable	*assignable;
	class LLScriptGlobalVariable	*global;
	class LLScriptEvent				*event;
	class LLScriptEventHandler		*handler;
	class LLScriptExpression		*expression;
	class LLScriptStatement			*statement;
	class LLScriptGlobalFunctions	*global_funcs;
	class LLScriptFunctionDec		*global_decl;
	class LLScriptState				*state;
	class LLScritpGlobalStorage		*global_store;
	class LLScriptScript			*script;
};

%token					INTEGER
%token					FLOAT_TYPE
%token					STRING
%token					LLKEY
%token					VECTOR
%token					QUATERNION
%token					LIST

%token					STATE
%token					EVENT
%token					JUMP
%token					RETURN

%token					STATE_ENTRY
%token					STATE_EXIT
%token					TOUCH_START
%token					TOUCH
%token					TOUCH_END
%token					COLLISION_START
%token					COLLISION
%token					COLLISION_END
%token					LAND_COLLISION_START
%token					LAND_COLLISION
%token					LAND_COLLISION_END
%token					TIMER
%token					CHAT
%token					SENSOR
%token					NO_SENSOR
%token					CONTROL
%token					AT_TARGET
%token					NOT_AT_TARGET
%token					AT_ROT_TARGET
%token					NOT_AT_ROT_TARGET
%token					MONEY
%token					EMAIL
%token					RUN_TIME_PERMISSIONS
%token					INVENTORY
%token					ATTACH
%token					DATASERVER
%token					MOVING_START
%token					MOVING_END
%token					REZ
%token					OBJECT_REZ
%token					LINK_MESSAGE
%token					REMOTE_DATA
%token					HTTP_RESPONSE
%token					HTTP_REQUEST

%token <sval>			IDENTIFIER
%token <sval>			STATE_DEFAULT

%token <ival>			INTEGER_CONSTANT
%token <ival>			INTEGER_TRUE
%token <ival>			INTEGER_FALSE

%token <fval>			FP_CONSTANT

%token <sval>			STRING_CONSTANT

%token					INC_OP
%token					DEC_OP
%token					ADD_ASSIGN
%token					SUB_ASSIGN
%token					MUL_ASSIGN
%token					DIV_ASSIGN
%token					MOD_ASSIGN

%token					EQ
%token					NEQ
%token					GEQ
%token					LEQ

%token					BOOLEAN_AND
%token					BOOLEAN_OR

%token					SHIFT_LEFT
%token					SHIFT_RIGHT

%token					IF
%token					ELSE
%token					FOR
%token					DO
%token					WHILE

%token					PRINT

%token					PERIOD

%token					ZERO_VECTOR
%token					ZERO_ROTATION

%token                  TOUCH_INVALID_VECTOR
%token                  TOUCH_INVALID_TEXCOORD

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE


%type <script>			lscript_program
%type <global_store>	globals
%type <global_store>	global
%type <global>			global_variable
%type <assignable>		simple_assignable
%type <assignable>		simple_assignable_no_list
%type <constant>		constant
%type <ival>			integer_constant
%type <fval>			fp_constant
%type <assignable>		special_constant
%type <assignable>		vector_constant
%type <assignable>		quaternion_constant
%type <assignable>		list_constant
%type <assignable>		list_entries
%type <assignable>		list_entry
%type <type>			typename
%type <global_funcs>	global_function
%type <global_decl>		function_parameters
%type <global_decl>		function_parameter
%type <state>			states
%type <state>			other_states
%type <state>			default
%type <state>			state
%type <handler>			state_body
%type <handler>			event
%type <event>			state_entry
%type <event>			state_exit
%type <event>			touch_start
%type <event>			touch
%type <event>			touch_end
%type <event>			collision_start
%type <event>			collision
%type <event>			collision_end
%type <event>			land_collision_start
%type <event>			land_collision
%type <event>			land_collision_end
%type <event>			at_target
%type <event>			not_at_target
%type <event>			at_rot_target
%type <event>			not_at_rot_target
%type <event>			money
%type <event>			email
%type <event>			run_time_permissions
%type <event>			inventory
%type <event>			attach
%type <event>			dataserver
%type <event>			moving_start
%type <event>			moving_end
%type <event>			rez
%type <event>			object_rez
%type <event>			remote_data
%type <event>			http_response
%type <event>			http_request
%type <event>			link_message
%type <event>			timer
%type <event>			chat
%type <event>			sensor
%type <event>			no_sensor
%type <event>			control
%type <statement>		compound_statement
%type <statement>		statement
%type <statement>		statements
%type <statement>		declaration
%type <statement>		';'
%type <statement>		'@'
%type <expression>		nextforexpressionlist
%type <expression>		forexpressionlist
%type <expression>		nextfuncexpressionlist
%type <expression>		funcexpressionlist
%type <expression>		nextlistexpressionlist
%type <expression>		listexpressionlist
%type <expression>		unarypostfixexpression
%type <expression>		vector_initializer
%type <expression>		quaternion_initializer
%type <expression>		list_initializer
%type <expression>		lvalue
%type <expression>		'-'
%type <expression>		'!'
%type <expression>		'~'
%type <expression>		'='
%type <expression>		'<'
%type <expression>		'>'
%type <expression>		'+'
%type <expression>		'*'
%type <expression>		'/'
%type <expression>		'%'
%type <expression>		'&'
%type <expression>		'|'
%type <expression>		'^'
%type <expression>		ADD_ASSIGN
%type <expression>		SUB_ASSIGN
%type <expression>		MUL_ASSIGN
%type <expression>		DIV_ASSIGN
%type <expression>		MOD_ASSIGN
%type <expression>		EQ
%type <expression>		NEQ
%type <expression>		LEQ
%type <expression>		GEQ
%type <expression>		BOOLEAN_AND
%type <expression>		BOOLEAN_OR
%type <expression>		SHIFT_LEFT
%type <expression>		SHIFT_RIGHT
%type <expression>		INC_OP
%type <expression>		DEC_OP
%type <expression>		'('
%type <expression>		')'
%type <expression>		PRINT
%type <identifier>		name_type
%type <expression>		expression
%type <expression>		unaryexpression
%type <expression>		typecast

%right '=' MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%left 	BOOLEAN_AND BOOLEAN_OR
%left	'|'
%left	'^'
%left	'&'
%left	EQ NEQ
%left	'<' LEQ '>' GEQ
%left	SHIFT_LEFT SHIFT_RIGHT
%left 	'+' '-'
%left	'*' '/' '%'
%right	'!' '~' INC_OP DEC_OP
%nonassoc INITIALIZER

%%

lscript_program
	: globals states		
	{
		$$ = new LLScriptScript($1, $2);
		gAllocationManager->addAllocation($$);
		gScriptp = $$;
	}
	| states		
	{
		$$ = new LLScriptScript(NULL, $1);
		gAllocationManager->addAllocation($$);
		gScriptp = $$;
	}
	;
	
globals
	: global																
	{
		$$ = $1;
	}
	| global globals
	{
		$$ = $1;
		$1->addGlobal($2);
	}
	;

global
	: global_variable
	{
		$$ = new LLScritpGlobalStorage($1);
		gAllocationManager->addAllocation($$);
	}
	| global_function
	{
		$$ = new LLScritpGlobalStorage($1);
		gAllocationManager->addAllocation($$);
	}
	;
	
name_type
	: typename IDENTIFIER
	{
		$$ = new LLScriptIdentifier(gLine, gColumn, $2, $1);	
		gAllocationManager->addAllocation($$);
	}
	;

global_variable
	: name_type ';'	
	{
		$$ = new LLScriptGlobalVariable(gLine, gColumn, $1->mType, $1, NULL);
		gAllocationManager->addAllocation($$);
	}
	| name_type '=' simple_assignable ';'
	{
		$$ = new LLScriptGlobalVariable(gLine, gColumn, $1->mType, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	;

simple_assignable
	: simple_assignable_no_list
	{
		$$ = $1;
	}
	| list_constant
	{
		$$ = $1;
	}
	;

simple_assignable_no_list
	: IDENTIFIER																	
	{
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptSAIdentifier(gLine, gColumn, id);	
		gAllocationManager->addAllocation($$);
	}
	| constant																		
	{
		$$ = new LLScriptSAConstant(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| special_constant	
	{
		$$ = $1;
	}
	;

constant
	: integer_constant
	{
		$$ = new LLScriptConstantInteger(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| fp_constant
	{
		$$ = new LLScriptConstantFloat(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| STRING_CONSTANT
	{
		$$ = new LLScriptConstantString(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	;

fp_constant
	: FP_CONSTANT
	{
		$$ = $1;
	}
	| '-' FP_CONSTANT
	{
		$$ = -$2;
	}
	;

integer_constant
	: INTEGER_CONSTANT
	{
		$$ = $1;
	}
	| INTEGER_TRUE
	{
		$$ = $1;
	}
	| INTEGER_FALSE
	{
		$$ = $1;
	}
	| '-' INTEGER_CONSTANT
	{
		$$ = -$2;
	}
	;

special_constant
	: vector_constant
	{
		$$ = $1;
	}
	| quaternion_constant
	{
		$$ = $1;
	}
	;

vector_constant
	: '<' simple_assignable ',' simple_assignable ',' simple_assignable '>'
	{
		$$ = new LLScriptSAVector(gLine, gColumn, $2, $4, $6);
		gAllocationManager->addAllocation($$);
	}
	| ZERO_VECTOR
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptSAConstant *sa0 = new LLScriptSAConstant(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptSAConstant *sa1 = new LLScriptSAConstant(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptSAConstant *sa2 = new LLScriptSAConstant(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptSAVector(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	| TOUCH_INVALID_VECTOR
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptSAConstant *sa0 = new LLScriptSAConstant(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptSAConstant *sa1 = new LLScriptSAConstant(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptSAConstant *sa2 = new LLScriptSAConstant(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptSAVector(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	| TOUCH_INVALID_TEXCOORD
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, -1.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptSAConstant *sa0 = new LLScriptSAConstant(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, -1.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptSAConstant *sa1 = new LLScriptSAConstant(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptSAConstant *sa2 = new LLScriptSAConstant(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptSAVector(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	;
	
quaternion_constant
	: '<' simple_assignable ',' simple_assignable ',' simple_assignable ',' simple_assignable '>'
	{
		$$ = new LLScriptSAQuaternion(gLine, gColumn, $2, $4, $6, $8);
		gAllocationManager->addAllocation($$);
	}
	| ZERO_ROTATION
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptSAConstant *sa0 = new LLScriptSAConstant(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptSAConstant *sa1 = new LLScriptSAConstant(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptSAConstant *sa2 = new LLScriptSAConstant(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		LLScriptConstantFloat *cf3 = new LLScriptConstantFloat(gLine, gColumn, 1.f);
		gAllocationManager->addAllocation(cf3);
		LLScriptSAConstant *sa3 = new LLScriptSAConstant(gLine, gColumn, cf3);
		gAllocationManager->addAllocation(sa3);
		$$ = new LLScriptSAQuaternion(gLine, gColumn, sa0, sa1, sa2, sa3);
		gAllocationManager->addAllocation($$);
	}
	;

list_constant
	: '[' list_entries ']'
	{
		$$ = new LLScriptSAList(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| '[' ']'
	{
		$$ = new LLScriptSAList(gLine, gColumn, NULL);
		gAllocationManager->addAllocation($$);
	}
	;

list_entries
	: list_entry																	
	{
		$$ = $1;
	}
	| list_entry ',' list_entries
	{
		$$ = $1;
		$1->addAssignable($3);
	}
	;

list_entry
	: simple_assignable_no_list
	{
		$$ = $1;
	}
	;	

typename
	: INTEGER																		
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_INTEGER);
		gAllocationManager->addAllocation($$);
	}
	| FLOAT_TYPE																			
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_FLOATINGPOINT);
		gAllocationManager->addAllocation($$);
	}
	| STRING																		
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_STRING);
		gAllocationManager->addAllocation($$);
	}
	| LLKEY																		
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_KEY);
		gAllocationManager->addAllocation($$);
	}
	| VECTOR																		
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_VECTOR);
		gAllocationManager->addAllocation($$);
	}
	| QUATERNION																	
	{  
		$$ = new LLScriptType(gLine, gColumn, LST_QUATERNION);
		gAllocationManager->addAllocation($$);
	}
	| LIST																			
	{
		$$ = new LLScriptType(gLine, gColumn, LST_LIST);
		gAllocationManager->addAllocation($$);
	}
	;
	
global_function
	: IDENTIFIER '(' ')' compound_statement
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptGlobalFunctions(gLine, gColumn, NULL, id, NULL, $4);
		gAllocationManager->addAllocation($$);
	}
	| name_type '(' ')' compound_statement
	{
		$$ = new LLScriptGlobalFunctions(gLine, gColumn, $1->mType, $1, NULL, $4);
		gAllocationManager->addAllocation($$);
	}
	| IDENTIFIER '(' function_parameters ')' compound_statement
	{
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptGlobalFunctions(gLine, gColumn, NULL, id, $3, $5);
		gAllocationManager->addAllocation($$);
	}
	| name_type '(' function_parameters ')' compound_statement
	{  
		$$ = new LLScriptGlobalFunctions(gLine, gColumn, $1->mType, $1, $3, $5);
		gAllocationManager->addAllocation($$);
	}
	;
	
function_parameters
	: function_parameter															
	{  
		$$ = $1;
	}
	| function_parameter ',' function_parameters									
	{  
		$$ = $1;
		$1->addFunctionParameter($3);
	}
	;
	
function_parameter
	: typename IDENTIFIER															
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptFunctionDec(gLine, gColumn, $1, id);
		gAllocationManager->addAllocation($$);
	}
	;

states
	: default																		
	{  
		$$ = $1;
	}
	| default other_states
	{  
		$$ = $1;
		$1->mNextp = $2;
	}
	;
	
other_states
	: state																			
	{  
		$$ = $1;
	}
	| state other_states 															
	{  
		$$ = $1;
		$1->addState($2);
	}
	;
	
default
	: STATE_DEFAULT '{' state_body '}'													
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptState(gLine, gColumn, LSSTYPE_DEFAULT, id, $3);
		gAllocationManager->addAllocation($$);
	}
	;
	
state
	: STATE IDENTIFIER '{' state_body '}'											
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptState(gLine, gColumn, LSSTYPE_USER, id, $4);
		gAllocationManager->addAllocation($$);
	}
	;
	
state_body	
	: event																			
	{  
		$$ = $1;
	}
	| event state_body															
	{  
		$$ = $1;
		$1->addEvent($2);
	}
	;
	
event
	: state_entry compound_statement												
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| state_exit compound_statement													
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| touch_start compound_statement												
	{
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| touch compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| touch_end compound_statement													
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| collision_start compound_statement											
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| collision compound_statement													
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| collision_end compound_statement												
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| land_collision_start compound_statement											
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| land_collision compound_statement													
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| land_collision_end compound_statement												
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| timer compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| chat compound_statement														
	{
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| sensor compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| no_sensor compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| at_target compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| not_at_target compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| at_rot_target compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| not_at_rot_target compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| money compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| email compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| run_time_permissions compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| inventory compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| attach compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| dataserver compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| control compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| moving_start compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| moving_end compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| rez compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| object_rez compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| link_message compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| remote_data compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| http_response compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	| http_request compound_statement														
	{  
		$$ = new LLScriptEventHandler(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	;
	
state_entry
	: STATE_ENTRY '(' ')'															
	{  
		$$ = new LLScriptStateEntryEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

state_exit
	: STATE_EXIT '(' ')'															
	{  
		$$ = new LLScriptStateExitEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

touch_start
	: TOUCH_START '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptTouchStartEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

touch
	: TOUCH '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptTouchEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

touch_end
	: TOUCH_END '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptTouchEndEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

collision_start
	: COLLISION_START '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptCollisionStartEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

collision
	: COLLISION '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptCollisionEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

collision_end
	: COLLISION_END '(' INTEGER IDENTIFIER ')'					
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptCollisionEndEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

land_collision_start
	: LAND_COLLISION_START '(' VECTOR IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptLandCollisionStartEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

land_collision
	: LAND_COLLISION '(' VECTOR IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptLandCollisionEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

land_collision_end
	: LAND_COLLISION_END '(' VECTOR IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptLandCollisionEndEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

at_target
	: AT_TARGET '(' INTEGER IDENTIFIER ',' VECTOR IDENTIFIER ',' VECTOR IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		$$ = new LLScriptAtTarget(gLine, gColumn, id1, id2, id3);
		gAllocationManager->addAllocation($$);
	}
	;

not_at_target
	: NOT_AT_TARGET '(' ')'															
	{  
		$$ = new LLScriptNotAtTarget(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

at_rot_target
	: AT_ROT_TARGET '(' INTEGER IDENTIFIER ',' QUATERNION IDENTIFIER ',' QUATERNION IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		$$ = new LLScriptAtRotTarget(gLine, gColumn, id1, id2, id3);
		gAllocationManager->addAllocation($$);
	}
	;

not_at_rot_target
	: NOT_AT_ROT_TARGET '(' ')'															
	{  
		$$ = new LLScriptNotAtRotTarget(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

money
	: MONEY '(' LLKEY IDENTIFIER ',' INTEGER IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		$$ = new LLScriptMoneyEvent(gLine, gColumn, id1, id2);
		gAllocationManager->addAllocation($$);
	}
	;

email
	: EMAIL '(' STRING IDENTIFIER ',' STRING IDENTIFIER ',' STRING IDENTIFIER ',' STRING IDENTIFIER ',' INTEGER IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		LLScriptIdentifier	*id4 = new LLScriptIdentifier(gLine, gColumn, $13);	
		gAllocationManager->addAllocation(id4);
		LLScriptIdentifier	*id5 = new LLScriptIdentifier(gLine, gColumn, $16);	
		gAllocationManager->addAllocation(id5);
		$$ = new LLScriptEmailEvent(gLine, gColumn, id1, id2, id3, id4, id5);
		gAllocationManager->addAllocation($$);
	}
	;

run_time_permissions
	: RUN_TIME_PERMISSIONS '(' INTEGER IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptRTPEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

inventory
	: INVENTORY '(' INTEGER IDENTIFIER ')'																	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptInventoryEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

attach
	: ATTACH '(' LLKEY IDENTIFIER ')'																	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptAttachEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

dataserver
	: DATASERVER '(' LLKEY IDENTIFIER ',' STRING IDENTIFIER')'																	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		$$ = new LLScriptDataserverEvent(gLine, gColumn, id1, id2);
		gAllocationManager->addAllocation($$);
	}
	;

moving_start
	: MOVING_START '(' ')'																	
	{  
		$$ = new LLScriptMovingStartEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

moving_end
	: MOVING_END '(' ')'																	
	{  
		$$ = new LLScriptMovingEndEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

timer
	: TIMER '(' ')'																	
	{  
		$$ = new LLScriptTimerEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

chat
	: CHAT '(' INTEGER IDENTIFIER ',' STRING IDENTIFIER ',' LLKEY IDENTIFIER ',' STRING IDENTIFIER ')'							
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		LLScriptIdentifier	*id4 = new LLScriptIdentifier(gLine, gColumn, $13);	
		gAllocationManager->addAllocation(id4);
		$$ = new LLScriptChatEvent(gLine, gColumn, id1, id2, id3, id4);
		gAllocationManager->addAllocation($$);
	}
	;

sensor
	: SENSOR '(' INTEGER IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptSensorEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

no_sensor
	: NO_SENSOR '(' ')'															
	{  
		$$ = new LLScriptNoSensorEvent(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	;

control
	: CONTROL '(' LLKEY IDENTIFIER ',' INTEGER IDENTIFIER ',' INTEGER IDENTIFIER ')'	
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		$$ = new LLScriptControlEvent(gLine, gColumn, id1, id2, id3);
		gAllocationManager->addAllocation($$);
	}
	;

rez
	: REZ '(' INTEGER IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptRezEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

object_rez
	: OBJECT_REZ '(' LLKEY IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		$$ = new LLScriptObjectRezEvent(gLine, gColumn, id1);
		gAllocationManager->addAllocation($$);
	}
	;

link_message
	: LINK_MESSAGE '(' INTEGER IDENTIFIER ','  INTEGER IDENTIFIER ',' STRING IDENTIFIER ',' LLKEY IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		LLScriptIdentifier	*id4 = new LLScriptIdentifier(gLine, gColumn, $13);	
		gAllocationManager->addAllocation(id4);
		$$ = new LLScriptLinkMessageEvent(gLine, gColumn, id1, id2, id3, id4);
		gAllocationManager->addAllocation($$);
	}
	;

remote_data
	: REMOTE_DATA '(' INTEGER IDENTIFIER ','  LLKEY IDENTIFIER ','  LLKEY IDENTIFIER ','  STRING IDENTIFIER ',' INTEGER IDENTIFIER ',' STRING IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		LLScriptIdentifier	*id4 = new LLScriptIdentifier(gLine, gColumn, $13);	
		gAllocationManager->addAllocation(id4);
		LLScriptIdentifier	*id5 = new LLScriptIdentifier(gLine, gColumn, $16);	
		gAllocationManager->addAllocation(id4);
		LLScriptIdentifier	*id6 = new LLScriptIdentifier(gLine, gColumn, $19);	
		gAllocationManager->addAllocation(id4);
		$$ = new LLScriptRemoteEvent(gLine, gColumn, id1, id2, id3, id4, id5, id6);
		gAllocationManager->addAllocation($$);
	}
	;

http_response
	: HTTP_RESPONSE '(' LLKEY IDENTIFIER ','  INTEGER IDENTIFIER ','  LIST IDENTIFIER ',' STRING IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		LLScriptIdentifier	*id4 = new LLScriptIdentifier(gLine, gColumn, $13);	
		gAllocationManager->addAllocation(id4);
		$$ = new LLScriptHTTPResponseEvent(gLine, gColumn, id1, id2, id3, id4);
		gAllocationManager->addAllocation($$);
	}
	;

http_request
	: HTTP_REQUEST '(' LLKEY IDENTIFIER ','  STRING IDENTIFIER ',' STRING IDENTIFIER ')'															
	{  
		LLScriptIdentifier	*id1 = new LLScriptIdentifier(gLine, gColumn, $4);	
		gAllocationManager->addAllocation(id1);
		LLScriptIdentifier	*id2 = new LLScriptIdentifier(gLine, gColumn, $7);	
		gAllocationManager->addAllocation(id2);
		LLScriptIdentifier	*id3 = new LLScriptIdentifier(gLine, gColumn, $10);	
		gAllocationManager->addAllocation(id3);
		$$ = new LLScriptHTTPRequestEvent(gLine, gColumn, id1, id2, id3);
		gAllocationManager->addAllocation($$);
	}
	;
	
compound_statement
	: '{' '}'																		
	{  
		$$ = new LLScriptCompoundStatement(gLine, gColumn, NULL);
		gAllocationManager->addAllocation($$);
	}
	| '{' statements '}'															
	{  
		$$ = new LLScriptCompoundStatement(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	;
	
statements
	: statement																		
	{  
		$$ = $1;
	}
	| statements statement															
	{  
		$$ = new LLScriptStatementSequence(gLine, gColumn, $1, $2);
		gAllocationManager->addAllocation($$);
	}
	;
	
statement
	: ';'																			
	{  
		$$ = new LLScriptNOOP(gLine, gColumn);
		gAllocationManager->addAllocation($$);
	}
	| STATE IDENTIFIER ';'						
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptStateChange(gLine, gColumn, id);
		gAllocationManager->addAllocation($$);
	}
	| STATE STATE_DEFAULT ';'						
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptStateChange(gLine, gColumn, id);
		gAllocationManager->addAllocation($$);
	}
	| JUMP IDENTIFIER ';'						
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptJump(gLine, gColumn, id);
		gAllocationManager->addAllocation($$);
	}
	| '@' IDENTIFIER ';'						
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptLabel(gLine, gColumn, id);
		gAllocationManager->addAllocation($$);
	}
	| RETURN expression ';'						
	{  
		$$ = new LLScriptReturn(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| RETURN ';'								
	{  
		$$ = new LLScriptReturn(gLine, gColumn, NULL);
		gAllocationManager->addAllocation($$);
	}
	| expression ';'							
	{  
		$$ = new LLScriptExpressionStatement(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| declaration ';'
	{  
		$$ = $1;
	}
	| compound_statement						
	{ 
		$$ = $1;
	}
	| IF '(' expression ')' statement	%prec LOWER_THAN_ELSE			
	{  
		$$ = new LLScriptIf(gLine, gColumn, $3, $5);
		$5->mAllowDeclarations = FALSE;
		gAllocationManager->addAllocation($$);
	}
	| IF '(' expression ')' statement ELSE statement					
	{  
		$$ = new LLScriptIfElse(gLine, gColumn, $3, $5, $7);
		$5->mAllowDeclarations = FALSE;
		$7->mAllowDeclarations = FALSE;
		gAllocationManager->addAllocation($$);
	}
	| FOR '(' forexpressionlist ';' expression ';' forexpressionlist ')' statement	
	{  
		$$ = new LLScriptFor(gLine, gColumn, $3, $5, $7, $9);
		$9->mAllowDeclarations = FALSE;
		gAllocationManager->addAllocation($$);
	}
	| DO statement WHILE '(' expression ')' ';' 
	{  
		$$ = new LLScriptDoWhile(gLine, gColumn, $2, $5);
		$2->mAllowDeclarations = FALSE;
		gAllocationManager->addAllocation($$);
	}
	| WHILE '(' expression ')' statement		
	{  
		$$ = new LLScriptWhile(gLine, gColumn, $3, $5);
		$5->mAllowDeclarations = FALSE;
		gAllocationManager->addAllocation($$);
	}
	;
	
declaration
	: typename IDENTIFIER						
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptDeclaration(gLine, gColumn, $1, id, NULL);
		gAllocationManager->addAllocation($$);
	}
	| typename IDENTIFIER '=' expression		
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $2);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptDeclaration(gLine, gColumn, $1, id, $4);
		gAllocationManager->addAllocation($$);
	}
	;

forexpressionlist
	: /* empty */								
	{  
		$$ = NULL;
	}
	| nextforexpressionlist						
	{
		$$ = $1;
	}
	;

nextforexpressionlist
	: expression								
	{  
		$$ = new LLScriptForExpressionList(gLine, gColumn, $1, NULL);
		gAllocationManager->addAllocation($$);
	}
	| expression ',' nextforexpressionlist		
	{
		$$ = new LLScriptForExpressionList(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	;

funcexpressionlist
	: /* empty */								
	{  
		$$ = NULL;
	}
	| nextfuncexpressionlist						
	{
		$$ = $1;
	}
	;

nextfuncexpressionlist
	: expression								
	{  
		$$ = new LLScriptFuncExpressionList(gLine, gColumn, $1, NULL);
		gAllocationManager->addAllocation($$);
	}
	| expression ',' nextfuncexpressionlist		
	{
		$$ = new LLScriptFuncExpressionList(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	;

listexpressionlist
	: /* empty */								
	{  
		$$ = NULL;
	}
	| nextlistexpressionlist						
	{
		$$ = $1;
	}
	;

nextlistexpressionlist
	: expression								
	{  
		$$ = new LLScriptListExpressionList(gLine, gColumn, $1, NULL);
		gAllocationManager->addAllocation($$);
	}
	| expression ',' nextlistexpressionlist		
	{
		$$ = new LLScriptListExpressionList(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	;

expression
	: unaryexpression							
	{  
		$$ = $1;
	}
	| lvalue '=' expression						
	{  
		$$ = new LLScriptAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| lvalue ADD_ASSIGN expression				
	{  
		$$ = new LLScriptAddAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| lvalue SUB_ASSIGN expression				
	{  
		$$ = new LLScriptSubAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| lvalue MUL_ASSIGN expression				
	{  
		$$ = new LLScriptMulAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| lvalue DIV_ASSIGN expression				
	{  
		$$ = new LLScriptDivAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| lvalue MOD_ASSIGN expression				
	{  
		$$ = new LLScriptModAssignment(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression EQ expression					
	{  
		$$ = new LLScriptEquality(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression NEQ expression					
	{  
		$$ = new LLScriptNotEquals(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression LEQ expression					
	{  
		$$ = new LLScriptLessEquals(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression GEQ expression					
	{  
		$$ = new LLScriptGreaterEquals(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '<' expression					
	{  
		$$ = new LLScriptLessThan(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '>' expression					
	{  
		$$ = new LLScriptGreaterThan(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '+' expression					
	{  
		$$ = new LLScriptPlus(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '-' expression					
	{  
		$$ = new LLScriptMinus(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '*' expression					
	{  
		$$ = new LLScriptTimes(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '/' expression					
	{  
		$$ = new LLScriptDivide(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '%' expression					
	{  
		$$ = new LLScriptMod(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '&' expression					
	{  
		$$ = new LLScriptBitAnd(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '|' expression					
	{  
		$$ = new LLScriptBitOr(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression '^' expression					
	{  
		$$ = new LLScriptBitXor(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression BOOLEAN_AND expression			
	{  
		$$ = new LLScriptBooleanAnd(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression BOOLEAN_OR expression			
	{  
		$$ = new LLScriptBooleanOr(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression SHIFT_LEFT expression
	{
		$$ = new LLScriptShiftLeft(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	| expression SHIFT_RIGHT expression
	{
		$$ = new LLScriptShiftRight(gLine, gColumn, $1, $3);
		gAllocationManager->addAllocation($$);
	}
	;

unaryexpression
	: '-' expression						
	{  
		$$ = new LLScriptUnaryMinus(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| '!' expression							
	{  
		$$ = new LLScriptBooleanNot(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| '~' expression							
	{  
		$$ = new LLScriptBitNot(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| INC_OP lvalue							
	{  
		$$ = new LLScriptPreIncrement(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| DEC_OP lvalue							
	{  
		$$ = new LLScriptPreDecrement(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	| typecast				
	{
		$$ = $1;
	}
	| unarypostfixexpression
	{  
		$$ = $1;
	}
	| '(' expression ')'						
	{  
		$$ = new LLScriptParenthesis(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	;

typecast
	: '(' typename ')' lvalue				
	{
		$$ = new LLScriptTypeCast(gLine, gColumn, $2, $4);
		gAllocationManager->addAllocation($$);
	}
	| '(' typename ')' constant				
	{
		LLScriptConstantExpression *temp =  new LLScriptConstantExpression(gLine, gColumn, $4);
		gAllocationManager->addAllocation(temp);
		$$ = new LLScriptTypeCast(gLine, gColumn, $2, temp);
		gAllocationManager->addAllocation($$);
	}
	| '(' typename ')' unarypostfixexpression				
	{
		$$ = new LLScriptTypeCast(gLine, gColumn, $2, $4);
		gAllocationManager->addAllocation($$);
	}
	| '(' typename ')' '(' expression ')'				
	{
		$$ = new LLScriptTypeCast(gLine, gColumn, $2, $5);
		gAllocationManager->addAllocation($$);
	}
	;

unarypostfixexpression
	: vector_initializer 					
	{  
		$$ = $1;
	}
	| quaternion_initializer					
	{
		$$ = $1;
	}
	| list_initializer							
	{  
		$$ = $1;
	}
	| lvalue									
	{  
		$$ = $1;
	}
	| lvalue INC_OP							
	{  
		$$ = new LLScriptPostIncrement(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| lvalue DEC_OP							
	{  
		$$ = new LLScriptPostDecrement(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	| IDENTIFIER '(' funcexpressionlist ')'			
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptFunctionCall(gLine, gColumn, id, $3);
		gAllocationManager->addAllocation($$);
	}
	| PRINT '(' expression ')'			
	{  
		$$ = new LLScriptPrint(gLine, gColumn, $3);
		gAllocationManager->addAllocation($$);
	}
	| constant									
	{  
		$$ = new LLScriptConstantExpression(gLine, gColumn, $1);
		gAllocationManager->addAllocation($$);
	}
	;

vector_initializer
	: '<' expression ',' expression ',' expression '>'	%prec INITIALIZER
	{
		$$ = new LLScriptVectorInitializer(gLine, gColumn, $2, $4, $6);
		gAllocationManager->addAllocation($$);
	}
	| ZERO_VECTOR
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptConstantExpression *sa0 = new LLScriptConstantExpression(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptConstantExpression *sa1 = new LLScriptConstantExpression(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptConstantExpression *sa2 = new LLScriptConstantExpression(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptVectorInitializer(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	| TOUCH_INVALID_VECTOR
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptConstantExpression *sa0 = new LLScriptConstantExpression(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptConstantExpression *sa1 = new LLScriptConstantExpression(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptConstantExpression *sa2 = new LLScriptConstantExpression(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptVectorInitializer(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	| TOUCH_INVALID_TEXCOORD
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, -1.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptConstantExpression *sa0 = new LLScriptConstantExpression(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, -1.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptConstantExpression *sa1 = new LLScriptConstantExpression(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptConstantExpression *sa2 = new LLScriptConstantExpression(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		$$ = new LLScriptVectorInitializer(gLine, gColumn, sa0, sa1, sa2);
		gAllocationManager->addAllocation($$);
	}
	;

quaternion_initializer
	: '<' expression ',' expression ',' expression ',' expression '>' %prec INITIALIZER
	{
		$$ = new LLScriptQuaternionInitializer(gLine, gColumn, $2, $4, $6, $8);
		gAllocationManager->addAllocation($$);
	}
	| ZERO_ROTATION
	{
		LLScriptConstantFloat *cf0 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf0);
		LLScriptConstantExpression *sa0 = new LLScriptConstantExpression(gLine, gColumn, cf0);
		gAllocationManager->addAllocation(sa0);
		LLScriptConstantFloat *cf1 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf1);
		LLScriptConstantExpression *sa1 = new LLScriptConstantExpression(gLine, gColumn, cf1);
		gAllocationManager->addAllocation(sa1);
		LLScriptConstantFloat *cf2 = new LLScriptConstantFloat(gLine, gColumn, 0.f);
		gAllocationManager->addAllocation(cf2);
		LLScriptConstantExpression *sa2 = new LLScriptConstantExpression(gLine, gColumn, cf2);
		gAllocationManager->addAllocation(sa2);
		LLScriptConstantFloat *cf3 = new LLScriptConstantFloat(gLine, gColumn, 1.f);
		gAllocationManager->addAllocation(cf3);
		LLScriptConstantExpression *sa3 = new LLScriptConstantExpression(gLine, gColumn, cf3);
		gAllocationManager->addAllocation(sa3);
		$$ = new LLScriptQuaternionInitializer(gLine, gColumn, sa0, sa1, sa2, sa3);
		gAllocationManager->addAllocation($$);
	}
	;

list_initializer
	: '[' listexpressionlist ']' %prec INITIALIZER
	{  
		$$ = new LLScriptListInitializer(gLine, gColumn, $2);
		gAllocationManager->addAllocation($$);
	}
	;

lvalue 
	: IDENTIFIER								
	{  
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptLValue(gLine, gColumn, id, NULL);
		gAllocationManager->addAllocation($$);
	}
	| IDENTIFIER PERIOD IDENTIFIER
	{
		LLScriptIdentifier	*id = new LLScriptIdentifier(gLine, gColumn, $1);	
		gAllocationManager->addAllocation(id);
		LLScriptIdentifier	*ac = new LLScriptIdentifier(gLine, gColumn, $3);	
		gAllocationManager->addAllocation(id);
		$$ = new LLScriptLValue(gLine, gColumn, id, ac);
		gAllocationManager->addAllocation($$);
	}
	;
		
%%
