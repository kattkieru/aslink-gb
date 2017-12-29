/** Scanner module --
    This module provides all services for tokenizing character streams
    in the generic SDCC linker.

    Tokenization is done on some input stream specified via a reader
    routine.  A token consists of a kind, an operator (when kind is
    "operator") and some external string representation.

    Tokens may be pushed back when doing a lookahead during parsing.

    Original version by Thomas Tensi, 2006-08
*/


#ifndef __SCANNER_H
#define __SCANNER_H

/*--------------------*/

#include "globdefs.h"
#include "list.h"
#include "string.h"
#include "typedescriptor.h"

/*--------------------*/

#define Scanner_endOfStreamChar (0xFF)
  /** character for telling that the end of the input stream has been
      reached */

#define Scanner_pushbackStackSize (100)
  /** maximum number of tokens pushed back for rereading */

typedef char (*Scanner_ReaderProc)(void);
  /** callback routine for reading next character on some input
      stream; returns <endOfStreamChar> when end of stream is
      reached */

typedef List_Type Scanner_TokenList;
  /** list of tokens */


typedef enum {
  Scanner_TokenKind_operator, Scanner_TokenKind_identifier,
  Scanner_TokenKind_number, Scanner_TokenKind_idOrNumber,
  Scanner_TokenKind_newline, Scanner_TokenKind_streamEnd,
  Scanner_TokenKind_comment, Scanner_TokenKind_other
} Scanner_TokenKind;
  /** kinds of tokens known by the scanner; <idOrNumber> is an
      ambiguous symbol which consists of hexadecimal characters
      only */


typedef enum {
  Scanner_Operator_plus, Scanner_Operator_minus, Scanner_Operator_times,
  Scanner_Operator_div, Scanner_Operator_mod, Scanner_Operator_shiftLeft,
  Scanner_Operator_shiftRight, Scanner_Operator_or, Scanner_Operator_and,
  Scanner_Operator_complement, Scanner_Operator_assignment,
  Scanner_Operator_other
} Scanner_Operator;
  /** different operator tokens */


typedef struct {
  Scanner_TokenKind kind;
  String_Type representation;
  Scanner_Operator operator;
} Scanner_Token;
  /** token returned by the scanner */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Scanner_initialize (void);
  /** initializes the internal data structures of the scanner */

/*--------------------*/

void Scanner_finalize (void);
  /** cleans up the internal data structures of the scanner */

/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

void Scanner_makeToken (out Scanner_Token *token);
  /** initializes <token> */

/*--------------------*/

void Scanner_makeTokenList (out Scanner_TokenList *tokenList,
			    in String_Type st);
  /** scans <st> and returns all tokens found in <tokenList> */

/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Scanner_destroyToken (inout Scanner_Token *token);
  /** finalizes <token> */

/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Scanner_getNextToken (inout Scanner_Token *token);
  /** returns next token on current input stream in <token> */

/*--------------------*/

void Scanner_ungetToken (in Scanner_Token token);
  /** pushes back <token> to current input stream; this may be
      repeatedly called up to a limit of <pushbackStackSize> tokens
      simultaneously pushed back */

/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Scanner_tokenToString (in Scanner_Token token, out String_Type *st);
  /** returns representation of <token> in <st> */


/*--------------------*/
/* CONFIGURATION      */
/*--------------------*/

void Scanner_redirectInput (in Scanner_ReaderProc readerProc);
  /** tells that <readerProc> is the new routine for getting at the
      next character */

#endif /* __SCANNER_H */
