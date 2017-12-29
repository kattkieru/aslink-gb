/** Scanner module --
    Implementation of module providing all services for tokenizing
    character streams in the generic SDCC linker.

    Original version by Thomas Tensi, 2006-08
*/


#include "scanner.h"

/*========================================*/

#include <ctype.h>
# define CType_isLower islower
# define CType_toUpper toupper
#include "error.h"
#include "string.h"

/*========================================*/

#define Scanner__lastChar (255)

typedef enum {
  Scanner__CharacterKind_whiteSpace, Scanner__CharacterKind_digit,
  Scanner__CharacterKind_digitOrLetter, Scanner__CharacterKind_letter,
  Scanner__CharacterKind_operator, Scanner__CharacterKind_newline,
  Scanner__CharacterKind_streamEnd, Scanner__CharacterKind_comment,
  Scanner__CharacterKind_other
} Scanner__CharacterKind;
  /** kinds of characters scanner knows about */

typedef struct {
  String_Type currentLine;
  UINT16 column;
  UINT16 lineLength;
} Scanner__StringInput;
  /* the read state of current string input for scanner */

static Scanner_ReaderProc Scanner__readerProc = NULL;
  /** routine to read next character */

static String_Type Scanner__radixCharacters;
  /** list of characters which may occur after a leading 0 in a number
      specifying the radix */

static Scanner__StringInput Scanner__stringInput;
  /** temporary data used for scanning a token list */

static Scanner__CharacterKind Scanner__characterKind[Scanner__lastChar+1];
  /** mapping from character to CharacterKind */

typedef struct {
  UINT16 effectiveSize;  /* size of stack */
  char data[Scanner_pushbackStackSize];
} Scanner__PushbackStack;
  /** stack for storing characters pushed back */

static Scanner__PushbackStack Scanner__pushbackStack;

static char *Scanner__kindString[Scanner_TokenKind_other + 1];
  /** external representations of the token kinds */

static Boolean Scanner__traceIsOn = false;
  /** tells whether all tokens read are logged */

/*--------------------*/

static void Scanner__destroyTokenObject (inout Object *token);
static Object Scanner__makeTokenObject (void);

static TypeDescriptor_Record Scanner__tokenTypeRecord =
  { /* .objectSize = */ 0, /* .assignmentProc = */ NULL,
    /* .comparisonProc = */ NULL,
    /* .constructionProc = */ Scanner__makeTokenObject,
    /* .destructionProc = */ Scanner__destroyTokenObject,
    /* .hashCodeProc = */ NULL, /* .keyValidationProc = */ NULL
  };

static TypeDescriptor_Type Scanner__tokenTypeDescriptor =
  &Scanner__tokenTypeRecord;


/*========================================*/
/*            INTERNAL ROUTINES           */
/*========================================*/

static void Scanner__ungetChar (in char ch);

/*--------------------*/

static void Scanner__destroyTokenObject (inout Object *token)
{
  Scanner_destroyToken(*token);
  DESTROY(*token);
}

/*--------------------*/

static char Scanner__getChar (void)
  /** gets next character; if pushback stack is empty, reads next
      character from input, otherwise gets next available character in
      pushback stack */
{
  Scanner__PushbackStack *buffer = &Scanner__pushbackStack;
  char ch;

  if (buffer->effectiveSize == 0) {
    /* pushback stack is empty ==> read character from input */
    ch = Scanner__readerProc();
  } else {
    /* get character from pushback stack */
    ch = buffer->data[--buffer->effectiveSize];
  }

  return ch;
}

/*--------------------*/

static void Scanner__getIdentifier (out Scanner_Token *token)
  /** collects all characters of an identifier where the
      next character on the input stream has already been
      verified as an identifier character; result is returned
      in <token> */
{
  token->kind = Scanner_TokenKind_identifier;
  String_clear(&token->representation);

  for (;;) {
    char ch = Scanner__getChar();
    Scanner__CharacterKind kind = Scanner__characterKind[ch];
    if (kind !=  Scanner__CharacterKind_letter
	&& kind != Scanner__CharacterKind_digitOrLetter
        && kind != Scanner__CharacterKind_digit) {
      Scanner__ungetChar(ch);
      break;
    }
    String_appendChar(&token->representation, ch);
  }
}

/*--------------------*/

static char Scanner__getLineCharacter (void)
  /** gets a single character from a string stored in
      <Scanner__stringInput> */
{
  Boolean isDone = false;
  char result = ' ';
  Scanner__StringInput *stringInput = &Scanner__stringInput;

  while (!isDone) {
    if (stringInput->column == 0) {
      stringInput->column = 1;
      stringInput->lineLength = String_length(stringInput->currentLine);
    }

    if (stringInput->column <= stringInput->lineLength) {
      result = String_getAt(stringInput->currentLine,
			    stringInput->column);
      stringInput->column++;
      isDone = true;
    } else {
      result = Scanner_endOfStreamChar;
      isDone = true;
    }
  }

  return result;
}

/*--------------------*/

static void Scanner__getNumber (out Scanner_Token *token)
  /** collects all characters of a number where the next character on
      the input stream has already been verified as a number
      character */
{
  char ch = Scanner__getChar();

  token->kind = Scanner_TokenKind_number;
  String_clear(&token->representation);
  String_appendChar(&token->representation, ch);

  if (ch == '0') {
    /* include following radix specification character */
    SizeType position;
    ch = Scanner__getChar();
    position = String_findCharacter(Scanner__radixCharacters, ch);

    if (position == String_notFound) {
      Scanner__ungetChar(ch);
    } else {
      if (CType_isLower(ch)) {
	ch = (char) CType_toUpper(ch);
      }

      String_appendChar(&token->representation, ch);
    }
  }

  for (;;) {
    Scanner__CharacterKind kind;
    ch = Scanner__getChar();
    kind = Scanner__characterKind[ch];

    if (kind != Scanner__CharacterKind_digit
	&& kind != Scanner__CharacterKind_digitOrLetter) {
      Scanner__ungetChar(ch);
      break;
    }

    String_appendChar(&token->representation, ch);
  }
}

/*--------------------*/

static void Scanner__getAmbiguousToken (out Scanner_Token *token)
  /** collects all characters of an identifier or number token where
      the next character on the input stream has not unambigously been
      verified as either an identifier character or a number
      character; result is returned in <token> */
{
  token->kind = Scanner_TokenKind_idOrNumber;

  String_clear(&token->representation);

  for (;;) {
    char ch = Scanner__getChar();
    Scanner__CharacterKind kind = Scanner__characterKind[ch];
    
    if (kind == Scanner__CharacterKind_letter) {
      token->kind = Scanner_TokenKind_identifier;
    } else if (kind != Scanner__CharacterKind_digit
	       && kind != Scanner__CharacterKind_digitOrLetter) {
      Scanner__ungetChar(ch);
      break;
    }

    String_appendChar(&token->representation, ch);
  }
}

/*--------------------*/

static void Scanner__getOperator (out Scanner_Token *token)
  /** collects all characters of an operator (typically exactly one)
      where the next character on the input stream has already been
      verified as an operator character */
{
  char ch = Scanner__getChar();
  Boolean isBadToken = false;

  String_clear(&token->representation);
  String_appendChar(&token->representation, ch);

  if (ch == '<' || ch == '>') {
    /* there are no relational operators in scanned language; hence
       those must be shifts */
    char nextChar = Scanner__getChar();

    if (nextChar == ch) {
      String_appendChar(&token->representation, nextChar);
    } else {
      Scanner__ungetChar(nextChar);
      isBadToken = true;
    }
  }

  if (isBadToken) {
    token->kind = Scanner_TokenKind_other;
  } else {
    Scanner_Operator operator;

    /* find out which one ...*/
    switch (ch) {
      case '=':  operator = Scanner_Operator_assignment;  break;
      case '+':  operator = Scanner_Operator_plus;  break;
      case '-':  operator = Scanner_Operator_minus;  break;
      case '*':  operator = Scanner_Operator_times;  break;
      case '/':  operator = Scanner_Operator_div;  break;
      case '%':  operator = Scanner_Operator_mod;  break;
      case '<':  operator = Scanner_Operator_shiftLeft;  break;
      case '>':  operator = Scanner_Operator_shiftRight;  break;
      case '|':  operator = Scanner_Operator_or;  break;
      case '&':  operator = Scanner_Operator_and;  break;
      case '^':  operator = Scanner_Operator_complement;  break;
      default:
	operator = Scanner_Operator_other;
	Error_raise(Error_Criticality_warning, "unknown operator used");
    }

    token->kind = Scanner_TokenKind_operator;
    token->operator = operator;
  }
}

/*--------------------*/

static void Scanner__makeTokenForChar (out Scanner_Token *token, in char ch,
				       in Scanner_TokenKind kind)
  /** makes a token from a single character <ch> by putting this
      character into token representation and setting token kind to
      <kind>; does not alter other fields of <token> */
{
  String_clear(&token->representation);
  String_appendChar(&token->representation, ch);
  token->kind = kind;
}

/*--------------------*/

static Object Scanner__makeTokenObject (void)
{
  Scanner_Token *token = NEW(Scanner_Token);
  Scanner_makeToken(token);
  return token;
}

/*--------------------*/

static void Scanner__setKindForCharacters (in char *charList,
					   in Scanner__CharacterKind kind)
  /** sets character kind for all characters in <charList> to
      <kind> */
{
  char *listPtr = charList;

  for (;;) {
    char ch = *listPtr++;

    if (ch == String_terminator) {
      break;
    }

    Scanner__characterKind[ch] = kind;
  }
}

/*--------------------*/

static void Scanner__ungetChar (in char ch)
  /** pushes back one character <ch> into pushback stack */
{
  Scanner__PushbackStack *buffer = &Scanner__pushbackStack;

  if (buffer->effectiveSize == Scanner_pushbackStackSize) {
    /* pushback stack is full ==> impossible to push back */
    Error_raise(Error_Criticality_fatalError,
		"scanner pushback stack is full");
  } else {
    /* put character to pushback stack */
    buffer->data[buffer->effectiveSize++] = ch;
  }
}

/*========================================*/
/*           EXPORTED ROUTINES            */
/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Scanner_initialize (void)
{
  UINT16 ch;

  Scanner__radixCharacters = String_makeFromCharArray("bB@oOqQdDxXhH");
  Scanner__stringInput.currentLine = String_make();

  /* define the categories of the characters*/
  for (ch = 0;  ch <= Scanner__lastChar;  ch++) {
    Scanner__characterKind[ch] = Scanner__CharacterKind_other;
  }

  Scanner__characterKind['\n'] = Scanner__CharacterKind_newline;
  Scanner__characterKind[';'] = Scanner__CharacterKind_comment;
  Scanner__characterKind[Scanner_endOfStreamChar] = 
    Scanner__CharacterKind_streamEnd;

  Scanner__setKindForCharacters(" \t\f", Scanner__CharacterKind_whiteSpace);
  Scanner__setKindForCharacters("0123456789", Scanner__CharacterKind_digit);
  Scanner__setKindForCharacters("ABCDEFabcdef",
				Scanner__CharacterKind_digitOrLetter);
  Scanner__setKindForCharacters("GHIJKLMNOPQRSTUVWXYZ"
				"ghijklmnopqrstuvwxyz_.$",
				Scanner__CharacterKind_letter);
  Scanner__setKindForCharacters("=+-*/%<>|&^", 
				Scanner__CharacterKind_operator);

  /* the external string representation of token kind */
  Scanner__kindString[Scanner_TokenKind_operator]   = "operator";
  Scanner__kindString[Scanner_TokenKind_identifier] = "identifier";
  Scanner__kindString[Scanner_TokenKind_number]     = "number";
  Scanner__kindString[Scanner_TokenKind_newline]    = "newline";
  Scanner__kindString[Scanner_TokenKind_streamEnd]  = "stream end";
  Scanner__kindString[Scanner_TokenKind_comment]    = "comment";
  Scanner__kindString[Scanner_TokenKind_other]      = "other";
}

/*--------------------*/

void Scanner_finalize (void)
{
  String_destroy(&Scanner__radixCharacters);
  String_destroy(&Scanner__stringInput.currentLine);
}


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void Scanner_makeToken (out Scanner_Token *token)
{
  token->representation = String_make();
}

/*--------------------*/

void Scanner_makeTokenList (out Scanner_TokenList *tokenList,
			    in String_Type st)
{
  Scanner_Token *token;

  *tokenList = List_make(Scanner__tokenTypeDescriptor);
  String_copy(&Scanner__stringInput.currentLine, st);
  Scanner__stringInput.column = 0;
  Scanner_redirectInput(&Scanner__getLineCharacter);

  do {
    Object *objectPtr = List_append(tokenList);
    token = *objectPtr;
    Scanner_getNextToken(token);
  } while (token->kind != Scanner_TokenKind_streamEnd);
}


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Scanner_destroyToken (inout Scanner_Token *token)
{
  String_destroy(&token->representation);
}


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Scanner_getNextToken (out Scanner_Token *token)
{
  Boolean isInWhiteSpace;

  do {
    unsigned char ch = Scanner__getChar();
    isInWhiteSpace = false;

    switch (Scanner__characterKind[ch]) {
      case Scanner__CharacterKind_whiteSpace:
        isInWhiteSpace = true;
        break;

      case Scanner__CharacterKind_digit:
        Scanner__ungetChar(ch);
        Scanner__getNumber(token);
        break;

      case Scanner__CharacterKind_letter:
        Scanner__ungetChar(ch);
        Scanner__getIdentifier(token);
        break;

      case Scanner__CharacterKind_digitOrLetter:
        Scanner__ungetChar(ch);
        Scanner__getAmbiguousToken(token);
        break;

      case Scanner__CharacterKind_operator:
        Scanner__ungetChar(ch);
        Scanner__getOperator(token);
        break;

      case Scanner__CharacterKind_newline:
	Scanner__makeTokenForChar(token, ch, Scanner_TokenKind_newline);
	break;

      case Scanner__CharacterKind_streamEnd:
	Scanner__makeTokenForChar(token, ch, Scanner_TokenKind_streamEnd);
	break;

      case Scanner__CharacterKind_comment:
	Scanner__makeTokenForChar(token, ch, Scanner_TokenKind_comment);
	break;

      case Scanner__CharacterKind_other:
	Scanner__makeTokenForChar(token, ch, Scanner_TokenKind_other);
	break;

    }
  }  while (isInWhiteSpace);

  if (Scanner__traceIsOn) {
    if (token->kind != Scanner_TokenKind_newline) {
      File_writeChar(&File_stderr, ' ');
    }

    if (token->kind == Scanner_TokenKind_streamEnd) {
      File_writeCharArray(&File_stderr, "<EOF>");
    } else {
      File_writeString(&File_stderr, token->representation);
    }
  }
}

/*--------------------*/

void Scanner_ungetToken (in Scanner_Token token)
{
  SizeType i;

  for (i = String_length(token.representation);  i != 0;  i--) {
    char ch = String_getCharacter(token.representation, i);
    Scanner__ungetChar(ch);
  }
}


/*--------------------*/
/* CONFIGURATION      */
/*--------------------*/

void Scanner_redirectInput (in Scanner_ReaderProc readerProc)
{
  Scanner__readerProc = readerProc;
  Scanner__pushbackStack.effectiveSize = 0;
}


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Scanner_tokenToString (in Scanner_Token token, out String_Type *st)
{
  String_copyCharArray(st, "[kind = ");
  String_appendCharArray(st, Scanner__kindString[token.kind]);
  String_appendCharArray(st, ", repr = '");
  String_append(st, token.representation);
  String_appendCharArray(st, "']");
}
