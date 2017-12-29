/** Parser module --
    This module provides all services for parsing tokenized character
    streams in the generic SDCC linker.

    The parser can parse a single object file or a list of them.  It
    normally calls other modules to build up internal object networks
    for an object file, but it also can on request do a reduced scan
    and simply return a list of symbols encountered e.g. when reading
    a library file.

    Other services can parse value assignment equations between some
    identifier and a long integer value.  Those parsing routines are
    generic because they do a callback to some key-value-assignment
    routine.  Note that in contrast to the original linker those
    routines cannot parse integer RHS expressions, but only simple
    values.

    Original version by Thomas Tensi, 2007-10
*/


#ifndef __PARSER_H
#define __PARSER_H

/*--------------------*/

#include "globdefs.h"
#include "stringlist.h"

/*--------------------*/

typedef struct {
  UINT8 defaultBase; /* the default base of number strings read */
  enum { littleEndian, bigEndian, unknown} endianness;
    /* the endianness of the numbers */
} Parser_Options;


typedef void (*Parser_KeyValueMappingProc)(in String_Type key, 
					   in long value);
  /** callback routine type for mapping string <key> to integer
      <value> to be used in <scanStringList> */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Parser_initialize (void);
  /** initializes the internal data structures of the parser */

/*--------------------*/

void Parser_finalize (void);
  /** cleans up the internal data structures of the parser */


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Parser_collectSymbolDefinitions (in String_Type objectFileName,
				      inout StringList_Type *symbolNameList);
 /** parses file given by <objectFileName> for symbol definitions in
     command S and returns them in <symbolNameList> */

/*--------------------*/

void Parser_setDefaultOptions (in Parser_Options options);
  /** sets the options for subsequent parsing  */

/*--------------------*/

void Parser_parseObjectFile (in Boolean isFirstPass, in String_Type fileName);
 /** parses the object file given by <fileName> for commands X, D, Q,
     H, M, A, S, T, R, and P; depending on whether this is in the
     first or second pass the processing is different */

/*--------------------*/

void Parser_parseObjectFiles (in Boolean isFirstPass,
			      in StringList_Type fileNameList);
 /** parses the object files in <fileNameList> for commands X, D, Q,
     H, M, A, S, T, R, and P; depending on whether this is in the
     first or second pass the processing is different */

/*--------------------*/

void Parser_setMappingFromList (in StringList_Type valueMapList,
			in Parser_KeyValueMappingProc setElementValueProc);
 /** parses the string list in <valueMapList> for lines of the form
     "name=value" and calls <setElementValueProc> for each
     (name,value) pair */

/*--------------------*/

void Parser_setMappingFromString (in String_Type valueMapString,
			in Parser_KeyValueMappingProc setElementValueProc);
 /** parses the string in <valueMapString> as a lines of the form
     "name=value" and calls <setElementValueProc> for given
     (name,value) pair */


#endif /* __PARSER_H */
