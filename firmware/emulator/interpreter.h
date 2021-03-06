//*****************************************************************************
// Filename : 'interpreter.h'
// Title    : Common defines for mchron interpreter
//*****************************************************************************

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdio.h>

// The program counter control block execution logic types
#define PC_CONTINUE		0
#define PC_REPEAT_FOR		1
#define PC_REPEAT_NEXT		2
#define PC_IF_THEN		3
#define PC_IF_ELSE_IF		4
#define PC_IF_ELSE		5
#define PC_IF_END		6

// The command argument profile types
#define ARG_CHAR		0
#define ARG_WORD		1
#define ARG_UNUM		2
#define ARG_NUM			3
#define ARG_STRING		4
#define ARG_STR_OPT		5
#define ARG_ASSIGN		6
#define ARG_END			7

// The argument value domain validation types
#define DOM_NULL_INFO		0
#define DOM_CHAR_LIST		1
#define DOM_WORD_LIST		2
#define DOM_NUM_RANGE		3
#define DOM_NUM_MAX		4
#define DOM_NUM_MIN		5
#define DOM_VAR_NAME		6

// The command input read methods
#define CMD_INPUT_READLINELIB	0
#define CMD_INPUT_MANUAL	1

// The command echo options
#define CMD_ECHO_NO		0
#define CMD_ECHO_YES		1
#define CMD_ECHO_INHERIT	2

// The command return values
#define CMD_RET_OK		0
#define CMD_RET_EXIT		1
#define CMD_RET_ERROR		-1
#define CMD_RET_RECOVER		-2
#define CMD_RET_INTERRUPT	-3

// To prevent recursive file loading define a max file depth when executing
// command files
#define CMD_FILE_DEPTH_MAX	8

// The max number of mchron command line arguments per argument type
#define ARG_TYPE_COUNT_MAX	10

// Generic stack trace header
#define CMD_STACK_TRACE		"--- stack trace ---\n"

// Definition of a structure holding a single command line, originating from
// the command line prompt or from a command file
typedef struct _cmdLine_t
{
  int lineNum;				// Line number
  char *input;				// The malloc-ed command from file/prompt
  struct _cmdCommand_t *cmdCommand;	// The associated command dictionary entry
  struct _cmdPcCtrl_t *cmdPcCtrlParent;	// Control block completed by this line
  struct _cmdPcCtrl_t *cmdPcCtrlChild;	// Control block started by this line
  struct _cmdLine_t *next;		// Pointer to next list element
} cmdLine_t;

// Definition of a structure holding a program counter control block for
// if-then-else and repeat commands
typedef struct _cmdPcCtrl_t
{
  int cmdPcCtrlType;			// The program counter control block type
  int initialized;			// Are control block arguments initialized
  int active;				// Is current block the active code block	
  char *cbArg1;				// Malloc-ed control block argument 1
  char *cbArg2;				// Malloc-ed control block argument 2
  char *cbArg3;				// Malloc-ed control block argument 3
  cmdLine_t *cmdLineParent;		// Pointer to associated parent command
  cmdLine_t *cmdLineChild;		// Pointer to associated child command
  struct _cmdPcCtrl_t *prev;		// Pointer to previous list element
  struct _cmdPcCtrl_t *next;		// Pointer to next list element
} cmdPcCtrl_t;

// Definition of a structure holding the parameters for reading input
// lines from an input stream, being either command line or file
typedef struct _cmdInput_t
{
  FILE *file;				// Input stream (stdin or file)
  char *input;				// Pointer to resulting single input line
  int readMethod;			// Input read method (readline or manual)
} cmdInput_t;

// Definition of a structure holding domain info for a command argument
typedef struct _cmdArgDomain_t
{
  int argDomainType;			// Argument domain type
  char *argTextList;			// Char/word argument value list 
  double argNumMin;			// Numeric argument min value
  double argNumMax;			// Numeric argument max value
  char *argDomainInfo;			// Additional domain info
} cmdArgDomain_t;

// Definition of a structure holding a command line argument
typedef struct _cmdArg_t
{
  int argType;				// Argument type
  char *argName;			// Argument name
  cmdArgDomain_t *cmdArgDomain;		// Argument domain
} cmdArg_t;

// Definition of a structure holding a single mchron command with argument
// profiles and command handler function
typedef struct _cmdCommand_t
{
  char *cmdName;			// The mchron command name
  int cmdPcCtrlType;			// Program counter control block type
  cmdArg_t *cmdArg;			// Array of command argument profiles
  int argCount;				// Profile argument count
  int (*cmdHandler)(cmdLine_t *);	// Handler for regular commands
  int (*cbHandler)(cmdLine_t **);	// Handler for control block commands
  char *cmdHandlerName;			// Execution handler name
  char *cmdNameDescr;			// Command name description
} cmdCommand_t;

// Definition of a structure holding a command group containing all mchron
// commands for the group.
// In array form this structure will create the mchron command dictionary.
typedef struct _cmdDict_t
{
  char cmdGroup;			// The mchron command group identifier
  char *cmdGroupDescr;			// The command group description
  cmdCommand_t *cmdCommand;		// Array of mchron commands in group
  int commandCount;			// Command group size
} cmdDict_t;
#endif

