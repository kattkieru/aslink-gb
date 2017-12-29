/** Parser module --
    Implementation of module providing all services for parsing
    tokenized character streams in the generic SDCC linker.

    Original version by Thomas Tensi, 2007-10
*/

#include "parser.h"

/*========================================*/

#include "area.h"
#include "banking.h"
#include "codeoutput.h"
#include "codesequence.h"
#include "error.h"
#include "file.h"
#include "globdefs.h"
#include "mapfile.h"
#include "module.h"
#include "scanner.h"
#include "set.h"
#include "string.h"
#include "stringlist.h"
#include "target.h"

/*========================================*/

static struct {
  StringList_Type nameList; /* name of all files */
  UINT16 index;
  UINT16 count;  /* length of nameList */
  File_Type currentFile;  /* currently open file */
  String_Type currentFileName; /* name of current open file */
  UINT32 currentLineIndex;
  String_Type currentLine;
  UINT16 column;
  UINT16 lineLength;
} Parser__fileSequence;
  /** the read state of current file set input for scanner */


static Parser_Options Parser__options;
  /** options for reading current input files */


static Parser_Options Parser__defaultOptions;
  /** default options for reading the input files */


typedef struct { 
  String_Type moduleName;
  String_Type line;
} Parser__CompilerOptions;


static Parser__CompilerOptions Parser__compilerOptions;
  /** relevant compiler options found in first object file */


/* several predefined sets of token kinds */

static Set_Type Parser__TokenKindSet_identifier;
  /* set of token kinds consisting of kind "identifier" or
     "idOrNumber"*/

static Set_Type Parser__TokenKindSet_newline;
  /* set of token kinds consisting of kind "newline" */

static Set_Type Parser__TokenKindSet_numberSequence;
  /* set of token kinds consisting of "newline", "number" or
     "idOrNumber" */

static Set_Type Parser__TokenKindSet_number;
  /* set of token kinds consisting of kind "number" or "idOrNumber" */

static Set_Type Parser__TokenKindSet_textSequence;
  /* set of token kinds consisting of arbitrary text */


/* several predefined scan states */

typedef short Parser__State;

#define Parser__State_inError    0
#define Parser__State_done       1
#define Parser__State_atNewline  2
#define Parser__State_firstState 3


typedef void (*Parser__StateTransitionProc)(in Boolean isFirstPass,
				    in Scanner_Token token,
				    inout Parser__State *state,
				    out Set_Type *expectedNextTokenKinds);
  /** callback routine for finite state automaton routine
      <executeFiniteStateAutomaton> processing single input line;
      <isFirstPass> tells whether processing takes place in the first
      pass of the parsing, <token> is the last token read, <state> the
      current state of line processing and <expectedNextTokenKinds> is
      a set of token kinds to come after current token */


static CodeSequence_Type Parser__codeSequence;
  /** previous code sequence read by parser (will be cleared when
      subsequent relocation command is processed) */



/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static INT32 Parser__evaluateNumber (in String_Type st);
static Target_Address Parser__makeWord (in UINT8 partA, in UINT8 partB);
static void Parser__markError (void);
static void Parser__skipToNewline (out Scanner_Token *token);

/*--------------------*/

static void Parser__doAreaStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for area definition command;
      conforms to type <StateTransitionProc> above */
{
  static Area_AttributeSet areaAttributeSet;
  static Target_Address areaTotalSize;
  static String_Type areaName;
  String_Type representation = String_make();

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atAreaName, State_atSizeLabel, State_atSize,
	 State_atFlagsLabel, State_atFlags } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      areaName = String_make();
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atAreaName:
      String_copy(&areaName, token.representation);
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atSizeLabel:
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atSize:
      if (isFirstPass) {
	areaTotalSize = Parser__evaluateNumber(representation);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atFlagsLabel:
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atFlags:
      if (isFirstPass) {
	UINT8 attributeSetEncoding =
	  (UINT8)Parser__evaluateNumber(representation);
	areaAttributeSet = Area_makeAttributeSet(attributeSetEncoding);
      }

      parserState = State_atNewline;
      *expectedNextTokenKinds = Parser__TokenKindSet_newline;
      break;

    case State_atNewline:
      if (isFirstPass) {
	Area_makeSegment(areaName,
			 areaTotalSize, areaAttributeSet);
      } else {
	Area_Segment currentSegment;
	Module_Type module = Module_currentModule();

	/* adapt banked area if necesssary */
	Banking_adaptAreaNameWhenBanked(module, &areaName);
	currentSegment = Module_getSegmentByName(module, areaName);
	Area_setCurrent(currentSegment);
      }
    
      parserState = State_done;
      String_destroy(&areaName);
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doCodeLineStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for code line definition;
      conforms to type <StateTransitionProc> above */
{
  static UINT8 addressPartA;
  static UINT8 addressPartB;
  String_Type representation = String_make();

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atAddressPartA, State_atAddressPartB,
	 State_atByteSequence } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      if (!isFirstPass) {
	Parser__codeSequence.length = 0;
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atAddressPartA:
      if (!isFirstPass) {
	addressPartA = (UINT8) Parser__evaluateNumber(representation);
      }

      parserState++;
      break;

    case State_atAddressPartB:
      if (!isFirstPass) {
	addressPartB = (UINT8) Parser__evaluateNumber(representation);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_numberSequence;
      break;

    case State_atByteSequence:
      if (token.kind == Scanner_TokenKind_newline) {
	if (!isFirstPass) {
	  Target_Address startAddress;
	  Parser__codeSequence.segment = Area_currentSegment();
	  startAddress = Parser__makeWord(addressPartA, addressPartB);
	  Parser__codeSequence.offsetAddress = startAddress;
	  /* the code sequence is stored for the subsequent relocation
	     line */
	}
    
	parserState = State_done;
      } else if (!isFirstPass) {
	UINT8 length = Parser__codeSequence.length;
	UINT8 currentByte = (UINT8) Parser__evaluateNumber(representation);

	if (length == CodeSequence_maxLength) {
	  Error_raise(Error_Criticality_warning,
		      "line too long; remainder skipped");
	  parserState = State_inError;
	} else {
	  Parser__codeSequence.byteList[length] = currentByte;
	  Parser__codeSequence.length++;
	}
      }

      break;
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doCompilerOptionsStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for compiler options command;
      complains when several option lines are not completely
      identical; conforms to type <StateTransitionProc> above */
{
  static String_Type optionLine;

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atToken } parserState;

  parserState = *state;

  switch (parserState) {
    case State_firstState:
      optionLine = String_make();
      *expectedNextTokenKinds = Parser__TokenKindSet_textSequence;
      parserState = State_atToken;
      break;

    case State_atToken:
      if (token.kind != Scanner_TokenKind_newline) {
	if (isFirstPass) {
	  String_appendChar(&optionLine, ' ');
	  String_append(&optionLine, token.representation);
	}
      } else {
	if (isFirstPass) {
	  String_Type moduleName = String_make();
	  Parser__CompilerOptions *options = &Parser__compilerOptions;

	  Module_getName(Module_currentModule(), &moduleName);

	  if (String_length(options->line) == 0) {
	    /* this is the first option line ever encountered
	       ==> store it */
	    String_copy(&options->line, optionLine);
	    String_copy(&options->moduleName, moduleName);
	  } else {
	    /* check whether options here are identical to previous */
	    if (!String_isEqual(options->line, optionLine)) {
	      Error_raise(Error_Criticality_warning,
			  "conflicting compiler options:\n"
			  "   \"%s\" in module \"%s\" and\n"
			  "   \"%s\" in module \"%s\".",
			  String_asCharPointer(options->line),
			  String_asCharPointer(options->moduleName),
			  String_asCharPointer(optionLine),
			  String_asCharPointer(moduleName));
	      Parser__markError();
	    }
	  }

	  String_destroy(&moduleName);
	}

	parserState = State_done;
	String_destroy(&optionLine);
      }

      break;
  }

  *state = parserState;
}
      
/*--------------------*/

static void Parser__doHeaderStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for header definition
      command; conforms to type <StateTransitionProc> above */
{
  String_Type representation = String_make();
  static Module_SegmentIndex segmentCount;
  static Module_SymbolIndex symbolCount;
  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atAreaCount, State_atAreaID, State_atSymbolCount,
	 State_atSymbolID1, State_atSymbolID2
	 } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      parserState++;

      if (isFirstPass) {
	segmentCount = 0;
	symbolCount = 0;
      } else {
	Boolean isFound;
	Module_setCurrentByFileName(Parser__fileSequence.currentFileName,
				    &isFound);
	if (!isFound) {
	  Error_raise(Error_Criticality_warning,
		      "unknown module for file");
	  Parser__markError();
	  parserState = State_inError;
	}
      }

      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atAreaCount:
      if (isFirstPass) {
	segmentCount = (Module_SegmentIndex)
                       Parser__evaluateNumber(representation);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atAreaID:
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atSymbolCount:
      if (isFirstPass) {
	symbolCount = (Module_SymbolIndex)
                      Parser__evaluateNumber(representation);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atSymbolID1:
      parserState++;
      break;

    case State_atSymbolID2:
      parserState = State_atNewline;
      *expectedNextTokenKinds = Parser__TokenKindSet_newline;
      break;

    case State_atNewline:
      if (isFirstPass) {
	Module_make(Parser__fileSequence.currentFileName,
		    segmentCount, symbolCount);
	/* all subsequent symbol definitions and references go to
	   absolute segment unless a segment definition has occured
	   before */
	Area_makeAbsoluteSegment();
      }

      parserState = State_done;
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doModuleStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for module definition
      command; conforms to type <StateTransitionProc> above */
{
  static String_Type moduleName;
  String_Type representation = String_make();

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atModuleName} parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      moduleName = String_make();
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atModuleName:
      String_copy(&moduleName, representation);
      parserState = State_atNewline;
      *expectedNextTokenKinds = Parser__TokenKindSet_newline;
      break;

    case State_atNewline:
      parserState = State_done;

      if (isFirstPass) {
	Module_setName(moduleName);
      } else {
	Boolean isFound;
	Module_setCurrentByName(moduleName, &isFound);

	if (!isFound) {
	  Error_raise(Error_Criticality_warning, "unknown module %s",
		      String_asCharPointer(moduleName));
	  Parser__markError();
	  parserState = State_inError;
	}
      }

      String_destroy(&moduleName);
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doRadixStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for radix definition command;
      conforms to type <StateTransitionProc> above */
{
  static String_Type command;
  char commandChar;
  String_Type representation = String_make();

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      command = String_make();
      String_copy(&command, representation);
      parserState = State_atNewline;
      *expectedNextTokenKinds = Parser__TokenKindSet_newline;
      break;

    case State_atNewline:
      commandChar = String_getCharacter(command, 1);
      Parser__options.defaultBase = (commandChar == 'X' ? 16
				     : (commandChar == 'D' ? 10 : 8 ));

      if (String_length(command) > 1) {
	/* some additional character tells about the endianness of the
	   input file */
	char endiannessChar = String_getCharacter(command, 2);

	switch (endiannessChar) {
	  case 'H': Parser__options.endianness = bigEndian;    break;
	  case 'L': Parser__options.endianness = littleEndian; break;
	}
      }

      parserState = State_done;
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doRelocLineStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for code line relocation
      command; conforms to type <StateTransitionProc> above */
{
  UINT16 areaIndex;
  static UINT16 areaMode;
  UINT8 currentByte = 0;
  char kindChar;
  static UINT8 previousByte;
  static CodeSequence_Relocation relocation;
  static CodeSequence_RelocationList relocationList;

  String_Type representation = String_make();

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atAreaModePartA, State_atAreaModePartB,
	 State_atAreaIndexPartA, State_atAreaIndexPartB,
	 State_atByteSequenceA, State_atByteSequenceB,
	 State_atByteSequenceC, State_atByteSequenceD } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  if (!isFirstPass && parserState > State_firstState
      && Set_isElement(Parser__TokenKindSet_number, token.kind)) {
    currentByte = (UINT8) Parser__evaluateNumber(representation);
  }

  switch (parserState) {
    case State_firstState:
      kindChar = String_getCharacter(representation, 1);
      relocationList.count = 0;
      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_number;
      break;

    case State_atAreaModePartA:
    case State_atAreaIndexPartA:
    case State_atByteSequenceC:
      previousByte = currentByte;
      parserState++;
      break;

    case State_atAreaModePartB:
      areaMode = Parser__makeWord(previousByte, currentByte);
      parserState++;
      break;

    case State_atAreaIndexPartB:
      areaIndex = Parser__makeWord(previousByte, currentByte);
      areaIndex++;  /* indexing starts at 1 */
      /* advance <areaIndex> by one to account for absolute
       segment inserted at first position */
      areaIndex++;

      if (!isFirstPass) {
	relocationList.segment = Module_getSegment(Module_currentModule(), 
						   areaIndex);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_numberSequence;
      break;

    case State_atByteSequenceA:
      if (token.kind == Scanner_TokenKind_newline) {
	if (!isFirstPass) {
	  /* relocate last code sequence and put it out */
	  CodeSequence_relocate(&Parser__codeSequence, areaMode, 
				&relocationList);
	  CodeOutput_writeLine(Parser__codeSequence);
	}
    
	parserState = State_done;
      } else {
	CodeSequence_makeKindFromInteger(&relocation.kind, currentByte);
	*expectedNextTokenKinds = Parser__TokenKindSet_number;
	parserState++;
      }

      break;

    case State_atByteSequenceB:
      relocation.index = currentByte - 2;  /* adjust offset by two because of
					      missing base address bytes */
      parserState++;
      break;

    case State_atByteSequenceD:
      if (!isFirstPass) {
	relocation.value = Parser__makeWord(previousByte, currentByte);
	relocationList.list[relocationList.count] = relocation;
	relocationList.count++;
      }

      parserState = State_atByteSequenceA;
      *expectedNextTokenKinds = Parser__TokenKindSet_numberSequence;
      break;
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static void Parser__doSymbolStateTransition (
				     in Boolean isFirstPass,
				     in Scanner_Token token,
				     inout Parser__State *state,
				     out Set_Type *expectedNextTokenKinds)
  /** performs a single state transition for area definition command;
      conforms to type <StateTransitionProc> above */
{
  char kindChar;
  String_Type representation = String_make();
  static Boolean isDefinition;
  static Target_Address symbolAddress;
  static String_Type symbolName;

  enum { State_inError = Parser__State_inError,
	 State_done = Parser__State_done,
	 State_atNewline = Parser__State_atNewline,
	 State_firstState = Parser__State_firstState,
	 State_atSymbolName, State_atSymbolLabel } parserState;

  String_copy(&representation, token.representation);
  
  parserState = *state;

  switch (parserState) {
    case State_firstState:
      if (isFirstPass) {
	symbolName = String_make();
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atSymbolName:
      if (isFirstPass) {
	String_copy(&symbolName, representation);
      }

      parserState++;
      *expectedNextTokenKinds = Parser__TokenKindSet_identifier;
      break;

    case State_atSymbolLabel:
      if (!isFirstPass) {
	parserState = State_atNewline;
	*expectedNextTokenKinds = Parser__TokenKindSet_newline;
      } else {
	kindChar = String_getCharacter(representation, 1);

	if (kindChar != 'D' && kindChar != 'R'
	    || String_length(representation) < 4) {
	  Error_raise(Error_Criticality_warning, "bad symbol flags");
	  Parser__markError();
	  parserState = State_inError;
	} else {
	  String_Type addressString = String_make();
	  isDefinition = (kindChar == 'D');
	  String_getSubstring(&addressString, representation, 4, 10);
	  symbolAddress = Parser__evaluateNumber(addressString);
	  parserState = State_atNewline;
	  *expectedNextTokenKinds = Parser__TokenKindSet_newline;
	  String_destroy(&addressString);
	}
      }

      break; 

    case State_atNewline:
      if (isFirstPass) {
	Symbol_make(symbolName, isDefinition, symbolAddress);
	String_destroy(&symbolName);
      }
    
      parserState = State_done;
  }

  String_destroy(&representation);
  *state = parserState;
}

/*--------------------*/

static Boolean Parser__ensureKind (in Scanner_Token token,
				   in Set_Type allowedKindSet)
  /** checks whether <token.kind> is in <allowedKindSet>; otherwise
      an error message is issued and the error position is marked */
{
  Boolean isOkay = Set_isElement(allowedKindSet, token.kind);

  if (!isOkay) {
    Error_raise(Error_Criticality_warning, "unexpected token kind");
    Parser__markError();
  }

  return isOkay;
}

/*--------------------*/

static INT32 Parser__evaluateNumber (in String_Type st)
  /** returns value of number in <st> for either current base or base
      given by some prefix */
{
  long result;

  if (!String_convertToLong(st, Parser__options.defaultBase, &result)) {
    Error_raise(Error_Criticality_warning, "number expected");
    Parser__markError();
  }

  return result;
}

/*--------------------*/

static void Parser__executeFiniteStateAutomaton (
			in Boolean isFirstPass,
			out Scanner_Token *token,
			in Parser__StateTransitionProc transitionProc)
 /** reads tokens and goes through finite state automaton where
     transition function is given by callback routine
     <transitionProc>; the processing starts with state <firstState>
     and ends when state <done> or <atNewline> is reached*/
{
  Parser__State parserState = Parser__State_firstState;
  Set_Type expectedNextTokenKinds;

  /* do a transition into the correct first state and set up all local
     stuff */
  transitionProc(isFirstPass, *token, &parserState, &expectedNextTokenKinds);

  while (parserState != Parser__State_done) {
    Scanner_getNextToken(token);

    if (parserState != Parser__State_inError) {
      if (!Parser__ensureKind(*token, expectedNextTokenKinds)) {
	parserState = Parser__State_inError;
      }
    }

    if (parserState == Parser__State_inError) {
      Parser__skipToNewline(token);
      parserState = Parser__State_done;
    } else {
      transitionProc(isFirstPass, *token, &parserState,
		     &expectedNextTokenKinds);
    }
  }
}
				   
/*--------------------*/

static char Parser__getFileListCharacter (void)
  /** gets a single character from list of files; automatically
      advances to the next file in the list, when current file is
      exhausted */
{
  Boolean isDone = false;
  char result = ' ';

  if (Parser__fileSequence.index == 0) {
    Parser__fileSequence.lineLength = 0;
  }

  while (!isDone) {
    if (Parser__fileSequence.lineLength != 0) {
      if (Parser__fileSequence.column <= Parser__fileSequence.lineLength) {
	result = String_getAt(Parser__fileSequence.currentLine,
			      Parser__fileSequence.column);
	Parser__fileSequence.column++;
	isDone = true;
      } else {
	/* line is exhausted, get another one */
	File_readLine(&Parser__fileSequence.currentFile,
		      &Parser__fileSequence.currentLine);
	Parser__fileSequence.currentLineIndex++;
	Parser__fileSequence.column = 1;
	Parser__fileSequence.lineLength = 
	  String_length(Parser__fileSequence.currentLine);
      }
    } else {
      /* at end of current file */
      Parser__fileSequence.index++;

      /* close current file */
      if (Parser__fileSequence.index > 1) {
         File_close(&Parser__fileSequence.currentFile);
      }

      if (Parser__fileSequence.index > Parser__fileSequence.count) {
	/* input is exhausted ==> return <Scanner_endOfStreamChar> */
	result = Scanner_endOfStreamChar;
	isDone = true;
      } else {
        Boolean isOpen;
        String_Type fileName;

	/* open next file in list */
	fileName = List_getElement(Parser__fileSequence.nameList,
				   Parser__fileSequence.index);

	isOpen = File_open(&Parser__fileSequence.currentFile, fileName,
			   File_Mode_read);

	if (!isOpen) {
	  Error_raise(Error_Criticality_fatalError,
		      "could not open link file %s", 
		      String_asCharPointer(fileName));
	} else {
	  String_copy(&Parser__fileSequence.currentFileName, fileName);
	  Parser__fileSequence.currentLineIndex = 0;
	  Parser__fileSequence.lineLength = 1;
	  Parser__fileSequence.column = 2;

	  /* reset options to default */
	  Parser__options = Parser__defaultOptions;
	}
      }
    }
  }

  return result;
}

/*--------------------*/

static Target_Address Parser__makeWord (in UINT8 partA, in UINT8 partB)
  /** combines single byte address parts <partA> and <partB> into a
      combined address (depending on the selected endianness); if no
      endianness has been specified,  */
{
  UINT16 result;
  Boolean isBigEndian;

  switch (Parser__options.endianness) {
    case bigEndian:     isBigEndian = true;  break;
    case littleEndian:  isBigEndian = false;  break;
    default:            isBigEndian = Target_info.isBigEndian;
  }

  if (isBigEndian) {
    result = (partA << 8) + partB;
  } else {
    result = (partB << 8) + partA;
  }

  return result;
}

/*--------------------*/

static void Parser__markError (void)
  /** writes information about current input position to
      <File_stderr> */
{
  String_Type firstPart = String_make();
  String_Type secondPart = String_make();
  String_Type leadIn = String_make();
  String_Type *currentLine = &Parser__fileSequence.currentLine;

  /* split current line at column into first and second part */
  String_getSubstring(&firstPart, *currentLine, 1,
		      Parser__fileSequence.column - 1);

  String_getSubstring(&secondPart, *currentLine, Parser__fileSequence.column,
		      String_length(*currentLine));

  /* write out erroneous line with additional properties */
  String_copy(&leadIn, Parser__fileSequence.currentFileName);
  String_appendCharArray(&leadIn, "(");
  String_appendInteger(&leadIn, Parser__fileSequence.currentLineIndex,
			    10);
  String_appendCharArray(&leadIn, "): ");
  String_append(&leadIn, firstPart);

  File_writeString(&File_stderr, leadIn);
  File_writeChar(&File_stderr, '\n');

  String_fillWithCharacter(&leadIn, ' ', String_length(leadIn));
  String_append(&leadIn, secondPart);

  File_writeString(&File_stderr, leadIn);
  File_writeChar(&File_stderr, '\n');

  String_destroy(&leadIn);
  String_destroy(&secondPart);
  String_destroy(&firstPart);
}

/*--------------------*/

Boolean Parser__setMappingFromLine (in String_Type valueMapLine,
			in Parser_KeyValueMappingProc setElementValueProc)
 /** parses the newline terminated line in <valueMapLine> of the form
     "name=value" and calls <setElementValueProc> for given
     (name,value) pair; returns whether line has been parsed
     successfully */
{
  String_Type elementName = String_make();
  long elementValue = 0;
  Boolean lineIsBad = false;
  enum {State_atName, State_atEquals, State_atValue, State_atNewline,
	State_done } state;
  Scanner_TokenList tokenList;
  List_Cursor tokenCursor;

  Scanner_makeTokenList(&tokenList, valueMapLine);
  state = State_atName;

  for (tokenCursor = List_resetCursor(tokenList);
       tokenCursor != NULL;
       List_advanceCursor(&tokenCursor)) {
    Scanner_Token *token = List_getElementAtCursor(tokenCursor);
    Scanner_TokenKind kind = token->kind;
    String_Type representation = token->representation;
   
    switch (state) {
      case State_atName:
	if (kind == Scanner_TokenKind_streamEnd) {
	  state = State_done;
	} else {
	  lineIsBad = (kind != Scanner_TokenKind_identifier 
		       && kind != Scanner_TokenKind_idOrNumber);

	  if (!lineIsBad) {
	    String_copy(&elementName, representation);
	  }

	  state++;
	}

	break;

      case State_atEquals:
	lineIsBad = (lineIsBad
		     || kind != Scanner_TokenKind_operator
		     || token->operator != Scanner_Operator_assignment);
	state++;
	break;

      case State_atValue:
	lineIsBad = (lineIsBad
		     || (kind != Scanner_TokenKind_number 
			 && kind != Scanner_TokenKind_idOrNumber)
		     || !String_convertToLong(representation, 16,
					      &elementValue));
	state++;
	break;

      case State_atNewline:
	if (!lineIsBad) {
	  setElementValueProc(elementName, elementValue);
	}

	state = State_done;
	break;

      case State_done:
	lineIsBad = (lineIsBad || (kind != Scanner_TokenKind_streamEnd));
	break;
    }
  }

  String_destroy(&elementName);
  List_destroy(&tokenList);
  return !lineIsBad;
}

/*--------------------*/

static void Parser__skipToNewline (out Scanner_Token *token)
  /** reads tokens from input until either a newline or an end of
      stream token is read */
{
  do {
    Scanner_getNextToken(token);
  } while (token->kind != Scanner_TokenKind_newline
	   && token->kind != Scanner_TokenKind_streamEnd);
}


/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Parser_initialize (void)
{
  Parser__fileSequence.currentFileName = String_make();
  Parser__fileSequence.currentLine     = String_make();
  Parser__fileSequence.nameList        = StringList_make();

  Parser__compilerOptions.moduleName = String_make();
  Parser__compilerOptions.line       = String_make();

  Parser__TokenKindSet_newline        = Set_make(Scanner_TokenKind_newline);

  Parser__TokenKindSet_identifier     = Set_make(Scanner_TokenKind_identifier);
  Set_include(&Parser__TokenKindSet_identifier, Scanner_TokenKind_idOrNumber);

  Parser__TokenKindSet_number         = Set_make(Scanner_TokenKind_number);
  Set_include(&Parser__TokenKindSet_number, Scanner_TokenKind_idOrNumber);

  Parser__TokenKindSet_numberSequence = Set_make(Scanner_TokenKind_newline);
  Set_include(&Parser__TokenKindSet_numberSequence, Scanner_TokenKind_number);
  Set_include(&Parser__TokenKindSet_numberSequence,
	      Scanner_TokenKind_idOrNumber);

  Parser__TokenKindSet_textSequence   = Set_make(Scanner_TokenKind_operator);
  Set_include(&Parser__TokenKindSet_textSequence, Scanner_TokenKind_number);
  Set_include(&Parser__TokenKindSet_textSequence, Scanner_TokenKind_newline);
  Set_include(&Parser__TokenKindSet_textSequence,
	      Scanner_TokenKind_identifier);
  Set_include(&Parser__TokenKindSet_textSequence,
	      Scanner_TokenKind_idOrNumber);

  Parser__defaultOptions.endianness  = unknown;
  Parser__defaultOptions.defaultBase = 10;
}

/*--------------------*/

void Parser_finalize (void)
{
  String_destroy(&Parser__fileSequence.currentFileName);
  String_destroy(&Parser__fileSequence.currentLine);
  List_destroy(&Parser__fileSequence.nameList);

  String_destroy(&Parser__compilerOptions.moduleName);
  String_destroy(&Parser__compilerOptions.line);

}

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Parser_collectSymbolDefinitions (in String_Type objectFileName,
				      inout StringList_Type *symbolNameList)
{
  Scanner_Token token;
  Boolean isDone = false;

  Scanner_makeToken(&token);

  List_clear(symbolNameList);
  Scanner_redirectInput(&Parser__getFileListCharacter);
  List_clear(&Parser__fileSequence.nameList);
  StringList_append(&Parser__fileSequence.nameList, objectFileName);
  Parser__fileSequence.index = 0;
  Parser__fileSequence.count = List_length(Parser__fileSequence.nameList);

  while (!isDone) {
    Boolean isError = false;

    Scanner_getNextToken(&token);

    if (token.kind == Scanner_TokenKind_streamEnd) {
      isDone = true;
    } else if (!Set_isElement(Parser__TokenKindSet_identifier, token.kind)) {
      isError = true;
    } else if (String_length(token.representation) > 0) {
      char commandCharacter = String_getCharacter(token.representation, 1);

      switch (commandCharacter) {
        case 'X':
        case 'D':
        case 'Q':
        case 'H':
 	case 'M':
 	case 'A':
	case 'R':
	case 'P':
	case 'O':
	  Parser__skipToNewline(&token);
	  break;

	case 'T':
	  Parser__skipToNewline(&token);
	  File_close(&Parser__fileSequence.currentFile);
	  isDone = true;
	  break;

 	case 'S':
	  Scanner_getNextToken(&token);

	  if (!Set_isElement(Parser__TokenKindSet_identifier, token.kind)) {
	    isError = true;
	  } else {
	    String_Type symbolName = String_make();
	    String_copy(&symbolName, token.representation);
	    Scanner_getNextToken(&token);

	    if (!Set_isElement(Parser__TokenKindSet_identifier, token.kind)) {
	      isError = true;
	    } else {
	      char kindChar = String_getCharacter(token.representation, 1);

	      if (kindChar != 'D' && kindChar != 'R'
		  || String_length(token.representation) < 4) {
		isError = true;
	      } else if (kindChar == 'D') {
		StringList_append(symbolNameList, symbolName);
	      }

	      Parser__skipToNewline(&token);
	    }
	  }
	  
	  break;

	default:
	  isError = true;
      }
    }

    if (isError) {
      Error_raise(Error_Criticality_warning, "bad command");
      Parser__markError();
      Parser__skipToNewline(&token);
    }
  }
   
  Scanner_destroyToken(&token);
}

/*--------------------*/

void Parser_setDefaultOptions (in Parser_Options options)
{
  Parser__defaultOptions = options;
}

/*--------------------*/

void Parser_parseObjectFile (in Boolean isFirstPass, in String_Type fileName)
{
  StringList_Type fileNameList = StringList_make();
  StringList_append(&fileNameList, fileName);
  Parser_parseObjectFiles(isFirstPass, fileNameList);
  List_destroy(&fileNameList);
}

/*--------------------*/

void Parser_parseObjectFiles (in Boolean isFirstPass,
			      in StringList_Type fileNameList)
{
  Scanner_Token token;
  Boolean isDone = false;

  Scanner_makeToken(&token);

  Scanner_redirectInput(&Parser__getFileListCharacter);
  List_copy(&Parser__fileSequence.nameList, fileNameList);
  Parser__fileSequence.index = 0;
  Parser__fileSequence.count = List_length(Parser__fileSequence.nameList);

  while (!isDone) {
    Scanner_getNextToken(&token);

    if (token.kind == Scanner_TokenKind_streamEnd) {
      isDone = true;
    } else if (token.kind == Scanner_TokenKind_comment) {
      /* a comment might be interesting for open map files during
	 first pass */
      if (isFirstPass) {
	MapFile_writeSpecialComment(Parser__fileSequence.currentLine);
      }
      Parser__skipToNewline(&token);
    } else if (token.kind != Scanner_TokenKind_identifier
      	       && token.kind != Scanner_TokenKind_idOrNumber) {
      Error_raise(Error_Criticality_warning, "bad command");
      Parser__markError();
      Parser__skipToNewline(&token);
    } else if (String_length(token.representation) > 0) {
      char commandCharacter = String_getCharacter(token.representation, 1);

      switch (commandCharacter) {
        case 'X':
        case 'D':
        case 'Q':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doRadixStateTransition);
	  break;

        case 'H':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doHeaderStateTransition);
	  break;

 	case 'M':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doModuleStateTransition);
	  break;

 	case 'A':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doAreaStateTransition);
	  break;

 	case 'S':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doSymbolStateTransition);
	  break;

	case 'T':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doCodeLineStateTransition);
	  break;

	case 'R':
	case 'P':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
				      &token, 
				      &Parser__doRelocLineStateTransition);
	  break;


	case 'O':
	  Parser__executeFiniteStateAutomaton(isFirstPass,
			      &token, 
			      &Parser__doCompilerOptionsStateTransition);
	  break;

	default:
	  Error_raise(Error_Criticality_warning, "bad command");
	  Parser__markError();
	  Parser__skipToNewline(&token);
      }
    }
  }
   
  Scanner_destroyToken(&token);
}

/*--------------------*/

void Parser_setMappingFromList (in StringList_Type valueMapList,
			in Parser_KeyValueMappingProc setElementValueProc)
{
  List_Cursor stringCursor;

  for (stringCursor = List_resetCursor(valueMapList);
       stringCursor != NULL;
       List_advanceCursor(&stringCursor)) {
    String_Type st = List_getElementAtCursor(stringCursor);
    Boolean isOkay;

    isOkay = Parser__setMappingFromLine(st, setElementValueProc);

    if (!isOkay) {
      Error_raise(Error_Criticality_warning, "bad definition: %s",
		  String_asCharPointer(st));
    }
  }  
}

/*--------------------*/

void Parser_setMappingFromString (in String_Type valueMapString,
			in Parser_KeyValueMappingProc setElementValueProc)
{
  Boolean isOkay;
  isOkay = Parser__setMappingFromLine(valueMapString, setElementValueProc);

  if (!isOkay) {
    Error_raise(Error_Criticality_warning, "bad definition: %s",
		String_asCharPointer(valueMapString));
  }
  
}
