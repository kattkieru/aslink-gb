/** Symbol module --
    This module provides all services for handling external symbols
    within the SDCC linker.

    A symbol has a name, some segment and address which may be set and
    queried.  When a symbol is newly introduced one has to tell
    whether this introduction is a symbol definition or symbol
    reference.  This property may also be queried.

    For interbank calls a symbol may be split into a real and a
    surrogate symbol.  It is also possible to get a list of referenced
    but undefined symbols (necessary when checking the libraries).

    Original version by Thomas Tensi, 2004-09
*/

#ifndef __SYMBOL_H
#define __SYMBOL_H

/*========================================*/

#include "file.h"
#include "globdefs.h"
#include "list.h"
#include "set.h"
#include "string.h"
#include "target.h"
#include "typedescriptor.h"

/* the types are declared here because it is needed in area.h */
typedef struct Symbol__Record *Symbol_Type;
  /** a external or internal symbol within the linker (as an opaque
      type) */

typedef List_Type Symbol_List;
  /** a list of symbols */

#include "area.h"

/*========================================*/

extern TypeDescriptor_Type Symbol_typeDescriptor;
  /** variable used for describing the type properties when symbol
      objects occur in generic types like lists */

/*========================================*/

/*--------------------*/
/* MODULE SETUP/CLOSE */ 
/*--------------------*/

void Symbol_initialize (in Boolean platformIsCaseSensitive);
  /** sets up all internal data structures; <platformIsCaseSensitive>
      tells whether case matters for identifiers or not */

/*--------------------*/

void Symbol_finalize (void);
  /** cleans up all internal data structures */


/*--------------------*/
/* CONSTRUCTION       */
/*--------------------*/

Symbol_Type Symbol_make (in String_Type symbolName, in Boolean isDefinition,
			 in Target_Address startAddress);
  /** makes a new symbol with <symbolName>; additionally it is
      specified whether this is a definition and the <startAddress> */

/*--------------------*/

Symbol_Type Symbol_makeBySplit (in Symbol_Type oldSymbol,
				in String_Type symbolName);
  /** splits <oldSymbol> and creates new symbol with <symbolName>; all
      references to <oldSymbol> are transferred to new symbol; the new
      symbol is referenced but undefined, while the old symbol is
      defined but unreferenced */


/*--------------------*/
/* DESTRUCTION        */
/*--------------------*/

void Symbol_destroy (inout Symbol_Type *symbol);
  /** destroys <symbol> */


/*--------------------*/
/* ACCESS             */
/*--------------------*/

void Symbol_getName (in Symbol_Type symbol, out String_Type *name);
  /** returns name of <symbol> in <name>*/

/*--------------------*/

Area_Segment Symbol_getSegment (in Symbol_Type symbol);
  /** returns segment of <symbol> */

/*--------------------*/

Boolean Symbol_isDefined (in Symbol_Type symbol);
  /** tells whether <symbol> is defined in some module */

/*--------------------*/

Boolean Symbol_isSurrogate (in Symbol_Type symbol);
  /** tells whether <symbol> is a surrogate symbol (used for banking)*/


/*--------------------*/
/* CHANGE             */
/*--------------------*/

void Symbol_setAddressForName (in String_Type symbolName,
			       in Target_Address address);
  /** sets address of existing symbol with <symbolName> to <address> */


/*--------------------*/
/* MEASUREMENT        */
/*--------------------*/

Symbol_Type Symbol_lookup (in String_Type symbolName);
  /** returns <symbol> with <symbolName> or NULL when not found */

/*--------------------*/

Target_Address Symbol_absoluteAddress (in Symbol_Type symbol);
  /** returns absolute address of <symbol> (by adding the segment base
      address) */


/*--------------------*/
/* TRANSFORMATION     */
/*--------------------*/

void Symbol_getUndefinedSymbolList (inout Symbol_List *undefinedSymbolList);
  /** returns list of undefined symbols in <undefinedSymbolList> */

/*--------------------*/

void Symbol_checkForUndefinedSymbols (inout File_Type *file);
  /** scans the table of symbols for referenced but undefined symbols;
      for each of those symbols a message is output to <file> telling
      the module where a reference has been made */


/*--------------------*/
/* CONVERSION         */
/*--------------------*/

void Symbol_toString (in Symbol_Type symbol, out String_Type *representation);
  /** constructs a printable representation of <symbol> and its
      internal data (for debugging purposes) and concatenates it to
      <representation> */

#endif /* __SYMBOL_H */
